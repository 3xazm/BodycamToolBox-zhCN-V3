#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <gdiplus.h> // 使用 GDI+ 原生解码内存图片

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Win32_API.h"
#include "Direct3D_Resource.h"
#include "AppRenderer.h"

#include "BodycamAppIcon.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib") // 链接 GDI+ 库
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool g_IsRendering = false;

void SafeRenderFrame() {
    if (g_IsRendering) return;
    g_IsRendering = true;
    RenderFrame();
    g_IsRendering = false;
}

// 使用 GDI+ 从内存中的 PNG/JPEG 字节流无损安全转换 HICON
HICON CreateHIconFromMemory() {
    if (!BodycamAppIconData || BodycamAppIconDataSize == 0) return nullptr;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, BodycamAppIconDataSize);
    if (!hMem) return nullptr;

    void* pMem = GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        return nullptr;
    }
    memcpy(pMem, BodycamAppIconData, BodycamAppIconDataSize);
    GlobalUnlock(hMem);

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        return nullptr;
    }

    HICON hIcon = nullptr;
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
    if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok) {
        pBitmap->GetHICON(&hIcon);
        delete pBitmap;
    }

    pStream->Release(); // 释放内存流
    return hIcon;
}

int main(int, char**) {
    // 1. 在程序启动时正确初始化 GDI+ 环境
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // 安全生成图标
    HICON hIcon = CreateHIconFromMemory();

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L, 0L,
        hInstance,
        hIcon,                    // 大图标
        nullptr,
        nullptr,
        nullptr,
        L"LiquidGlassApp",
        hIcon                     // 小图标
    };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"Bodycam工具箱V3",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        150, 150, (int)(1000 * scale), (int)(620 * scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    DWORD cornerPreference = 2;
    DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)33, &cornerPreference, sizeof(cornerPreference));

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    EnableAcrylic(hwnd, 0x00000000);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    InitAppRenderer(hwnd, scale);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * scale, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    SetupAppleGlassTheme(scale);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

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

        SafeRenderFrame();
    }

    if (hIcon) DestroyIcon(hIcon);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ShutdownAppRenderer();
    if (g_pBlendState) g_pBlendState->Release();
    CleanupDeviceD3D();

    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // 2. 退出前清理 GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_ENTERSIZEMOVE:
        SetTimer(hWnd, 1, 1, nullptr);
        break;
    case WM_EXITSIZEMOVE:
        KillTimer(hWnd, 1);
        break;
    case WM_MOVING:
    case WM_SIZING:
    case WM_TIMER:
        if (msg != WM_TIMER || wParam == 1) {
            SafeRenderFrame();
        }
        break;
    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rect; GetClientRect(hWnd, &rect);

        const int bw = static_cast<int>(8.0f * g_Scale);

        if (pt.y < bw && pt.x < bw) return HTTOPLEFT;
        if (pt.y < bw && pt.x > rect.right - bw) return HTTOPRIGHT;
        if (pt.y > rect.bottom - bw && pt.x < bw) return HTBOTTOMLEFT;
        if (pt.y > rect.bottom - bw && pt.x > rect.right - bw) return HTBOTTOMRIGHT;
        if (pt.x < bw) return HTLEFT;
        if (pt.x > rect.right - bw) return HTRIGHT;
        if (pt.y < bw) return HTTOP;
        if (pt.y > rect.bottom - bw) return HTBOTTOM;

        float headerH = 42.0f * g_Scale;
        float rightReservedW = 120.0f * g_Scale;

        if (pt.y >= bw && pt.y < headerH && pt.x < rect.right - rightReservedW) {
            return HTCAPTION;
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