#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <cmath>
#include <algorithm>

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

// 按钮图标类型枚举
enum MacBtnType {
    MAC_BTN_CLOSE,    // X
    MAC_BTN_MAXIMIZE, // 口 / 双框
    MAC_BTN_MINIMIZE  // -
};

// ==================== 2. 带液态玻璃感与鼠标跟踪高光的控制按钮 ====================
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

    // 平滑动画插值
    anim = CustomLerp(anim, hovered ? 1.0f : 0.0f, dt * 12.0f);
    storage->SetFloat(id, anim);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 mousePos = ImGui::GetMousePos();

    // 基础颜色设定 (默认带有液态玻璃的低饱和透明感)
    ImU32 baseThemeColor;
    if (type == MAC_BTN_CLOSE)      baseThemeColor = IM_COL32(255, 75, 75, 255);   // 红
    else if (type == MAC_BTN_MAXIMIZE) baseThemeColor = IM_COL32(255, 185, 45, 255);  // 黄
    else                            baseThemeColor = IM_COL32(50, 205, 80, 255);   // 绿

    // 泡泡平滑微放缩
    float currentRadius = radius + (radius * 0.26f * anim);

    // 1. 液态玻璃基础底色（默认半透明，悬停时被激活发光）
    int bgAlpha = static_cast<int>(85.0f + 160.0f * anim);
    ImU32 glassBgCol = (baseThemeColor & 0x00FFFFFF) | (static_cast<unsigned int>(bgAlpha) << 24);
    drawList->AddCircleFilled(pos, currentRadius, glassBgCol);

    // 2. 玻璃外边框（顶部亮白高光反光，底部微暗）
    drawList->AddCircle(pos, currentRadius, IM_COL32(255, 255, 255, static_cast<int>(80.0f + 100.0f * anim)), 0, 1.2f);

    // 3. 鼠标手电筒/聚光灯自动跟踪 (Spotlight Follow Effect)
    if (anim > 0.01f) {
        // 计算鼠标相对于按钮中心的向量
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

        // 手电筒聚光灯核心光源
        drawList->AddCircleFilled(spotCenter, currentRadius * 0.55f, IM_COL32(255, 255, 255, static_cast<int>(165.0f * anim)));

        // 柔和弥散发光圈
        for (int i = 1; i <= 3; i++) {
            float alpha = 32.0f * anim / static_cast<float>(i);
            ImU32 glowCol = (baseThemeColor & 0x00FFFFFF) | (static_cast<unsigned int>(alpha) << 24);
            drawList->AddCircleFilled(spotCenter, currentRadius + (static_cast<float>(i) * 3.0f * anim), glowCol);
        }
    }

    // 4. 纯矢量 Geometry 图标绘制
    ImU32 iconColor = IM_COL32(255, 255, 255, static_cast<int>(160 + 95 * anim)); // 默认半透白，悬停高亮纯白
    float iconScale = currentRadius * 0.42f;

    if (type == MAC_BTN_MINIMIZE) {
        // - 最小化 (横线)
        drawList->AddLine(
            ImVec2(pos.x - iconScale, pos.y),
            ImVec2(pos.x + iconScale, pos.y),
            iconColor, 1.8f
        );
    }
    else if (type == MAC_BTN_MAXIMIZE) {
        // 口 / 双框 最大化
        if (isMaximized) {
            // 后框
            drawList->AddRect(
                ImVec2(pos.x - iconScale * 0.3f, pos.y - iconScale * 0.9f),
                ImVec2(pos.x + iconScale * 0.9f, pos.y + iconScale * 0.3f),
                iconColor, 0.0f, 0, 1.2f
            );
            // 前框
            drawList->AddRect(
                ImVec2(pos.x - iconScale * 0.9f, pos.y - iconScale * 0.3f),
                ImVec2(pos.x + iconScale * 0.3f, pos.y + iconScale * 0.9f),
                iconColor, 0.0f, 0, 1.2f
            );
        }
        else {
            // 单框
            drawList->AddRect(
                ImVec2(pos.x - iconScale * 0.8f, pos.y - iconScale * 0.8f),
                ImVec2(pos.x + iconScale * 0.8f, pos.y + iconScale * 0.8f),
                iconColor, 0.0f, 0, 1.5f
            );
        }
    }
    else if (type == MAC_BTN_CLOSE) {
        // X 关闭
        drawList->AddLine(
            ImVec2(pos.x - iconScale * 0.75f, pos.y - iconScale * 0.75f),
            ImVec2(pos.x + iconScale * 0.75f, pos.y + iconScale * 0.75f),
            iconColor, 1.8f
        );
        drawList->AddLine(
            ImVec2(pos.x + iconScale * 0.75f, pos.y - iconScale * 0.75f),
            ImVec2(pos.x - iconScale * 0.75f, pos.y + iconScale * 0.75f),
            iconColor, 1.8f
        );
    }

    return pressed;
}

