// Dear ImGui: 应用程序主入口 (Windows API + DirectX 11)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "DashboardPage.h"

//## 侧边栏与 UI 渲染辅助
struct NavigationItem {
    const char* label;
    int tab_index;
};

bool RenderSidebarItem(const char* label, bool is_selected, float main_scale) {
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    bool pressed = ImGui::Selectable(label, is_selected, 0, ImVec2(0, 38.0f * main_scale));
    ImGui::PopStyleVar();
    return pressed;
}

//## Windows 亚克力 (Acrylic) 效果支持
typedef enum _WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
} WINDOWCOMPOSITIONATTRIB;

typedef struct _ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIB_DATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIB_DATA;

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIB_DATA*);

void EnableAcrylicBlur(HWND hwnd, COLORREF colorWithAlpha) {
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser) {
        pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY policy = { 4, 0, (int)colorWithAlpha, 0 };
            WINDOWCOMPOSITIONATTRIB_DATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }
}

//## Direct3D 11 全局变量
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                    g_SwapChainOccluded = false;
static UINT                    g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

//## 前置函数声明
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//## 程序主入口
int main(int, char**)
{
    // 1. DPI 适配与初始化
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // 2. 注册窗口类
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);

    // 3. 创建无边框原生窗口
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Bodycam 工具箱 V3",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        100, 100,
        (int)(1280 * main_scale), (int)(800 * main_scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // 4. DWM 属性设置 (毛玻璃 & 暗黑模式)
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    BOOL USE_DARK_MODE = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &USE_DARK_MODE, sizeof(USE_DARK_MODE));

    // 5. 初始化 D3D 设备与显示窗口
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 6. ImGui 上下文与字体配置
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f * main_scale, &font_config, io.Fonts->GetGlyphRangesChineseFull());

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // 7. 主渲染循环
    bool done = false;
    while (!done)
    {
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

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 窗口覆盖配置与透明色调
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiStyle& current_style = ImGui::GetStyle();
        current_style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.75f);
        current_style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.15f, 0.17f, 0.60f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("MainWindow", nullptr, window_flags);

        // 自定义标题栏
        WINDOWPLACEMENT wp = { sizeof(wp) };
        ::GetWindowPlacement(hwnd, &wp);
        bool is_maximized = (wp.showCmd == SW_SHOWMAXIMIZED);

        float header_height = 32.0f * main_scale;
        float btn_w = 46.0f * main_scale;
        float btn_h = header_height;
        float total_btn_width = btn_w * 3.0f;
        float right_padding = is_maximized ? (8.0f * main_scale) : 0.0f;
        float top_padding = is_maximized ? (8.0f * main_scale) : 0.0f;

        // 标题栏拖拽响应区
        ImGui::SetCursorPos(ImVec2(0, top_padding));
        ImGui::InvisibleButton("##TitleDragArea", ImVec2(ImGui::GetWindowWidth() - total_btn_width - right_padding, header_height));

        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ::ReleaseCapture();
            ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        // 标题文本
        ImGui::SetCursorPos(ImVec2(12.0f * main_scale, top_padding + (header_height - 18.0f * main_scale) * 0.5f));
        ImGui::TextDisabled("  BODYCAM 工具箱 V3");

        // 控制按钮 (最小化 / 最大化 / 关闭)
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - total_btn_width - right_padding, top_padding));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.20f));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 icon_color = ImGui::GetColorU32(ImGuiCol_Text);

        // 最小化
        if (ImGui::Button("##MinBtn", ImVec2(btn_w, btn_h))) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
        ImVec2 min_p_min = ImGui::GetItemRectMin();
        ImVec2 min_p_max = ImGui::GetItemRectMax();
        float line_y = (min_p_min.y + min_p_max.y) * 0.5f;
        draw_list->AddLine(
            ImVec2(min_p_min.x + 17.0f * main_scale, line_y),
            ImVec2(min_p_max.x - 17.0f * main_scale, line_y),
            icon_color, 1.0f * main_scale
        );
        ImGui::SameLine();

        // 最大化 / 还原
        if (ImGui::Button("##MaxBtn", ImVec2(btn_w, btn_h))) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, is_maximized ? SC_RESTORE : SC_MAXIMIZE, 0);
        }
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        float w = p_max.x - p_min.x;
        float h = p_max.y - p_min.y;

        if (is_maximized) {
            draw_list->AddRect(ImVec2(p_min.x + w * 0.40f, p_min.y + h * 0.30f), ImVec2(p_min.x + w * 0.68f, p_min.y + h * 0.58f), icon_color, 0.0f, 0, 1.0f * main_scale);
            draw_list->AddRect(ImVec2(p_min.x + w * 0.32f, p_min.y + h * 0.42f), ImVec2(p_min.x + w * 0.60f, p_min.y + h * 0.70f), icon_color, 0.0f, 0, 1.0f * main_scale);
        }
        else {
            draw_list->AddRect(ImVec2(p_min.x + w * 0.36f, p_min.y + h * 0.34f), ImVec2(p_min.x + w * 0.64f, p_min.y + h * 0.66f), icon_color, 0.0f, 0, 1.0f * main_scale);
        }
        ImGui::SameLine();

        // 关闭按钮
        ImGui::PopStyleColor(2);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.11f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.10f, 0.12f, 1.0f));

        if (ImGui::Button("X", ImVec2(btn_w, btn_h))) {
            ::PostQuitMessage(0);
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorPosY(header_height + top_padding);
        ImGui::Separator();

        // 分栏布局
        if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 200.0f * main_scale);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // 左侧侧边栏
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("BODYCAM 工具箱");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
            ImGui::Selectable("首页", true, 0, ImVec2(0, 38.0f * main_scale));
            ImGui::PopStyleVar();

            // 右侧主展示区
            ImGui::TableSetColumnIndex(1);
            RenderDashboardPage(hwnd, main_scale);

            ImGui::EndTable();
        }

        ImGui::End();

        // 画面呈现与清屏
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // 8. 资源清理退出
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

//## Direct3D 11 辅助函数实现
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

//## Windows 消息回调过程 (WndProc)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);

        RECT rect;
        GetClientRect(hWnd, &rect);

        const int border_width = 8;

        bool left = pt.x < border_width;
        bool right = pt.x > rect.right - border_width;
        bool top = pt.y < border_width;
        bool bottom = pt.y > rect.bottom - border_width;

        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;

        return HTCLIENT;
    }

    case WM_NCCALCSIZE:
    {
        if (wParam == TRUE)
            return 0;
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