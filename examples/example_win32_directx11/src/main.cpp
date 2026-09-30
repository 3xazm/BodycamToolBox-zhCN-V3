#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <gdiplus.h> // 使用 GDI+ 原生解码内存中的图片数据

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "curseFiles\Win32_API\Win32_API.h"
#include "curseFiles\Direct3D_Resource\Direct3D_Resource.h"
#include "curseFiles\AppRenderer\AppRenderer.h"

#include "Assets\BodycamAppIcon\BodycamAppIcon.h"

// 静态链接库定义
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib") // 链接 GDI+ 库以使用图像解码功能
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup") // 隐藏控制台黑框，入口点设为 main

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 渲染锁，防止多线程/消息回调重入渲染
static bool g_IsRendering = false;

// 安全渲染一帧，防止重入
void SafeRenderFrame( ) {
	if ( g_IsRendering ) return;
	g_IsRendering = true;
	RenderFrame( );
	g_IsRendering = false;
}

// 使用 GDI+ 从内存中的 PNG/JPEG 字节流无损安全创建 Windows 原生图标 (HICON)
HICON CreateHIconFromMemory( ) {
	if ( !BodycamAppIconData || BodycamAppIconDataSize == 0 ) return nullptr;

	// 1. 分配移动内存并拷贝二进制图标数据
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, BodycamAppIconDataSize);
	if ( !hMem ) return nullptr;

	void* pMem = GlobalLock(hMem);
	if ( !pMem ) {
		GlobalFree(hMem);
		return nullptr;
	}
	memcpy(pMem, BodycamAppIconData, BodycamAppIconDataSize);
	GlobalUnlock(hMem);

	// 2. 基于内存创建 IStream 流
	IStream* pStream = nullptr;
	if ( FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream)) ) {
		return nullptr;
	}

	// 3. 通过 GDI+ 加载图像并转换句柄
	HICON hIcon = nullptr;
	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
	if ( pBitmap && pBitmap->GetLastStatus( ) == Gdiplus::Ok ) {
		pBitmap->GetHICON(&hIcon);
		delete pBitmap;
	}

	pStream->Release( ); // 释放内存流（会自动释放 hMem）
	return hIcon;
}

int main(int, char**) {
	// 1. 程序启动时初始化 GDI+ 环境
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken = 0;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	// 2. 获取主显示器 DPI 缩放因子
	ImGui_ImplWin32_EnableDpiAwareness( );
	float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	// 3. 从内存创建应用图标
	HICON hIcon = CreateHIconFromMemory( );

	// 4. 注册 Win32 窗口类
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	WNDCLASSEXW wc = {
		sizeof(wc),
		CS_CLASSDC,
		WndProc,
		0L, 0L,
		hInstance,
		hIcon,                    // 任务栏及 Alt+Tab 大图标
		nullptr,
		nullptr,
		nullptr,
		L"LiquidGlassApp",
		hIcon                     // 标题栏小图标
	};
	::RegisterClassExW(&wc);

	// 5. 创建主窗口（使用基础样式，后续接管无边框拖拽）
	HWND hwnd = ::CreateWindowW(
		wc.lpszClassName, L"Bodycam工具箱V3",
		WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		150, 150, ( int ) (1000 * scale), ( int ) (620 * scale),
		nullptr, nullptr, wc.hInstance, nullptr
	);

	// 6. 配置 Win11/DWM 现代窗口特性
	// DWMWA_WINDOW_CORNER_PREFERENCE (33): 开启 Windows 11 圆角样式 (2 = DWMSWCP_ROUND)
	DWORD cornerPreference = 2;
	DwmSetWindowAttribute(hwnd, ( DWMWINDOWATTRIBUTE ) 33, &cornerPreference, sizeof(cornerPreference));

	// DWMWA_USE_IMMERSIVE_DARK_MODE (19): 开启暗色主题标题栏/边框
	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));

	// 开启 Win10/11 亚克力 / 亚克力亚态效果
	EnableAcrylic(hwnd, 0x00000000);

	// 7. 初始化 Direct3D11 渲染设备
	if ( !CreateDeviceD3D(hwnd) ) {
		CleanupDeviceD3D( );
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		Gdiplus::GdiplusShutdown(gdiplusToken);
		return 1;
	}

	// 8. 初始化 App 渲染管线与资源
	InitAppRenderer(hwnd, scale);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// 9. 初始化 ImGui 上下文与字体
	IMGUI_CHECKVERSION( );
	ImGui::CreateContext( );
	ImGuiIO& io = ImGui::GetIO( );

	ImFontConfig font_cfg;
	font_cfg.FontDataOwnedByAtlas = false;
	// 加载系统微软雅黑字体，并包含全量中文字符集
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * scale, &font_cfg, io.Fonts->GetGlyphRangesChineseFull( ));

	// 应用自定义 Glass 玻璃质感 UI 主题
	SetupAppleGlassTheme(scale);

	// 10. 初始化 ImGui Win32 & DX11 后端
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// 11. 主消息循环
	bool done = false;
	while ( !done ) {
		MSG msg;
		while ( ::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE) ) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if ( msg.message == WM_QUIT ) done = true;
		}
		if ( done ) break;

		// 处理交换链被遮挡（最小化）的情况
		if ( g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED ) {
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// 执行帧渲染
		SafeRenderFrame( );
	}

	// 12. 资源清理与注销
	if ( hIcon ) DestroyIcon(hIcon);

	ImGui_ImplDX11_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );

	ShutdownAppRenderer( );
	if ( g_pBlendState ) g_pBlendState->Release( );
	CleanupDeviceD3D( );

	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	// 退出前关闭 GDI+
	Gdiplus::GdiplusShutdown(gdiplusToken);

	return 0;
}

