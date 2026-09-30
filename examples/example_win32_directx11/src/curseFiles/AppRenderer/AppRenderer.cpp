#include "AppRenderer.h"
#include <algorithm>

// ImGui 核心与 Win32 / DX11 后端
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// stb_image 用于解码二进制图片数据
#define STB_IMAGE_IMPLEMENTATION
#include "..\..\src\Assets\STB_IMAGE_IMPLEMENTATION\stb_image.h"
#include "..\..\src\Assets\BodycamAppIcon\BodycamAppIcon.h" // 存放 HEX 格式的二进制图片数组

// UI 与特效模块
#include "..\..\Assets\RainEffect_HLSL\RainEffectPipeline.h"
#include "..\..\src\curseFiles\Direct3D_Resource\Direct3D_Resource.h"
#include "..\..\src\UI\Header\UI_Header.h"
#include "..\..\src\UI\Sidebar\UI_Sidebar.h"
#include "..\..\src\UI\MainViews\MainViews.h"

// 全局变量定义
HWND g_hWnd = nullptr;
float g_Scale = 1.0f;
int g_CurrentTab = 0;
char g_SearchBuffer[ 128 ] = "";
LiquidAnimationState g_LiquidState;

static RainEffectPipeline g_RainPipeline;            // 背景雨滴 HLSL 特效管线
static ID3D11ShaderResourceView* g_pAppIconSRV = nullptr; // 应用图标的 Direct3D11 纹理视图 (SRV)

// 从内存 Hex 数组创建 D3D11 2D 纹理视图
static void LoadAppIconTexture( ) {
	if ( !g_pd3dDevice || g_pAppIconSRV ) return;

	int width = 0, height = 0, channels = 0;
	// 使用 stb_image 将内存中的 PNG 二进制字节解压为 RGBA 像素数据
	unsigned char* pixels = stbi_load_from_memory(
		BodycamAppIconData,
		( int ) BodycamAppIconDataSize,
		&width, &height, &channels, 4
	);
	if ( !pixels ) return;

	// 配置 D3D11 2D 纹理描述
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
	if ( SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &subData, &pTexture)) ) {
		g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, &g_pAppIconSRV);
		pTexture->Release( ); // 创建 SRV 后即可释放原始 2D Texture 对象
	}

	stbi_image_free(pixels); // 释放 stb CPU 端的临时像素内存
}

void InitAppRenderer(HWND hwnd, float scale) {
	g_hWnd = hwnd;
	g_Scale = scale;
	g_RainPipeline.Init(g_pd3dDevice);

	LoadAppIconTexture( ); // 初始化 Icon 的 D3D11 纹理
}

