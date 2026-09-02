#pragma once
#include <windows.h>
#include "UI_Theme.h"
#include "UI_Header.h" // <--- 必须补充包含 UI_Header.h

// 暴露给 main.cpp 调用的全局 UI 状态
extern HWND g_hWnd;
extern float g_Scale;
extern int g_CurrentTab;
extern char g_SearchBuffer[128];
extern LiquidAnimationState g_LiquidState;

// 渲染初始化与帧绘制函数
void InitAppRenderer(HWND hwnd, float scale);
void RenderFrame();
void ShutdownAppRenderer();