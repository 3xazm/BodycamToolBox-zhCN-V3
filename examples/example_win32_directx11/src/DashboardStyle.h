#pragma once
#include <windows.h>

class DashboardModel; // 前置声明

class DashboardStyle {
public:
	static void RenderPageUI(DashboardModel& model, HWND hwnd, float scale);
};