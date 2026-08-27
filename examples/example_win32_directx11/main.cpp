// Dear ImGui: 应用程序主入口 (Windows API + DirectX 11)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <vector>
#include <cstdlib>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include "DashboardPage.h"
#include "ResolutionPage.h"

// ==========================================
// 1. D3D11 与 Shader 全局变量声明
// ==========================================
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                    g_SwapChainOccluded = false;
static UINT                    g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// 雨滴 Shader 独立渲染资源
ID3D11VertexShader* g_pRainVS = nullptr;
ID3D11PixelShader* g_pRainPS = nullptr;
ID3D11InputLayout* g_pRainInputLayout = nullptr;
ID3D11Buffer* g_pRainVB = nullptr;
ID3D11Buffer* g_pRainBuffer = nullptr;

// 雨滴离屏 Texture 与 RTV/SRV
ID3D11Texture2D* g_pRainTexture = nullptr;
ID3D11RenderTargetView* g_pRainRTV = nullptr;
ID3D11ShaderResourceView* g_pRainSRV = nullptr;
int                            g_RainTexWidth = 0, g_RainTexHeight = 0;

struct RainCB {
    float time;
    float resolution[2];
    float padding;
};

struct SimpleVertex {
    float x, y;
    float u, v;
};

// ==========================================
// 2. 雨滴 Shader 离屏资源初始化与释放
// ==========================================
void CleanupRainRenderTarget() {
    if (g_pRainSRV) { g_pRainSRV->Release(); g_pRainSRV = nullptr; }
    if (g_pRainRTV) { g_pRainRTV->Release(); g_pRainRTV = nullptr; }
    if (g_pRainTexture) { g_pRainTexture->Release(); g_pRainTexture = nullptr; }
}

void CreateRainRenderTarget(int width, int height) {
    CleanupRainRenderTarget();
    if (width <= 0 || height <= 0) return;

    g_RainTexWidth = width;
    g_RainTexHeight = height;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    g_pd3dDevice->CreateTexture2D(&texDesc, nullptr, &g_pRainTexture);
    g_pd3dDevice->CreateRenderTargetView(g_pRainTexture, nullptr, &g_pRainRTV);
    g_pd3dDevice->CreateShaderResourceView(g_pRainTexture, nullptr, &g_pRainSRV);
}

bool InitRainShader() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 编译 Vertex Shader
    HRESULT hr = D3DCompileFromFile(L"RainEffect.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pRainVS);

    // 编译 Pixel Shader
    hr = D3DCompileFromFile(L"RainEffect.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        vsBlob->Release();
        return false;
    }
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pRainPS);

    // Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pRainInputLayout);
    vsBlob->Release();
    psBlob->Release();

    // 绘制雨滴的屏顶点
    SimpleVertex vertices[] = {
        { -1.0f,  1.0f, 0.0f, 0.0f },
        {  1.0f,  1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f, 0.0f },
        {  1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 0.0f, 1.0f },
    };
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vInitData = { vertices, 0, 0 };
    g_pd3dDevice->CreateBuffer(&vbd, &vInitData, &g_pRainVB);

    // Constant Buffer
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(RainCB);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pd3dDevice->CreateBuffer(&desc, nullptr, &g_pRainBuffer);

    return true;
}

void RenderRainTexture(float width, float height) {
    if (width <= 0 || height <= 0 || !g_pRainPS) return;
    if ((int)width != g_RainTexWidth || (int)height != g_RainTexHeight) {
        CreateRainRenderTarget((int)width, (int)height);
    }

    // 透明清屏
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRainRTV, clearColor);
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRainRTV, nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_pd3dDeviceContext->RSSetViewports(1, &vp);

    static float g_time = 0.0f;
    g_time += 0.016f;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pRainBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
        RainCB* cb = (RainCB*)mappedResource.pData;
        cb->time = g_time;
        cb->resolution[0] = width;
        cb->resolution[1] = height;
        g_pd3dDeviceContext->Unmap(g_pRainBuffer, 0);
    }

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &g_pRainVB, &stride, &offset);
    g_pd3dDeviceContext->IASetInputLayout(g_pRainInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pd3dDeviceContext->VSSetShader(g_pRainVS, nullptr, 0);
    g_pd3dDeviceContext->PSSetShader(g_pRainPS, nullptr, 0);
    g_pd3dDeviceContext->PSSetConstantBuffers(0, 1, &g_pRainBuffer);

    g_pd3dDeviceContext->Draw(6, 0);
}
// ==========================================
// 3. Windows 亚克力 (Acrylic) 效果支持
// ==========================================
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

