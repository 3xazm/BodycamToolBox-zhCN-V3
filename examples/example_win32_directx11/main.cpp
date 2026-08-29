#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <cmath>
#include <algorithm>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "RainEffectPipeline.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

// 全局 Direct3D 资源
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11BlendState* g_pBlendState = nullptr;

static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static bool                     g_SwapChainOccluded = false;

static RainEffectPipeline       g_RainPipeline;

// Win32 API 模糊透明支撑
typedef enum _WINDOWCOMPOSITIONATTRIB { WCA_ACCENT_POLICY = 19 } WINDOWCOMPOSITIONATTRIB;
typedef struct _ACCENT_POLICY { int AccentState; int AccentFlags; int GradientColor; int AnimationId; } ACCENT_POLICY;
typedef struct _WINDOWCOMPOSITIONATTRIB_DATA { WINDOWCOMPOSITIONATTRIB Attribute; PVOID pvData; SIZE_T cbData; } WINDOWCOMPOSITIONATTRIB_DATA;
typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIB_DATA*);

void EnableAcrylic(HWND hwnd, COLORREF colorWithAlpha) {
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

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
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

// ==================== 1. 动效辅助插值函数 ====================
static inline float CustomLerp(float a, float b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

enum MacBtnType {
    MAC_BTN_CLOSE,
    MAC_BTN_MAXIMIZE,
    MAC_BTN_MINIMIZE
};

// ==================== 2. 带液态玻璃感与控制按钮 ====================
bool DrawMacCircleButton(const char* id_str, const ImVec2& pos, float radius, MacBtnType type, bool isMaximized = false) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiID id = window->GetID(id_str);
    ImRect bb(ImVec2(pos.x - radius, pos.y - radius), ImVec2(pos.x + radius, pos.y + radius));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGuiStorage* storage = window->DC.StateStorage;
    float anim = storage->GetFloat(id, 0.0f);
    float dt = ImGui::GetIO().DeltaTime;

    anim = CustomLerp(anim, hovered ? 1.0f : 0.0f, dt * 12.0f);
    storage->SetFloat(id, anim);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 mousePos = ImGui::GetMousePos();

    ImU32 baseThemeColor;
    if (type == MAC_BTN_CLOSE)      baseThemeColor = IM_COL32(255, 75, 75, 255);
    else if (type == MAC_BTN_MAXIMIZE) baseThemeColor = IM_COL32(255, 185, 45, 255);
    else                            baseThemeColor = IM_COL32(50, 205, 80, 255);

    float currentRadius = radius + (radius * 0.26f * anim);

    int bgAlpha = static_cast<int>(85.0f + 160.0f * anim);
    ImU32 glassBgCol = (baseThemeColor & 0x00FFFFFF) | (static_cast<unsigned int>(bgAlpha) << 24);
    drawList->AddCircleFilled(pos, currentRadius, glassBgCol);
    drawList->AddCircle(pos, currentRadius, IM_COL32(255, 255, 255, static_cast<int>(80.0f + 100.0f * anim)), 0, 1.2f);

    if (anim > 0.01f) {
        ImVec2 delta(mousePos.x - pos.x, mousePos.y - pos.y);
        float distSq = delta.x * delta.x + delta.y * delta.y;
        float maxOffset = currentRadius * 0.45f;

        ImVec2 lightOffset(0, 0);
        if (distSq > 0.0001f) {
            float dist = std::sqrt(distSq);
            float clampDist = (std::min)(dist, maxOffset);
            lightOffset = ImVec2((delta.x / dist) * clampDist, (delta.y / dist) * clampDist);
        }

        ImVec2 spotCenter(pos.x + lightOffset.x, pos.y + lightOffset.y);
        drawList->AddCircleFilled(spotCenter, currentRadius * 0.55f, IM_COL32(255, 255, 255, static_cast<int>(165.0f * anim)));

        for (int i = 1; i <= 3; i++) {
            float alpha = 32.0f * anim / static_cast<float>(i);
            ImU32 glowCol = (baseThemeColor & 0x00FFFFFF) | (static_cast<unsigned int>(alpha) << 24);
            drawList->AddCircleFilled(spotCenter, currentRadius + (static_cast<float>(i) * 3.0f * anim), glowCol);
        }
    }

    ImU32 iconColor = IM_COL32(255, 255, 255, static_cast<int>(160 + 95 * anim));
    float iconScale = currentRadius * 0.42f;

    if (type == MAC_BTN_MINIMIZE) {
        drawList->AddLine(ImVec2(pos.x - iconScale, pos.y), ImVec2(pos.x + iconScale, pos.y), iconColor, 1.8f);
    }
    else if (type == MAC_BTN_MAXIMIZE) {
        if (isMaximized) {
            drawList->AddRect(ImVec2(pos.x - iconScale * 0.3f, pos.y - iconScale * 0.9f), ImVec2(pos.x + iconScale * 0.9f, pos.y + iconScale * 0.3f), iconColor, 0.0f, 0, 1.2f);
            drawList->AddRect(ImVec2(pos.x - iconScale * 0.9f, pos.y - iconScale * 0.3f), ImVec2(pos.x + iconScale * 0.3f, pos.y + iconScale * 0.9f), iconColor, 0.0f, 0, 1.2f);
        }
        else {
            drawList->AddRect(ImVec2(pos.x - iconScale * 0.8f, pos.y - iconScale * 0.8f), ImVec2(pos.x + iconScale * 0.8f, pos.y + iconScale * 0.8f), iconColor, 0.0f, 0, 1.5f);
        }
    }
    else if (type == MAC_BTN_CLOSE) {
        drawList->AddLine(ImVec2(pos.x - iconScale * 0.75f, pos.y - iconScale * 0.75f), ImVec2(pos.x + iconScale * 0.75f, pos.y + iconScale * 0.75f), iconColor, 1.8f);
        drawList->AddLine(ImVec2(pos.x + iconScale * 0.75f, pos.y - iconScale * 0.75f), ImVec2(pos.x - iconScale * 0.75f, pos.y + iconScale * 0.75f), iconColor, 1.8f);
    }

    return pressed;
}

// ==================== 3. 无方框轻量 Sidebar 选项 (完美稳定版) ====================
bool DrawSidebarOption(const char* label, bool selected, const ImVec2& size, ImVec2* outCenterPos = nullptr) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    // 1. 使用 InvisibleButton 作为底层标准交互控件（解决 ID 冲突与布局游标异常）
    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();

    // 2. 安全获取与平滑更新动画帧状态 (anim)
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID id = window->GetID(label);
    float anim = storage->GetFloat(id, 0.0f);
    float dt = ImGui::GetIO().DeltaTime;

    anim = CustomLerp(anim, hovered ? 6.0f : 0.0f, dt * 12.0f); //selct text
    storage->SetFloat(id, anim);

    // 3. 计算选项中心点供液态胶囊吸收定位
    if (outCenterPos) {
        *outCenterPos = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    }

    // 4. 安全绘制文字与平滑过渡
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 textCol = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 205, 215, static_cast<int>(180.0f + 75.0f * anim));

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos(
        pos.x + 24.0f + (selected ? 4.0f : (anim * 3.0f)),
        pos.y + (size.y - textSize.y) * 0.5f
    );

    drawList->AddText(textPos, textCol, label);

    return pressed;
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

    DWORD cornerPreference = 2; // DWMWCP_ROUND
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

    // 全局液态胶囊滑动位置静态变量
    static float liquidCurrentY = -1.0f;
    static bool isFirstFrameLiquid = true;
    static float fluidWeight = 6.0f; // <--- 新增：用于平滑过渡流体波浪强度
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

        // 窗口轮廓线
        drawList->AddRect(
            ImVec2(0, 0), windowSize,
            IM_COL32(255, 255, 255, 80), 16.0f * scale, 0, 1.5f * scale
        );

        // ==================== 1. 顶部 Header 区域 ====================
        float headerH = 42.0f * scale;

        // 左侧标题胶囊块
        ImVec2 titleCapsuleSize(200.0f * scale, 34.0f * scale);
        ImVec2 titlePos(16.0f * scale, 12.0f * scale);
        drawList->AddRectFilled(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 25), 17.0f * scale);
        drawList->AddRect(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 80), 17.0f * scale);

        ImVec2 titleTextSize = ImGui::CalcTextSize("Bodycam工具箱V3");
        drawList->AddText(ImVec2(titlePos.x + (titleCapsuleSize.x - titleTextSize.x) * 0.5f, titlePos.y + (titleCapsuleSize.y - titleTextSize.y) * 0.5f), IM_COL32(255, 255, 255, 230), "Bodycam工具箱V3");

        // 中间搜索栏
        float searchW = 300.0f * scale;
        float searchX = (windowSize.x - searchW) * 0.5f;
        ImGui::SetCursorPos(ImVec2(searchX, 12.0f * scale));
        ImGui::PushItemWidth(searchW);
        ImGui::InputTextWithHint("##Search", "搜索...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        ImGui::PopItemWidth();

        // 拖拽窗口暗区域
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##TitleDrag", ImVec2(windowSize.x - 120.0f * scale, headerH));
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ::ReleaseCapture();
            ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        // 右上角控制灯
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

        // ==================== 2. 主体分栏布局 ====================
        float contentStartY = headerH + 16.0f * scale;
        float contentH = windowSize.y - contentStartY - 16.0f * scale;
        float sidebarW = 200.0f * scale;

        // --- 左侧 Sidebar 容器板 ---
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

        // ==================== 1. 水滴玻璃附着与水痕蒸发系统 ====================
        struct TrailSegment {
            float y;          // 水痕中心位置 Y
            float alpha;      // 当前透明度 (1.0 -> 0.0 蒸发)
            float width;      // 水痕宽度
            float height;     // 水痕高度
        };
        static std::vector<TrailSegment> waterTrails; // 保存残留水痕的数组

        static float liquidVelY = 0.0f;          // 移动速度
        static float liquidStretch = 0.0f;       // 液滴上下拉伸感
        static float lastY = -1.0f;              // 上一帧位置

        float targetY = optPositions[currentTab].y - optItemH * 0.5f;

        if (isFirstFrameLiquid) {
            liquidCurrentY = targetY;
            lastY = targetY;
            isFirstFrameLiquid = false;
        }
        else {
            // 水滴滑动物理模型：非对称高粘滞力（模拟水在玻璃上的附着力）
            float dist = targetY - liquidCurrentY;

            float stiffness = (dist > 0.0f) ? 200.0f : 230.0f;
            float damping = (std::abs(dist) < 12.0f) ? 6.0f : 12.0f; // 靠近目标时骤增阻尼（强行粘在玻璃上）

            float force = dist * stiffness;
            liquidVelY += force * io.DeltaTime;
            liquidVelY -= liquidVelY * damping * io.DeltaTime;
            liquidCurrentY += liquidVelY * io.DeltaTime;

            // 根据速度计算形变
            float targetStretch = (std::abs(liquidVelY) / 500.0f);
            targetStretch = (std::min)(targetStretch, 0.5f);
            liquidStretch = CustomLerp(liquidStretch, targetStretch, io.DeltaTime * 18.0f);

            // 当水滴在玻璃上快速滑动时，沿途留下半透明水痕
            if (std::abs(liquidCurrentY - lastY) > 4.0f * scale) {
                TrailSegment seg;
                seg.y = (liquidCurrentY + lastY) * 0.5f + optItemH * 0.5f;
                seg.alpha = 0.45f; // 初始水痕透明度
                seg.width = (optItemW - 30.0f * scale) * (1.0f - liquidStretch * 0.2f);
                seg.height = std::abs(liquidCurrentY - lastY) + 6.0f * scale;
                waterTrails.push_back(seg);
                lastY = liquidCurrentY;
            }
        }

        // 更新并绘制所有残留水痕（模拟水痕在玻璃上慢慢蒸发干涸）
        for (auto it = waterTrails.begin(); it != waterTrails.end(); ) {
            // 水痕逐渐透明蒸发
            it->alpha -= io.DeltaTime * 1.2f;

            if (it->alpha <= 0.0f) {
                it = waterTrails.erase(it); // 彻底干涸蒸发
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

        // 2. 悬停判断：只检测鼠标是否悬停在【当前选中的选项】
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 selectedOptMin(sidebarPos.x + 10.0f * scale, optPositions[currentTab].y - optItemH * 0.5f);
        ImVec2 selectedOptMax(selectedOptMin.x + optItemW, selectedOptMin.y + optItemH);

        bool isHoveringLiquid = (mousePos.x >= selectedOptMin.x && mousePos.x <= selectedOptMax.x &&
            mousePos.y >= selectedOptMin.y && mousePos.y <= selectedOptMax.y);

        // 3. 计算流体波浪平滑权重
        float targetWeight = isHoveringLiquid ? 2.0f : 0.0f;
        fluidWeight = CustomLerp(fluidWeight, targetWeight, io.DeltaTime * 4.0f);

        // ==================== 4. 渲染主水滴 (Water Droplet Shape) ====================
        const int numSegments = 64;
        ImVec2 wavePoints[64];
        float time = static_cast<float>(ImGui::GetTime()) * 2.8f;

        ImVec2 center(liquidMin.x + optItemW * 0.5f, liquidMin.y + optItemH * 0.5f);
        float rx = optItemW * 0.49f;
        float ry = optItemH * 0.49f;

        // 动态计算水滴移动时的非对称拉伸（Y轴拉长，X轴变窄）
        float stretchY = 1.0f + liquidStretch * 0.45f;
        float stretchX = 1.0f - liquidStretch * 0.20f;

        for (int i = 0; i < numSegments; ++i) {
            float a = (static_cast<float>(i) / static_cast<float>(numSegments)) * 2.0f * 3.14159265f;

            float wave1 = std::sin(a * 2.0f + time) * 2.0f;
            float wave2 = std::cos(a * 3.0f - time * 0.8f) * 1.2f;

            float sinA = std::sin(a);
            float cosA = std::cos(a);

            // 运动方向判定：1.0 为下滑，-1.0 为上吸
            float moveDir = (liquidVelY >= 0.0f) ? 1.0f : -1.0f;

            // 下滑时上方水尾收尖，下方水头饱满
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

        // 滑动时水滴亮度提亮
        float motionAlphaExtra = (std::min)(liquidStretch * 90.0f, 50.0f);
        int bgAlpha = static_cast<int>(CustomLerp(35.0f, 55.0f, fluidWeight) + motionAlphaExtra);
        int borderAlpha = static_cast<int>(CustomLerp(90.0f, 220.0f, fluidWeight) + motionAlphaExtra);

        drawList->AddConvexPolyFilled(wavePoints, numSegments, IM_COL32(255, 255, 255, bgAlpha));
        drawList->AddPolyline(wavePoints, numSegments, IM_COL32(255, 255, 255, borderAlpha), ImDrawFlags_Closed, 1.5f);

        // 4. 蓝色指示条
        drawList->AddRectFilled(
            ImVec2(liquidMin.x + 6.0f, liquidMin.y + 8.0f),
            ImVec2(liquidMin.x + 11.0f, liquidMax.y - 8.0f),
            IM_COL32(0, 122, 255, 230), 2.0f
        );

        // --- 右侧主内容面板 ---
        float mainX = sidebarPos.x + sidebarW + 16.0f * scale;
        float mainW = windowSize.x - mainX - 16.0f * scale;

        ImGui::SetCursorPos(ImVec2(mainX, contentStartY));

        // 必须保证 BeginChild 与 EndChild 严格一对一调用
        if (ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true)) {
            if (currentTab == 0) {
                ImGui::Text("右侧内容面板");
                ImGui::Separator();
                ImGui::Text("欢迎使用全新的液态玻璃界面系统。");
            }
            else if (currentTab == 1) {
                ImGui::Text("分辨率修复设置模块");
                ImGui::Separator();
                // 如果此 Tab 内后续有加控件，确保没有未闭合的 Group/Combo/Tree
            }
            else if (currentTab == 2) {
                ImGui::Text("配置设置");
                ImGui::Separator();
            }
        }

        ImGui::EndChild(); // 无论是否 Skip 都会安全闭合

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

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
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
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext)))
        return false;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    g_pd3dDevice->CreateBlendState(&blendDesc, &g_pBlendState);

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
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