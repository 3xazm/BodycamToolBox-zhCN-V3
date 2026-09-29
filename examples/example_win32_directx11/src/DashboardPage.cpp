#include "DashboardPage.h"
#include "DashboardStyle.h"

void DashboardPage::Render(HWND hwnd, float scale) {
	// 调用 Style 绘制并传入 Model 实例及句柄参数
	DashboardStyle::RenderPageUI(m_Model, hwnd, scale);
}