// ==========================================
// 4. 前置函数声明
// ==========================================
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==========================================
// 5. 程序主入口
// ==========================================
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
        (int)(1050 * main_scale), (int)(650 * main_scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // 4. DWM 属性设置 (亚克力背景效果)
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    BOOL USE_DARK_MODE = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &USE_DARK_MODE, sizeof(USE_DARK_MODE));
    //磨砂/模糊效果
    EnableAcrylicBlur(hwnd, 0x00000000);

    // 5. 初始化 D3D 设备与窗口
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    InitRainShader();

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

        // --- Step A: 渲染雨滴离屏纹理 ---
        RenderRainTexture(io.DisplaySize.x, io.DisplaySize.y);

        // --- Step B: ImGui UI 绘制 ---
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiStyle& current_style = ImGui::GetStyle();
        current_style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);  //设置窗口背景为半透明黑色 (0.05f, 0.05f, 0.05f, 0.20f);
		current_style.Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);    //设置子窗口背景为半透明白色 (1.00f, 1.00f, 1.00f, 0.05f); 

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("MainWindow", nullptr, window_flags);

        // 把渲染好的雨滴离屏纹理贴在窗口背景底层
        if (g_pRainSRV) {
            ImDrawList* bg_draw = ImGui::GetWindowDrawList();
            bg_draw->AddImage((ImTextureID)g_pRainSRV, ImVec2(0, 0), io.DisplaySize);
        }

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

        // 1. 静态/全局实例化页面对象
        static ResolutionPage g_ResolutionPage;

        // 2. 将默认选中的页面设为 1 (首页)
        static int g_SelectedTab = 1;

        // 主分栏布局
        if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 200.0f * main_scale);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // ==========================================
            // 左侧侧边栏
            // ==========================================
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("BODYCAM 工具箱");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

            // 菜单 1：首页 (ID 设为 1)
            if (ImGui::Selectable("首页", g_SelectedTab == 1, 0, ImVec2(0, 38.0f * main_scale))) {
                g_SelectedTab = 1;
            }

            // 菜单 2：分辨率与其他 (ID 设为 2)
            if (ImGui::Selectable("分辨率与其他", g_SelectedTab == 2, 0, ImVec2(0, 38.0f * main_scale))) {
                g_SelectedTab = 2;
            }

            /* 预留位置：
            // 菜单 0：设置 (后续添加)
            if (ImGui::Selectable("设置", g_SelectedTab == 0, 0, ImVec2(0, 38.0f * main_scale))) {
                g_SelectedTab = 0;
            }
            */

            ImGui::PopStyleVar();

            // ==========================================
            // 右侧主展示区
            // ==========================================
            ImGui::TableSetColumnIndex(1);

            // 根据选中的 Tab 切换渲染内容
            switch (g_SelectedTab) {
            case 0:
                // RenderSettingsPage(); // 留给以后的“设置”页面
                break;
            case 1:
                RenderDashboardPage(hwnd, main_scale); // 首页内容
                break;
            case 2:
                g_ResolutionPage.Render();            // 分辨率修复页面
                break;
            default:
                break;
            }

            ImGui::EndTable();
        }

        ImGui::End();

        // --- Step C: 渲染主 FrameBuffer 并提交输出 ---
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // 8. 退出与资源释放
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupRainRenderTarget();
    if (g_pRainVS) { g_pRainVS->Release(); g_pRainVS = nullptr; }
    if (g_pRainPS) { g_pRainPS->Release(); g_pRainPS = nullptr; }
    if (g_pRainInputLayout) { g_pRainInputLayout->Release(); g_pRainInputLayout = nullptr; }
    if (g_pRainVB) { g_pRainVB->Release(); g_pRainVB = nullptr; }
    if (g_pRainBuffer) { g_pRainBuffer->Release(); g_pRainBuffer = nullptr; }

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// ==========================================
// 6. Direct3D 11 辅助函数实现
// ==========================================
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
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// ==========================================
// 7. Windows 消息回调过程 (WndProc)
// ==========================================
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