void RenderFrame( ) {
	if ( !g_pd3dDeviceContext || !g_mainRenderTargetView ) return;

	ImGuiIO& io = ImGui::GetIO( );

	// 1. 响应窗口 Size 改变，重构 D3D 交换链缓冲与 RenderTargetView
	if ( g_ResizeWidth != 0 && g_ResizeHeight != 0 ) {
		CleanupRenderTarget( );
		g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
		g_ResizeWidth = g_ResizeHeight = 0;
		CreateRenderTarget( );
	}

	// 2. 渲染背景动态雨滴 HLSL 特效
	g_RainPipeline.Resize(g_pd3dDevice, ( int ) io.DisplaySize.x, ( int ) io.DisplaySize.y);
	g_RainPipeline.Render(g_pd3dDeviceContext, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f, io.DeltaTime);

	// 3. ImGui 帧逻辑初始化
	ImGui_ImplDX11_NewFrame( );
	ImGui_ImplWin32_NewFrame( );
	ImGui::NewFrame( );

	// 铺满整个 Win32 窗口
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	// 【Style栈 1】压入窗口外边距 StyleVar
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * g_Scale, 12 * g_Scale));
	ImGui::Begin("MainWindow", nullptr, flags);

	ImDrawList* drawList = ImGui::GetWindowDrawList( );

	// 绘制背景雨滴 Shader SRV 纹理到 Window 底层
	if ( g_RainPipeline.pSRV ) {
		drawList->AddImage(( ImTextureID ) g_RainPipeline.pSRV, ImVec2(0, 0), io.DisplaySize);
	}

	ImVec2 windowSize = ImGui::GetWindowSize( );

	// 绘制外层玻璃感半透明细边框
	drawList->AddRect(
		ImVec2(0, 0), windowSize,
		IM_COL32(255, 255, 255, 80), 16.0f * g_Scale, 0, 1.5f * g_Scale
	);

	// --- Header (顶部标题栏) ---
	float headerH = 42.0f * g_Scale;
	RenderHeader(
		g_hWnd,
		drawList,
		windowSize,
		g_Scale,
		headerH,
		g_SearchBuffer,
		IM_ARRAYSIZE(g_SearchBuffer),
		( ImTextureID ) g_pAppIconSRV
	);

	// --- 界面布局尺寸计算 ---
	float contentStartY = headerH + 16.0f * g_Scale;
	float contentH = windowSize.y - contentStartY - 16.0f * g_Scale;
	float sidebarW = 200.0f * g_Scale;
	ImVec2 sidebarPos(16.0f * g_Scale, contentStartY);

	// --- Sidebar (侧边导航栏) ---
	RenderSidebar(drawList, g_CurrentTab, g_LiquidState, sidebarPos, sidebarW, contentH, g_Scale, io.DeltaTime);

	// --- Main Content (主内容区域) ---
	float mainX = sidebarPos.x + sidebarW + 16.0f * g_Scale;
	float mainW = windowSize.x - mainX - 16.0f * g_Scale;

	// ================= Tab 切换时的平滑轻弹/淡入淡出动画逻辑 =================
	static int s_LastTab = g_CurrentTab;
	static float s_AnimProgress = 1.0f;

	if ( s_LastTab != g_CurrentTab ) {
		s_LastTab = g_CurrentTab;
		s_AnimProgress = 0.0f; // 切换 Tab 时重置进度
	}

	if ( s_AnimProgress < 1.0f ) {
		s_AnimProgress += io.DeltaTime * 4.5f;
		if ( s_AnimProgress > 1.0f ) s_AnimProgress = 1.0f;
	}

	// 三次方 Ease-Out 缓动算法
	float progress = ( std::min ) (1.0f, ( std::max ) (0.0f, s_AnimProgress));
	float invProgress = 1.0f - progress;
	float eased = 1.0f - (invProgress * invProgress * invProgress);

	float alpha = ( std::min ) (1.0f, ( std::max ) (0.0f, eased));
	float offsetY = (1.0f - eased) * (20.0f * g_Scale); // Y 轴微量偏移弹跳效果

	ImGui::SetCursorPos(ImVec2(mainX, contentStartY + offsetY));

	// 【Style栈 2】压入 Alpha 透明度 StyleVar 产生淡入效果
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

	if ( ImGui::BeginChild("MainContentPanel", ImVec2(mainW, contentH), true) ) {
		RenderMainViews(g_CurrentTab, g_Scale);
	}
	ImGui::EndChild( );

	// 【Style栈 2】弹出 Alpha StyleVar (必须严格对应 Push)
	ImGui::PopStyleVar(1);

	ImGui::End( ); // 结束 MainWindow

	// 【Style栈 1】弹出 WindowPadding StyleVar (必须严格对应 Push)
	ImGui::PopStyleVar(1);

	// 4. Direct3D11 最终画面呈现
	ImGui::Render( );
	const float clear_color[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };

	g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData( ));
	g_pSwapChain->Present(1, 0); // 垂直同步开启 (1)
}

void ShutdownAppRenderer( ) {
	if ( g_pAppIconSRV ) {
		g_pAppIconSRV->Release( );
		g_pAppIconSRV = nullptr;
	}
	g_RainPipeline.Shutdown( );
}