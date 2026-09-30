#pragma once
#include <windows.h>
#include "DashboardModel.h"

class DashboardPage {
public:
	DashboardPage( ) = default;
	~DashboardPage( ) = default;

	// 适配 MainViews 调用的渲染入口
	void Render(HWND hwnd, float scale);

private:
	DashboardModel m_Model;
};