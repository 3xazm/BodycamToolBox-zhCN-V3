#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
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
            ACCENT_POLICY policy = { 4, 0, (int)colorWithAlpha, 0 }; // 4 = ACCENT_ENABLE_BLURBEHIND / ACRYLIC
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

// 初始化苹果 GUI 默认全套主题配置
void SetupAppleGlassTheme(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();

    // 圆角与边框配置
    style.WindowRounding = 16.0f * scale;
    style.ChildRounding = 12.0f * scale;
    style.FrameRounding = 8.0f * scale;
    style.PopupRounding = 10.0f * scale;
    style.ScrollbarRounding = 12.0f * scale;
    style.GrabRounding = 6.0f * scale;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;

    // 经典 Apple Liquid Transparent 配色方案
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.12f, 0.35f); // 深度半透明
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.18f); // 高亮边缘切线
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.15f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.32f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.64f, 0.70f, 1.00f);
}

int main(int, char**) {
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"LiquidGlassApp", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"Apple Liquid Glass ImGui",
        WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        150, 150, (int)(1100 * scale), (int)(700 * scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // 启用系统暗色调与原生无边框 Acrylic 混合
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
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f * scale, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    SetupAppleGlassTheme(scale);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static int currentTab = 0;
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

        // 传入真实的帧间隔时间以及鼠标 X/Y 坐标（驱动你的 m 参数）
        float mouseXNormalized = io.MousePos.x / io.DisplaySize.x;
        float mouseYNormalized = io.MousePos.y / io.DisplaySize.y;

        // 1. 动态生成/更新雨滴离屏纹理
        g_RainPipeline.Resize(g_pd3dDevice, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        g_RainPipeline.Render(g_pd3dDeviceContext, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f, io.DeltaTime);

        // 2. ImGui 逻辑绘制
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 绑定主全屏透明窗口
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("MainWindow", nullptr, flags);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // [A] 将雨滴纹理覆盖贴在最底层
        if (g_RainPipeline.pSRV) {
            drawList->AddImage((ImTextureID)g_RainPipeline.pSRV, ImVec2(0, 0), io.DisplaySize);
        }

        // [B] 绘制苹果“液态玻璃切边亮光” (Edge Highlight Ring)
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        drawList->AddRect(
            pos, ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(255, 255, 255, 75), 16.0f * scale, 0, 1.5f * scale
        );

        // [C] 顶部半透明柔光折射带 (Inner Soft Ambient)
        drawList->AddRectFilled(
            ImVec2(pos.x + 1, pos.y + 1),
            ImVec2(pos.x + size.x - 1, pos.y + 45.0f * scale),
            IM_COL32(255, 255, 255, 12), 15.0f * scale
        );

        // 自定义 Windows 标题栏
        float headerH = 36.0f * scale;
        float btnW = 46.0f * scale;

        ImGui::SetCursorPos(ImVec2(16 * scale, (headerH - 20 * scale) * 0.5f));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.9f), "BODYCAM TOOLKIT V3 — LIQUID GLASS");

        // 标题栏拖拽区域
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##TitleDrag", ImVec2(size.x - btnW * 3, headerH));
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ::ReleaseCapture();
            ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        // 最小化 / 关闭 按钮
        ImGui::SetCursorPos(ImVec2(size.x - btnW * 3, 0));
        if (ImGui::Button("-##Min", ImVec2(btnW, headerH))) ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ImGui::SameLine();
        if (ImGui::Button("[]##Max", ImVec2(btnW, headerH))) ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ImGui::SameLine();
        if (ImGui::Button("X##Close", ImVec2(btnW, headerH))) ::PostQuitMessage(0);

        ImGui::SetCursorPosY(headerH);
        ImGui::Separator();

        // 页面主体 2 栏布局
        if (ImGui::BeginTable("BodyLayout", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Side", ImGuiTableColumnFlags_WidthFixed, 220.0f * scale);
            ImGui::TableSetupColumn("Main", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // 左侧 Sidebar
            ImGui::TableSetColumnIndex(0);
            ImGui::Spacing();
            if (ImGui::Selectable("  控制台 Dashboard", currentTab == 0, 0, ImVec2(0, 40 * scale))) currentTab = 0;
            if (ImGui::Selectable("  分辨率修复 Resolution", currentTab == 1, 0, ImVec2(0, 40 * scale))) currentTab = 1;
            if (ImGui::Selectable("  系统设置 Settings", currentTab == 2, 0, ImVec2(0, 40 * scale))) currentTab = 2;

            // 右侧内容面板
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("ChildContent", ImVec2(0, 0), true);
            if (currentTab == 0) {
                ImGui::Text("欢迎使用全新重构的液态玻璃 UI 框架！");
                ImGui::Separator();
                static float val = 0.5f;
                ImGui::SliderFloat("玻璃透射感调整", &val, 0.0f, 1.0f);
                static char text[128] = "Liquid Glass Effect";
                ImGui::InputText("测试输入框", text, 128);
                ImGui::Button("应用效果 (Apply)", ImVec2(160 * scale, 36 * scale));
            }
            else if (currentTab == 1) {
                ImGui::Text("分辨率调整与修复模块已就绪。");
            }
            else {
                ImGui::Text("配置设置...");
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::End();

        // 3. DirectX 11 最终呈现渲染
        ImGui::Render();
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // 释放资源
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

// Direct3D 11 设备初始化与 Alpha Blend 支持
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

    // 关键：创建全局 Alpha 混合状态，解决黑色暗影/透明冲掉问题
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