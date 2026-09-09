#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Win32_API.h"
#include "Direct3D_Resource.h"
#include "AppRenderer.h"

// 引入 stb_image 解码库与图片内存数据
#include "stb_image.h"
#include "BodycamAppIcon.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 渲染防重入标志
static bool g_IsRendering = false;

void SafeRenderFrame() {
    if (g_IsRendering) return;
    g_IsRendering = true;
    RenderFrame();
    g_IsRendering = false;
}

// 从 BodycamAppIcon.h 内存字节流动态创建 Win32 HICON
HICON CreateHIconFromMemory() {
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(
        BodycamAppIconData,
        (int)BodycamAppIconDataSize,
        &width, &height, &channels, 4
    );
    if (!pixels) return nullptr;

    BITMAPV5HEADER bi = {};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = width;
    bi.bV5Height = -height; // Top-down
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc = GetDC(nullptr);
    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    if (hBitmap && pBits) {
        for (int i = 0; i < width * height; ++i) {
            unsigned char* src = pixels + i * 4;
            unsigned char* dst = (unsigned char*)pBits + i * 4;
            dst[0] = src[2]; // B
            dst[1] = src[1]; // G
            dst[2] = src[0]; // R
            dst[3] = src[3]; // A
        }
    }

    HBITMAP hMonoBitmap = CreateBitmap(width, height, 1, 1, nullptr);

    HICON hIcon = nullptr;
    if (hBitmap && hMonoBitmap) {
        ICONINFO ii = {};
        ii.fIcon = TRUE;
        ii.hbmMask = hMonoBitmap;
        ii.hbmColor = hBitmap;
        hIcon = CreateIconIndirect(&ii);
    }

    // 修复 C28183 警告：判断空指针后再释放
    if (hBitmap) DeleteObject(hBitmap);
    if (hMonoBitmap) DeleteObject(hMonoBitmap);
    stbi_image_free(pixels);

    return hIcon;
}

int main(int, char**) {
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

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
    return 0;
}

// 消息处理回调（补充被遗漏的实现）
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