#pragma once
#include <d3d11.h>

// 全局 Direct3D 资源全局变量声明（extern 允许跨文件共享）
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern ID3D11BlendState* g_pBlendState;

extern UINT g_ResizeWidth;
extern UINT g_ResizeHeight;
extern bool g_SwapChainOccluded;

// Direct3D 管理函数声明
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();