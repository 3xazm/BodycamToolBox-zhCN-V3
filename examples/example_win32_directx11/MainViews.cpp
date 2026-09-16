#include "MainViews.h"
#include "DashboardPage.h" // 1. 引入仪表盘头文件

// 声明定义在 AppRenderer.cpp 中的全局 HWND
extern HWND g_hWnd;

void RenderMainViews(int currentTab, float scale) {
    switch (currentTab) {
    case 0:
        RenderHomeView(scale);
        break;
    case 1:
        RenderResolutionFixView(scale);
        break;
    case 2:
        RenderSettingsView(scale);
        break;
    default:
        break;
    }
}

void RenderHomeView(float scale) {
    // 2. 将原本的简单文本替换为 DashboardPage 逻辑
    RenderDashboardPage(g_hWnd, scale);
}

void RenderResolutionFixView(float scale) {
    ImGui::Text("分辨率修复设置模块");
    ImGui::Separator();
}

void RenderSettingsView(float scale) {
    ImGui::Text("配置设置");
    ImGui::Separator();
}