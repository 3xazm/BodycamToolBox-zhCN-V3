#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Win32_API.h"
#include "Direct3D_Resource.h"
#include "AppRenderer.h" // 包含所有的全局 UI 声明与 AppRenderer 接口

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 渲染防重入标志
static bool g_IsRendering = false;

// 包装安全的渲染调用（防止在消息回调和主循环中同时渲染导致 ImGui 崩溃）
void SafeRenderFrame() {
    if (g_IsRendering) return; // 如果上一帧还没渲染完，直接跳过
    g_IsRendering = true;
    RenderFrame();
    g_IsRendering = false;
}

int main(int, char**) {
    // 1. 初始化 Win32 高 DPI 与窗口
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"LiquidGlassApp", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"BODYCAM TOOLKIT V3",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        150, 150, (int)(1000 * scale), (int)(620 * scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    DWORD cornerPreference = 2;
    DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)33, &cornerPreference, sizeof(cornerPreference));

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    EnableAcrylic(hwnd, 0x00000000);

    // 2. 初始化 Direct3D 11
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // 初始化 AppRenderer 模块
    InitAppRenderer(hwnd, scale);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 3. 初始化 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * scale, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    SetupAppleGlassTheme(scale);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // 4. 主循环
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // 使用安全的渲染函数
        SafeRenderFrame();
    }

    // 5. 资源清理
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ShutdownAppRenderer();
    if (g_pBlendState) g_pBlendState->Release();
    CleanupDeviceD3D();

    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// 消息处理回调
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_ENTERSIZEMOVE: {
        // 开启 1ms 级别的最高频率 Timer 驱动拖动和拉伸时的实时刷新
        SetTimer(hWnd, 1, 1, nullptr);
        break;
    }
    case WM_EXITSIZEMOVE: {
        KillTimer(hWnd, 1);
        break;
    }
    case WM_MOVING:
    case WM_SIZING:
    case WM_TIMER: {
        if (msg != WM_TIMER || wParam == 1) {
            SafeRenderFrame(); // 实时驱动 SafeRenderFrame 重绘，解决雨滴停止和黑边断层问题！
        }
        break;
    }
    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rect; GetClientRect(hWnd, &rect);

        const int bw = static_cast<int>(8.0f * g_Scale);

        // 1. 八方向调整窗口大小边框
        if (pt.y < bw && pt.x < bw) return HTTOPLEFT;
        if (pt.y < bw && pt.x > rect.right - bw) return HTTOPRIGHT;
        if (pt.y > rect.bottom - bw && pt.x < bw) return HTBOTTOMLEFT;
        if (pt.y > rect.bottom - bw && pt.x > rect.right - bw) return HTBOTTOMRIGHT;
        if (pt.x < bw) return HTLEFT;
        if (pt.x > rect.right - bw) return HTRIGHT;
        if (pt.y < bw) return HTTOP;
        if (pt.y > rect.bottom - bw) return HTBOTTOM;

        // 2. 将 Header 顶部空白区域标记为原生标题栏（支持无抖动拖拽 + 原生双击最大化）
        float headerH = 42.0f * g_Scale;
        float rightReservedW = 120.0f * g_Scale; // 预留右上角 Mac 控制按钮区域给 ImGui

        if (pt.y >= bw && pt.y < headerH && pt.x < rect.right - rightReservedW) {
            return HTCAPTION; // 告诉 Windows 这块就是标题栏！
        }

        return HTCLIENT;
    }
    case WM_NCCALCSIZE:
        if (wParam == TRUE) return 0;
        return 0;
    case WM_SIZE:
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