// 窗口消息处理回调函数
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// 优先交由 ImGui Win32 后端处理鼠标键盘消息
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if ( ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) ) return true;

	switch ( msg ) {
		// 拖动或调整窗口大小时，设置定时器触发实时重绘，消除拉伸黑屏/卡顿
	case WM_ENTERSIZEMOVE:
		SetTimer(hWnd, 1, 1, nullptr);
		break;
	case WM_EXITSIZEMOVE:
		KillTimer(hWnd, 1);
		break;
	case WM_MOVING:
	case WM_SIZING:
	case WM_TIMER:
		if ( msg != WM_TIMER || wParam == 1 ) {
			SafeRenderFrame( );
		}
		break;

		// 自定义无边框窗口 Hit-Test (碰撞检测)：处理窗口四周改变大小与顶部拖拽
	case WM_NCHITTEST: {
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		ScreenToClient(hWnd, &pt);
		RECT rect; GetClientRect(hWnd, &rect);

		const int bw = static_cast< int >(8.0f * g_Scale); // 判定调整大小的边框宽度

		// 判定 8 个方向调整窗口尺寸的区域
		if ( pt.y < bw && pt.x < bw ) return HTTOPLEFT;
		if ( pt.y < bw && pt.x > rect.right - bw ) return HTTOPRIGHT;
		if ( pt.y > rect.bottom - bw && pt.x < bw ) return HTBOTTOMLEFT;
		if ( pt.y > rect.bottom - bw && pt.x > rect.right - bw ) return HTBOTTOMRIGHT;
		if ( pt.x < bw ) return HTLEFT;
		if ( pt.x > rect.right - bw ) return HTRIGHT;
		if ( pt.y < bw ) return HTTOP;
		if ( pt.y > rect.bottom - bw ) return HTBOTTOM;

		// 判定顶部 Header 拖拽标题栏区域 (避开右侧按钮预留区)
		float headerH = 42.0f * g_Scale;
		float rightReservedW = 120.0f * g_Scale;

		if ( pt.y >= bw && pt.y < headerH && pt.x < rect.right - rightReservedW ) {
			return HTCAPTION;
		}

		return HTCLIENT;
	}
					 // 移除系统默认非客户区边框绘制
	case WM_NCCALCSIZE:
		if ( wParam == TRUE ) return 0;
		return 0;
		// 记录窗口尺寸变化，供后续 D3D 交换链 ResizeBuffers 逻辑使用
	case WM_SIZE:
		if ( wParam == SIZE_MINIMIZED ) return 0;
		g_ResizeWidth = ( UINT ) LOWORD(lParam);
		g_ResizeHeight = ( UINT ) HIWORD(lParam);
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}