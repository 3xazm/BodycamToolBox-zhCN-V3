#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <cmath>
#include <algorithm>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "RainEffectPipeline.h"
#include "Win32_API.h"
#include "Direct3D_Resource.h"
#include "UI_Controls.h" // 引入抽离出的 UI 控件

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

static RainEffectPipeline g_RainPipeline;

// Forward declarations
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void SetupAppleGlassTheme(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 16.0f * scale;
    style.ChildRounding = 12.0f * scale;
    style.FrameRounding = 8.0f * scale;
    style.PopupRounding = 10.0f * scale;
    style.ScrollbarRounding = 10.0f * scale;
    style.GrabRounding = 8.0f * scale;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.12f, 0.30f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
}

int main(int, char**) {
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

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    g_RainPipeline.Init(g_pd3dDevice);

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

    static int currentTab = 0;
    static char searchBuffer[128] = "";

    static float liquidCurrentY = -1.0f;
    static bool isFirstFrameLiquid = true;
    static float fluidWeight = 6.0f;
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

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        g_RainPipeline.Resize(g_pd3dDevice, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        g_RainPipeline.Render(g_pd3dDeviceContext, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f, io.DeltaTime);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * scale, 12 * scale));
        ImGui::Begin("MainWindow", nullptr, flags);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (g_RainPipeline.pSRV) {
            drawList->AddImage((ImTextureID)g_RainPipeline.pSRV, ImVec2(0, 0), io.DisplaySize);
        }

        ImVec2 windowSize = ImGui::GetWindowSize();

        drawList->AddRect(
            ImVec2(0, 0), windowSize,
            IM_COL32(255, 255, 255, 80), 16.0f * scale, 0, 1.5f * scale
        );

        // Header 区域
        float headerH = 42.0f * scale;

        ImVec2 titleCapsuleSize(200.0f * scale, 34.0f * scale);
        ImVec2 titlePos(16.0f * scale, 12.0f * scale);
        drawList->AddRectFilled(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 25), 17.0f * scale);
        drawList->AddRect(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 80), 17.0f * scale);

        ImVec2 titleTextSize = ImGui::CalcTextSize("Bodycam工具箱V3");
        drawList->AddText(ImVec2(titlePos.x + (titleCapsuleSize.x - titleTextSize.x) * 0.5f, titlePos.y + (titleCapsuleSize.y - titleTextSize.y) * 0.5f), IM_COL32(255, 255, 255, 230), "Bodycam工具箱V3");

        float searchW = 300.0f * scale;
        float searchX = (windowSize.x - searchW) * 0.5f;
        ImGui::SetCursorPos(ImVec2(searchX, 12.0f * scale));
        ImGui::PushItemWidth(searchW);
        ImGui::InputTextWithHint("##Search", "搜索...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        ImGui::PopItemWidth();

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##TitleDrag", ImVec2(windowSize.x - 120.0f * scale, headerH));
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ::ReleaseCapture();
            ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        float circleR = 13.0f * scale;
        float btnY = 24.0f * scale;
        float rightBaseX = windowSize.x - 24.0f * scale;
        float btnSpacing = 30.0f * scale;

        bool isMaximized = ::IsZoomed(hwnd);

        if (DrawMacCircleButton("CloseBtn", ImVec2(rightBaseX, btnY), circleR, MAC_BTN_CLOSE)) {
            ::PostQuitMessage(0);
        }
        if (DrawMacCircleButton("MaxBtn", ImVec2(rightBaseX - btnSpacing, btnY), circleR, MAC_BTN_MAXIMIZE, isMaximized)) {
            if (isMaximized) ::SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            else ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
        if (DrawMacCircleButton("MinBtn", ImVec2(rightBaseX - btnSpacing * 2.0f, btnY), circleR, MAC_BTN_MINIMIZE)) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }

        // 主体布局
        float contentStartY = headerH + 16.0f * scale;
        float contentH = windowSize.y - contentStartY - 16.0f * scale;
        float sidebarW = 200.0f * scale;

        ImVec2 sidebarPos(16.0f * scale, contentStartY);
        drawList->AddRectFilled(sidebarPos, ImVec2(sidebarPos.x + sidebarW, sidebarPos.y + contentH), IM_COL32(255, 255, 255, 15), 16.0f * scale);
        drawList->AddRect(sidebarPos, ImVec2(sidebarPos.x + sidebarW, sidebarPos.y + contentH), IM_COL32(255, 255, 255, 50), 16.0f * scale);

        float optItemW = sidebarW - 20.0f * scale;
        float optItemH = 38.0f * scale;

        ImVec2 optPositions[3];

        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale));
        if (DrawSidebarOption("首页", currentTab == 0, ImVec2(optItemW, optItemH), &optPositions[0])) {
            currentTab = 0;
        }

        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + optItemH + 8.0f * scale));
        if (DrawSidebarOption("分辨率修复", currentTab == 1, ImVec2(optItemW, optItemH), &optPositions[1])) {
            currentTab = 1;
        }

        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + contentH - optItemH - 12.0f * scale));
        if (DrawSidebarOption("设置", currentTab == 2, ImVec2(optItemW, optItemH), &optPositions[2])) {
            currentTab = 2;
        }

        // 蒸发水痕与流动胶囊系统
        struct TrailSegment {
            float y;
            float alpha;
            float width;
            float height;
        };
        static std::vector<TrailSegment> waterTrails;

        static float liquidVelY = 0.0f;
        static float liquidStretch = 0.0f;
        static float lastY = -1.0f;

        float targetY = optPositions[currentTab].y - optItemH * 0.5f;

        if (isFirstFrameLiquid) {
            liquidCurrentY = targetY;
            lastY = targetY;
            isFirstFrameLiquid = false;
        }
        else {
            float dist = targetY - liquidCurrentY;

            float stiffness = (dist > 0.0f) ? 200.0f : 230.0f;
            float damping = (std::abs(dist) < 12.0f) ? 6.0f : 12.0f;

            float force = dist * stiffness;
            liquidVelY += force * io.DeltaTime;
            liquidVelY -= liquidVelY * damping * io.DeltaTime;
            liquidCurrentY += liquidVelY * io.DeltaTime;

            float targetStretch = (std::abs(liquidVelY) / 500.0f);
            targetStretch = (std::min)(targetStretch, 0.5f);
            liquidStretch = CustomLerp(liquidStretch, targetStretch, io.DeltaTime * 18.0f);

            if (std::abs(liquidCurrentY - lastY) > 4.0f * scale) {
                TrailSegment seg;
                seg.y = (liquidCurrentY + lastY) * 0.5f + optItemH * 0.5f;
                seg.alpha = 0.45f;
                seg.width = (optItemW - 30.0f * scale) * (1.0f - liquidStretch * 0.2f);
                seg.height = std::abs(liquidCurrentY - lastY) + 6.0f * scale;
                waterTrails.push_back(seg);
                lastY = liquidCurrentY;
            }
        }

        for (auto it = waterTrails.begin(); it != waterTrails.end(); ) {
            it->alpha -= io.DeltaTime * 1.2f;

            if (it->alpha <= 0.0f) {
                it = waterTrails.erase(it);
            }
            else {
                float trailCenterX = sidebarPos.x + 10.0f * scale + optItemW * 0.5f;
                ImVec2 tMin(trailCenterX - it->width * 0.5f, it->y - it->height * 0.5f);
                ImVec2 tMax(trailCenterX + it->width * 0.5f, it->y + it->height * 0.5f);

                int fillAlpha = static_cast<int>(it->alpha * 90.0f);
                int borderAlpha = static_cast<int>(it->alpha * 140.0f);

                drawList->AddRectFilled(tMin, tMax, IM_COL32(255, 255, 255, fillAlpha), 8.0f * scale);
                drawList->AddRect(tMin, tMax, IM_COL32(255, 255, 255, borderAlpha), 0, 1.0f);

                ++it;
            }
        }

        ImVec2 liquidMin(sidebarPos.x + 10.0f * scale, liquidCurrentY);
        ImVec2 liquidMax(liquidMin.x + optItemW, liquidMin.y + optItemH);

        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 selectedOptMin(sidebarPos.x + 10.0f * scale, optPositions[currentTab].y - optItemH * 0.5f);
        ImVec2 selectedOptMax(selectedOptMin.x + optItemW, selectedOptMin.y + optItemH);

        bool isHoveringLiquid = (mousePos.x >= selectedOptMin.x && mousePos.x <= selectedOptMax.x &&
            mousePos.y >= selectedOptMin.y && mousePos.y <= selectedOptMax.y);

        float targetWeight = isHoveringLiquid ? 2.0f : 0.0f;
        fluidWeight = CustomLerp(fluidWeight, targetWeight, io.DeltaTime * 4.0f);

        const int numSegments = 64;
        ImVec2 wavePoints[64];
        float time = static_cast<float>(ImGui::GetTime()) * 2.8f;

        ImVec2 center(liquidMin.x + optItemW * 0.5f, liquidMin.y + optItemH * 0.5f);
        float rx = optItemW * 0.49f;
        float ry = optItemH * 0.49f;

        float stretchY = 1.0f + liquidStretch * 0.45f;
        float stretchX = 1.0f - liquidStretch * 0.20f;

        for (int i = 0; i < numSegments; ++i) {
            float a = (static_cast<float>(i) / static_cast<float>(numSegments)) * 2.0f * 3.14159265f;

            float wave1 = std::sin(a * 2.0f + time) * 2.0f;
            float wave2 = std::cos(a * 3.0f - time * 0.8f) * 1.2f;

            float sinA = std::sin(a);
            float cosA = std::cos(a);

            float moveDir = (liquidVelY >= 0.0f) ? 1.0f : -1.0f;

            float dropletAsymmetry = (sinA * moveDir < 0.0f) ? (1.0f - std::abs(sinA) * 0.38f * liquidStretch)
                : (1.0f + std::abs(sinA) * 0.18f * liquidStretch);

            float organicOffset = (wave1 + wave2) * fluidWeight;

            float power = CustomLerp(8.5f, 6.2f, fluidWeight);
            float pX = std::pow(std::abs(cosA), 2.0f / power) * (cosA >= 0 ? 1.0f : -1.0f);
            float pY = std::pow(std::abs(sinA), 2.0f / power) * (sinA >= 0 ? 1.0f : -1.0f);

            wavePoints[i] = ImVec2(
                center.x + (pX * rx * stretchX * 0.98f) + cosA * organicOffset,
                center.y + (pY * ry * stretchY * dropletAsymmetry * 0.85f) + sinA * organicOffset
            );
        }

        float motionAlphaExtra = (std::min)(liquidStretch * 90.0f, 50.0f);
        int bgAlpha = static_cast<int>(CustomLerp(35.0f, 55.0f, fluidWeight) + motionAlphaExtra);
        int borderAlpha = static_cast<int>(CustomLerp(90.0f, 220.0f, fluidWeight) + motionAlphaExtra);

        drawList->AddConvexPolyFilled(wavePoints, numSegments, IM_COL32(255, 255, 255, bgAlpha));
        drawList->AddPolyline(wavePoints, numSegments, IM_COL32(255, 255, 255, borderAlpha), ImDrawFlags_Closed, 1.5f);

        drawList->AddRectFilled(
            ImVec2(liquidMin.x + 6.0f, liquidMin.y + 8.0f),
            ImVec2(liquidMin.x + 11.0f, liquidMax.y - 8.0f),
            IM_COL32(0, 122, 255, 230), 2.0f
        );

        // 面板区域
        float mainX = sidebarPos.x + sidebarW + 16.0f * scale;
        float mainW = windowSize.x - mainX - 16.0f * scale;

        ImGui::SetCursorPos(ImVec2(mainX, contentStartY));

        if (ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true)) {
            if (currentTab == 0) {
                ImGui::Text("右侧内容面板");
                ImGui::Separator();
                ImGui::Text("欢迎使用全新的液态玻璃界面系统。");
            }
            else if (currentTab == 1) {
                ImGui::Text("分辨率修复设置模块");
                ImGui::Separator();
            }
            else if (currentTab == 2) {
                ImGui::Text("配置设置");
                ImGui::Separator();
            }
        }

        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

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

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rect; GetClientRect(hWnd, &rect);
        const int bw = 8;
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
    case WM_NCCALCSIZE: if (wParam == TRUE) return 0; return 0;
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