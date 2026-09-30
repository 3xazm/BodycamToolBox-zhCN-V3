#pragma once
#include <windows.h>
#include "..\..\src\UI\Theme\UI_Theme.h"
#include "..\..\src\UI\Header\UI_Header.h" // 包含 Header 头文件，获取 Header 绘制所需数据结构

// 暴露给 main.cpp / 全局调用的 UI 状态数据
extern HWND g_hWnd;                       // 主窗口句柄
extern float g_Scale;                     // DPI 缩放比例
extern int g_CurrentTab;                  // 当前选中的侧边栏 Tab 索引
extern char g_SearchBuffer[ 128 ];          // 搜索框输入缓冲区
extern LiquidAnimationState g_LiquidState;// 侧边栏流体选中动画状态

// 渲染管线核心接口
void InitAppRenderer(HWND hwnd, float scale); // 初始化渲染器与图标/特效纹理
void RenderFrame( );                           // 绘制一帧完整的 UI 界面
void ShutdownAppRenderer( );                   // 释放渲染器分配的 SRV 纹理与特效资源