// ==================== 3. 带柔和发光与微膨胀效果的 Sidebar 选项 ====================
bool DrawSidebarOption(const char* label, bool selected, const ImVec2& size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiID id = window->GetID(label);
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGuiStorage* storage = window->DC.StateStorage;
    float anim = storage->GetFloat(id, 0.0f);
    float dt = ImGui::GetIO().DeltaTime;

    // 平滑动画插值
    anim = CustomLerp(anim, hovered ? 1.0f : 0.0f, dt * 11.0f);
    storage->SetFloat(id, anim);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float rounding = size.y * 0.4f;

    // 泡泡平滑膨胀
    float expand = anim * 8.0f;
    ImVec2 minPos(pos.x - expand, pos.y - expand);
    ImVec2 maxPos(pos.x + size.x + expand, pos.y + size.y + expand);

    // 外发光
    if (anim > 0.01f && !selected) {
        drawList->AddRectFilled(
            ImVec2(minPos.x - 3.0f * anim, minPos.y - 3.0f * anim),
            ImVec2(maxPos.x + 3.0f * anim, maxPos.y + 3.0f * anim),
            IM_COL32(255, 255, 255, static_cast<int>(12.0f * anim)),
            rounding + 3.0f
        );
    }

    // 背景色
    ImU32 bgCol = selected ? IM_COL32(255, 255, 255, 46) : IM_COL32(255, 255, 255, 12 + static_cast<int>(55.0f * anim));

    drawList->AddRectFilled(minPos, maxPos, bgCol, rounding);
    drawList->AddRect(
        minPos, maxPos,
        IM_COL32(255, 255, 255, selected ? 150 : static_cast<int>(60.0f + 60.0f * anim)),
        rounding, 0, 1.0f
    );

    // 选中提示条
    if (selected) {
        drawList->AddRectFilled(
            ImVec2(minPos.x + 8.0f, minPos.y + 8.0f),
            ImVec2(minPos.x + 13.0f, maxPos.y - 8.0f),
            IM_COL32(0, 122, 255, 225), 1.0f
        );
    }

    ImU32 textCol = selected ? IM_COL32(20, 20, 20, 255) : IM_COL32(240, 240, 240, 255);
    ImVec2 textPos = ImVec2(
        minPos.x + (selected ? 24.0f : 16.0f) + (anim * 2.0f),
        minPos.y + (size.y + expand * 2.0f - ImGui::GetTextLineHeight()) * 0.5f
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

        // 右上角玻璃控制灯（从右向左：红 X -> 黄 口 -> 绿 -）
        float circleR = 13.0f * scale;   // <-- 修改这里的 11.0f 即可改变球体半径（比如调小成 8.0f，或调大成 14.0f）
        float btnY = 24.0f * scale;
        float rightBaseX = windowSize.x - 24.0f * scale;
        float btnSpacing = 30.0f * scale; // 如果调大了 circleR，可以适当增大按钮间距

        bool isMaximized = ::IsZoomed(hwnd);

        // 1. 最右侧：红色关闭 (X)
        if (DrawMacCircleButton("CloseBtn", ImVec2(rightBaseX, btnY), circleR, MAC_BTN_CLOSE)) {
            ::PostQuitMessage(0);
        }
        // 2. 中间：黄色最大化/还原 (口)
        if (DrawMacCircleButton("MaxBtn", ImVec2(rightBaseX - btnSpacing, btnY), circleR, MAC_BTN_MAXIMIZE, isMaximized)) {
            if (isMaximized) ::SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            else ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
        // 3. 最左侧：绿色最小化 (-)
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

        // Sidebar 顶部选项列表
        float optItemW = sidebarW - 20.0f * scale;
        float optItemH = 38.0f * scale;

        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale));
        if (DrawSidebarOption("首页", currentTab == 0, ImVec2(optItemW, optItemH))) {
            currentTab = 0;
        }

        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + optItemH + 8.0f * scale));
        if (DrawSidebarOption("分辨率修复", currentTab == 1, ImVec2(optItemW, optItemH))) {
            currentTab = 1;
        }

        // Sidebar 底部固定：设置 (Settings)
        ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + contentH - optItemH - 12.0f * scale));
        if (DrawSidebarOption("设置", currentTab == 2, ImVec2(optItemW, optItemH))) {
            currentTab = 2;
        }

        // --- 右侧主内容面板 ---
        float mainX = sidebarPos.x + sidebarW + 16.0f * scale;
        float mainW = windowSize.x - mainX - 16.0f * scale;

        ImGui::SetCursorPos(ImVec2(mainX, contentStartY));
        ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true);

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