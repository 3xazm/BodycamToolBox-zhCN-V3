// Dear ImGui: 应用程序主入口 (Windows API + DirectX 11)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// ============================================================================
// Windows 内部未公开 API：亚克力 (Acrylic) / 毛玻璃效果结构定义
// ============================================================================

typedef enum _WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
} WINDOWCOMPOSITIONATTRIB;

typedef struct _ACCENT_POLICY
{
    int AccentState;
    int AccentFlags;
    int GradientColor; // ABGR 格式 (控制亚克力背景色与透明度)
    int AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIB_DATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIB_DATA;

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIB_DATA*);

// 开启系统级亚克力模糊背景
void EnableAcrylicBlur(HWND hwnd, COLORREF colorWithAlpha) {
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser) {
        pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute) {
            // AccentState = 4 代表开启 ACCENT_ENABLE_BLURBEHIND / ACCENT_ENABLE_ACRYLICBLUR
            ACCENT_POLICY policy = { 4, 0, (int)colorWithAlpha, 0 };
            WINDOWCOMPOSITIONATTRIB_DATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }
}

// ============================================================================
// Direct3D 11 全局渲染变量
// ============================================================================

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// ============================================================================
// 前置函数声明
// ============================================================================
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // <-- 只保留这一行声明，把大括号 { ... } 整个函数体删掉

// ============================================================================
// 程序主入口
// ============================================================================

