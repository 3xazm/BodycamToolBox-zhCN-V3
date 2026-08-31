#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <cmath>
#include <algorithm>
#include <vector>

// ImGui 核心与后端头文件
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// 拆分后的自定义功能模块头文件
#include "RainEffectPipeline.h"  // 雨滴背景特效渲染管线
#include "Win32_API.h"           // 亚克力 / 毛玻璃 Win32 API 扩展
#include "Direct3D_Resource.h"   // Direct3D 11 基础设备与渲染目标管理
#include "UI_Controls.h"         // 自定义 UI 基础控件
#include "UI_Theme.h"            // Apple Glass 玻璃主题及动画状态定义
#include "UI_Header.h"           // 顶部标题栏、搜索框与窗口控制区模块
#include "UI_Sidebar.h"          // 侧边栏及液态流体胶囊动画模块
#include "MainViews.h"           // 右侧主视图/选项卡内容渲染模块

// 链接必要的系统库
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

// 全局雨滴特效渲染管线实例
static RainEffectPipeline g_RainPipeline;

// Win32 消息处理函数前置声明
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 程序主入口
int main(int, char**) {
    // ------------------------------------------------------------------------
    // 1. 初始化 Win32 高 DPI 缩放与窗口创建
    // ------------------------------------------------------------------------
    ImGui_ImplWin32_EnableDpiAwareness(); // 开启 ImGui DPI 感知
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY)); // 获取当前主显示器的缩放比例

    // 注册窗口类
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"LiquidGlassApp", nullptr };
    ::RegisterClassExW(&wc);

    // 创建主窗口（带可调整边框与最大化/最小化按钮）
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"BODYCAM TOOLKIT V3",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        150, 150, (int)(1000 * scale), (int)(620 * scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // 配置 DWM 窗口边角效果与暗色主题
    DWORD cornerPreference = 2; // 启用圆角
    DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)33, &cornerPreference, sizeof(cornerPreference));

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    EnableAcrylic(hwnd, 0x00000000); // 启用 Windows 亚克力 (Acrylic) 效果

    // ------------------------------------------------------------------------
    // 2. 初始化 Direct3D 11 与背景管线
    // ------------------------------------------------------------------------
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    g_RainPipeline.Init(g_pd3dDevice); // 初始化雨滴着色器管线

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // ------------------------------------------------------------------------
    // 3. 初始化 ImGui 上下文与样式配置
    // ------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // 加载系统微软雅黑字体（支持中文渲染）
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * scale, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    SetupAppleGlassTheme(scale); // 设置 Apple 玻璃风格主题样式

    // 初始化 ImGui 的 Win32 与 DirectX11 后端绑定
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // ------------------------------------------------------------------------
    // 4. UI 运行时状态变量
    // ------------------------------------------------------------------------
    static int currentTab = 0;              // 当前选中的 Tab 页索引
    static char searchBuffer[128] = "";     // 搜索框文本缓冲区
    static LiquidAnimationState liquidState;// 侧边栏水痕流体胶囊动画状态

    bool done = false;

    // ------------------------------------------------------------------------
    // 5. 主渲染与消息循环
    // ------------------------------------------------------------------------
    while (!done) {
        // 处理 Windows 事件消息
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // 处理窗口遮挡或最小化休眠
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // 处理窗口尺寸改变时的 RenderTarget 调整
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // 渲染背景动态雨滴特效
        g_RainPipeline.Resize(g_pd3dDevice, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        g_RainPipeline.Render(g_pd3dDeviceContext, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f, io.DeltaTime);

        // ImGui 帧初始化
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 覆盖整个窗口作为 UI 主画布
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * scale, 12 * scale));
        ImGui::Begin("MainWindow", nullptr, flags);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 绘制背景雨滴纹理
        if (g_RainPipeline.pSRV) {
            drawList->AddImage((ImTextureID)g_RainPipeline.pSRV, ImVec2(0, 0), io.DisplaySize);
        }

        ImVec2 windowSize = ImGui::GetWindowSize();

        // 绘制窗口外层半透明柔和高光边框
        drawList->AddRect(
            ImVec2(0, 0), windowSize,
            IM_COL32(255, 255, 255, 80), 16.0f * scale, 0, 1.5f * scale
        );

        // --- 模块 1：顶部 Header 区域渲染 (标题、搜索、窗口按钮) ---
        float headerH = 42.0f * scale;
        RenderHeader(hwnd, drawList, windowSize, scale, headerH, searchBuffer, IM_ARRAYSIZE(searchBuffer));

        // --- 布局坐标计算 ---
        float contentStartY = headerH + 16.0f * scale;
        float contentH = windowSize.y - contentStartY - 16.0f * scale;
        float sidebarW = 200.0f * scale;
        ImVec2 sidebarPos(16.0f * scale, contentStartY);

        // --- 模块 2：左侧 Sidebar 区域渲染 (导航与流体胶囊) ---
        RenderSidebar(drawList, currentTab, liquidState, sidebarPos, sidebarW, contentH, scale, io.DeltaTime);

        // --- 模块 3：右侧 Main Content 内容面板渲染 ---
        float mainX = sidebarPos.x + sidebarW + 16.0f * scale;
        float mainW = windowSize.x - mainX - 16.0f * scale;

        ImGui::SetCursorPos(ImVec2(mainX, contentStartY));

        if (ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true)) {
            RenderMainViews(currentTab, scale); // 根据 currentTab 渲染具体页面内容
        }

        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleVar();

        // --------------------------------------------------------------------
        // 6. 最终画面提交与 D3D11 呈现
        // --------------------------------------------------------------------
        ImGui::Render();
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // 开启垂直同步呈现
    }

    // ------------------------------------------------------------------------
    // 7. 资源清理与程序退出
    // ------------------------------------------------------------------------
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_RainPipeline.Shutdown();
    if (g_pBlendState) g_pBlendState->Release();
    CleanupDeviceD3D();

    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ----------------------------------------------------------------------------
// Win32 窗口消息回调函数 (支持无边框拖拽调整大小)
// ----------------------------------------------------------------------------
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_NCHITTEST: {
        // 自定义边缘拖拽检测（允许无边框窗口按四周边缘拉伸大小）
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rect; GetClientRect(hWnd, &rect);
        const int bw = 8; // 边缘判定宽度
        if (pt.y < bw && pt.x < bw) return HTTOPLEFT;
        if (pt.y < bw && pt.x > rect.right - bw) return HTTOPRIGHT;
        if (pt.y > rect.bottom - bw && pt.x < bw) return HTBOTTOMLEFT;
        if (pt.y > rect.bottom - bw && pt.x > rect.right - bw) return HTBOTTOMRIGHT;
        if (pt.x < bw) return HTLEFT;
        if (pt.x > rect.right - bw) return HTRIGHT;
        if (pt.y < bw) return HTTOP;
        if (pt.y > rect.bottom - bw) return HTBOTTOM;
        return HTCLIENT;
    }
    case WM_NCCALCSIZE:
        // 移除标准标题栏与客户区边框
        if (wParam == TRUE) return 0;
        return 0;
    case WM_SIZE:
        // 响应窗口最大化/还原/尺寸调整事件
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}