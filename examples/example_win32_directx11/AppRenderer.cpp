#include "AppRenderer.h"
#include <algorithm>

// ImGui 核心与后端
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// 引入 stb_image 解码库与图标十六进制数据
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "BodycamAppIcon.h" // 你的二进制图标数据文件

// 自定义模块
#include "RainEffectPipeline.h"
#include "Direct3D_Resource.h"
#include "UI_Header.h"
#include "UI_Sidebar.h"
#include "MainViews.h"

// 全局变量定义
HWND g_hWnd = nullptr;
float g_Scale = 1.0f;
int g_CurrentTab = 0;
char g_SearchBuffer[128] = "";
LiquidAnimationState g_LiquidState;

static RainEffectPipeline g_RainPipeline;
static ID3D11ShaderResourceView* g_pAppIconSRV = nullptr; // 存储图标纹理句柄

// 从内存 Hex 数组创建 D3D11 纹理视图
static void LoadAppIconTexture() {
    if (!g_pd3dDevice || g_pAppIconSRV) return;

    int width = 0, height = 0, channels = 0;
    // 使用 stb_image 将内存中的 PNG Hex 转换为 RGBA 像素数据
    unsigned char* pixels = stbi_load_from_memory(
        BodycamAppIconData,
        (int)BodycamAppIconDataSize,
        &width, &height, &channels, 4
    );
    if (!pixels) return;

    // 创建 D3D11 2D 纹理
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = pixels;
    subData.SysMemPitch = width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &subData, &pTexture))) {
        g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, &g_pAppIconSRV);
        pTexture->Release();
    }

    stbi_image_free(pixels); // 释放内存中的临时像素
}

void InitAppRenderer(HWND hwnd, float scale) {
    g_hWnd = hwnd;
    g_Scale = scale;
    g_RainPipeline.Init(g_pd3dDevice);

    LoadAppIconTexture(); // 初始化 Icon 纹理
}

void RenderFrame() {
    if (!g_pd3dDeviceContext || !g_mainRenderTargetView) return;

    ImGuiIO& io = ImGui::GetIO();

    if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        g_ResizeWidth = g_ResizeHeight = 0;
        CreateRenderTarget();
    }

    // 1. 渲染背景动态雨滴特效
    g_RainPipeline.Resize(g_pd3dDevice, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    g_RainPipeline.Render(g_pd3dDeviceContext, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f, io.DeltaTime);

    // 2. ImGui 帧初始化
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * g_Scale, 12 * g_Scale));
    ImGui::Begin("MainWindow", nullptr, flags);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (g_RainPipeline.pSRV) {
        drawList->AddImage((ImTextureID)g_RainPipeline.pSRV, ImVec2(0, 0), io.DisplaySize);
    }

    ImVec2 windowSize = ImGui::GetWindowSize();

    drawList->AddRect(
        ImVec2(0, 0), windowSize,
        IM_COL32(255, 255, 255, 80), 16.0f * g_Scale, 0, 1.5f * g_Scale
    );

    // --- Header ---
    float headerH = 42.0f * g_Scale;
    RenderHeader(
        g_hWnd,
        drawList,
        windowSize,
        g_Scale,
        headerH,
        g_SearchBuffer,
        IM_ARRAYSIZE(g_SearchBuffer),
        (ImTextureID)g_pAppIconSRV // 传入加载好的 Icon 纹理
    );

    // --- 布局计算 ---
    float contentStartY = headerH + 16.0f * g_Scale;
    float contentH = windowSize.y - contentStartY - 16.0f * g_Scale;
    float sidebarW = 200.0f * g_Scale;
    ImVec2 sidebarPos(16.0f * g_Scale, contentStartY);

    // --- Sidebar ---
    RenderSidebar(drawList, g_CurrentTab, g_LiquidState, sidebarPos, sidebarW, contentH, g_Scale, io.DeltaTime);

    // --- Main Content ---
    float mainX = sidebarPos.x + sidebarW + 16.0f * g_Scale;
    float mainW = windowSize.x - mainX - 16.0f * g_Scale;

    ImGui::SetCursorPos(ImVec2(mainX, contentStartY));

    if (ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true)) {
        RenderMainViews(g_CurrentTab, g_Scale);
    }

    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();

    // 3. D3D11 最终呈现
    ImGui::Render();
    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
}

void ShutdownAppRenderer() {
    if (g_pAppIconSRV) {
        g_pAppIconSRV->Release();
        g_pAppIconSRV = nullptr;
    }
    g_RainPipeline.Shutdown();
}