int main(int, char**)
{
    // 1. 设置系统 DPI 适配，获取主显示器缩放比例
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // 2. 注册 Windows 窗口类
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);

    // 3. 创建原生窗口：包含系统菜单与缩放边框（保留动画），但排除 WS_CAPTION（隐藏系统原生标题栏）
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Bodycam 工具箱 V3",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        100, 100,
        (int)(1280 * main_scale), (int)(800 * main_scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // 4. 配置 DWM 窗口属性 (开启毛玻璃与 Win10/11 深色模式标题栏支持)
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    BOOL USE_DARK_MODE = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &USE_DARK_MODE, sizeof(USE_DARK_MODE)); // DWMWA_USE_IMMERSIVE_DARK_MODE

    // 5. 初始化 Direct3D 11 渲染设备
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // 6. 显示并更新窗口
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 7. 初始化 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // 8. 配置字体 (加载系统微软雅黑，支持全中文集)
    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas = false; // 保持内存安全
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f * main_scale, &font_config, io.Fonts->GetGlyphRangesChineseFull());

    // 9. 开启键盘与手柄导航支持
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // 10. 全局 UI 控件 DPI 缩放适配
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // 11. 初始化 ImGui 渲染后端 (Win32 + DX11)
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // 状态控制变量
    ImVec4 clear_color = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);

    // ============================================================================
    // 渲染主循环 (Main Loop)
    // ============================================================================
    bool done = false;
    while (!done)
    {
        // 消息泵处理
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // 窗口最小化/遮挡时的挂起逻辑
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // 响应窗口尺寸变更
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // 开启 ImGui 新一帧
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 当前选中的侧边栏 Tab 索引
        static int current_tab = 0;

        // 设置 ImGui 主窗口覆盖整个 Win32 客户区
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        // 设置 ImGui 窗口半透明度 (配合底层 DWM 透出毛玻璃)
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.75f); // Alpha = 0.75 (半透明)
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.15f, 0.17f, 0.60f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("MainWindow", nullptr, window_flags);

        // ------------------------------------------------------------------------
        // 自定义窗口标题栏 (Windows 原生尺寸与精细图标)
        // ------------------------------------------------------------------------
        float header_height = 32.0f * main_scale; // 整体标题栏高度拉大到 32px
        float btn_w = 46.0f * main_scale;         // 按钮宽度参照 Windows 原生的 46px
        float btn_h = header_height;              // 按钮高度贴满整个标题栏

        // 右上角 3 个按钮的总宽度 (46 * 3 = 138)
        float total_btn_width = btn_w * 3.0f;

        // 1. 创建标题栏拖拽区域 (留出右侧按钮区)
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##TitleDragArea", ImVec2(ImGui::GetWindowWidth() - total_btn_width, header_height));

        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ::ReleaseCapture();
            ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        // 2. 绘制标题文本 (居中偏左放置)
        ImGui::SetCursorPos(ImVec2(12.0f * main_scale, (header_height - 18.0f * main_scale) * 0.5f));
        ImGui::TextDisabled("  BODYCAM 工具箱 V3");

        // 3. 绘制右侧控制按钮 (无缝贴合窗口右上角)
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - total_btn_width, 0.0f));

        // 临时去除按钮圆角与内边距，使其紧密拼合
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // 基础按钮颜色配置 (常态透明)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.20f));

        // 获取公用的画笔与颜色定义 (只在这里声明一次)
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 icon_color = ImGui::GetColorU32(ImGuiCol_Text);

        // --- 最小化按钮 ---
        if (ImGui::Button("##MinBtn", ImVec2(btn_w, btn_h))) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }

        // 绘制最小化横线
        ImVec2 min_p_min = ImGui::GetItemRectMin();
        ImVec2 min_p_max = ImGui::GetItemRectMax();
        float line_y = (min_p_min.y + min_p_max.y) * 0.5f;
        draw_list->AddLine(
            ImVec2(min_p_min.x + 17.0f * main_scale, line_y),
            ImVec2(min_p_max.x - 17.0f * main_scale, line_y),
            icon_color,
            1.0f * main_scale
        );
        ImGui::SameLine();

        // --- 最大化 / 还原按钮 ---
        WINDOWPLACEMENT wp = { sizeof(wp) };
        ::GetWindowPlacement(hwnd, &wp);
        bool is_maximized = (wp.showCmd == SW_SHOWMAXIMIZED);

        if (ImGui::Button("##MaxBtn", ImVec2(btn_w, btn_h))) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, is_maximized ? SC_RESTORE : SC_MAXIMIZE, 0);
        }

        // 获取按钮的实际绘制矩形区域
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        float w = p_max.x - p_min.x;
        float h = p_max.y - p_min.y;

        // 使用百分比相对定位，确保全屏/最大化时绝对不变形
        if (is_maximized) {
            // 还原状态：双叠框 ❐
            // 后方框
            draw_list->AddRect(
                ImVec2(p_min.x + w * 0.40f, p_min.y + h * 0.30f),
                ImVec2(p_min.x + w * 0.68f, p_min.y + h * 0.58f),
                icon_color, 0.0f, 0, 1.0f * main_scale
            );
            // 前方框
            draw_list->AddRect(
                ImVec2(p_min.x + w * 0.32f, p_min.y + h * 0.42f),
                ImVec2(p_min.x + w * 0.60f, p_min.y + h * 0.70f),
                icon_color, 0.0f, 0, 1.0f * main_scale
            );
        }
        else {
            // 最大化状态：单框 口
            draw_list->AddRect(
                ImVec2(p_min.x + w * 0.36f, p_min.y + h * 0.34f),
                ImVec2(p_min.x + w * 0.64f, p_min.y + h * 0.66f),
                icon_color, 0.0f, 0, 1.0f * main_scale
            );
        }
        ImGui::SameLine();

        // --- 关闭按钮 (悬停红色) ---
        ImGui::PopStyleColor(2); // 弹出通用 Hover/Active
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.11f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.10f, 0.12f, 1.0f));

        if (ImGui::Button("X", ImVec2(btn_w, btn_h))) {
            ::PostQuitMessage(0);
        }

        // 清除推入的临时样式
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        // 移动光标到标题栏下方
        ImGui::SetCursorPosY(header_height);
        ImGui::Separator();

        // ------------------------------------------------------------------------
        // 主界面分栏布局 (利用 ImGui Table 实现左右响应式布局)
        // ------------------------------------------------------------------------
        if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            // 第一列：左侧导航栏 (固定宽度 200px)
            ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 200.0f * main_scale);
            // 第二列：右侧内容区 (自动拉伸)
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // --- 左侧侧边导航栏 ---
            ImGui::TableSetColumnIndex(0);

            ImGui::TextDisabled("BODYCAM 工具箱");
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Selectable("  首页 / 仪表盘", current_tab == 0, 0, ImVec2(0, 35.0f * main_scale))) {
                current_tab = 0;
            }
            if (ImGui::Selectable("  游戏参数优化", current_tab == 1, 0, ImVec2(0, 35.0f * main_scale))) {
                current_tab = 1;
            }
            if (ImGui::Selectable("  画质与画幅", current_tab == 2, 0, ImVec2(0, 35.0f * main_scale))) {
                current_tab = 2;
            }

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::Selectable("  软件设置", current_tab == 3, 0, ImVec2(0, 35.0f * main_scale))) {
                current_tab = 3;
            }

            // --- 右侧核心功能展示区 ---
            ImGui::TableSetColumnIndex(1);

            switch (current_tab)
            {
            case 0: // 首页 / 仪表盘
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "欢迎使用 Bodycam 工具箱");
                ImGui::Separator();
                ImGui::Text("系统状态：已就绪");
                ImGui::Spacing();

                static char game_path[256] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Bodycam";
                ImGui::InputText("游戏根目录", game_path, sizeof(game_path));

                if (ImGui::Button("一键优化配置", ImVec2(150, 40))) {
                    MessageBoxW(hwnd, L"配置优化完成！", L"提示", MB_OK);
                }
                break;
            }
            case 1: // 游戏参数优化
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "游戏动态参数调节");
                ImGui::Separator();

                static bool enable_fov = true;
                static float fov_value = 90.0f;
                ImGui::Checkbox("解锁宽视野 (FOV)", &enable_fov);
                if (enable_fov) {
                    ImGui::SliderFloat("FOV 角度", &fov_value, 60.0f, 120.0f);
                }

                static bool remove_blur = false;
                ImGui::Checkbox("禁用运动模糊 (Motion Blur)", &remove_blur);
                break;
            }
            case 2: // 画质与画幅
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "显示与渲染设置");
                ImGui::Separator();

                static int render_scale = 100;
                ImGui::SliderInt("渲染分辨率比例 (%)", &render_scale, 50, 200);
                break;
            }
            case 3: // 软件设置
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "工具箱偏好设置");
                ImGui::Separator();

                ImGui::ColorEdit3("背景渲染颜色", (float*)&clear_color);
                break;
            }
            }

            ImGui::EndTable();
        }

        ImGui::End(); // 结束 MainWindow

        // ------------------------------------------------------------------------
        // 画面渲染与帧呈现 (Rendering)
        // ------------------------------------------------------------------------
        ImGui::Render();

        // 清屏颜色 Alpha 设为 0.0f，保证底色完全透明，透出 Windows 系统级的亚克力磨砂
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // 垂直同步交换缓冲链 (Present)
        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // ============================================================================
    // 程序退出与资源释放
    // ============================================================================
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// ============================================================================
// Direct3D 11 辅助功能函数实现
// ============================================================================

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// 转发 ImGui 的 Win32 消息处理函数
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Windows 消息回调过程 (WndProc)
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    // 拦截非客户区计算：彻底移除系统标题栏，同时保留 DWM 窗口动画与阴影
    case WM_NCCALCSIZE:
    {
        if (wParam == TRUE)
        {
            // 通过直接返回 0，告诉 Windows 整个窗口空间都属于客户区
            NCCALCSIZE_PARAMS* pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            // 将非客户区的边框偏移量清零
            pncsp->rgrc[0] = pncsp->rgrc[0];
            return 0;
        }
        return 0;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}