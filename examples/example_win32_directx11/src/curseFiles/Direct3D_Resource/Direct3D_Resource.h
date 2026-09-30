#pragma once
#include <d3d11.h>

// 全局 Direct3D 11 基础资源变量声明 (通过 extern 在多个模块间共享)
extern ID3D11Device* g_pd3dDevice;                  // D3D11 设备对象，用于创建资源
extern ID3D11DeviceContext* g_pd3dDeviceContext;    // D3D11 设备上下文，用于提交渲染指令
extern IDXGISwapChain* g_pSwapChain;                // DXGI 交换链，管理前后缓冲
extern ID3D11RenderTargetView* g_mainRenderTargetView; // 主渲染目标视图 (RTV)
extern ID3D11BlendState* g_pBlendState;             // 针对 Alpha 混合设置的 BlendState

// 窗口尺寸变化与交换链状态
extern UINT g_ResizeWidth;
extern UINT g_ResizeHeight;
extern bool g_SwapChainOccluded;

// Direct3D 11 生命周期管理函数
bool CreateDeviceD3D(HWND hWnd);  // 初始化 D3D 设备与交换链
void CleanupDeviceD3D( );         // 释放所有 D3D 资源
void CreateRenderTarget( );       // 基于交换链后备缓冲创建 RTV
void CleanupRenderTarget( );      // 释放 RTV 视图