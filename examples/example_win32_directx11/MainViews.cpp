#include "MainViews.h"
#include "DashboardPage.h"
#include "ResolutionPage.h" // 1. 包含 ResolutionPage 头文件

extern HWND g_hWnd;

void RenderMainViews(int currentTab, float scale) {
    switch (currentTab) {
    case 0:
        RenderHomeView(scale);
        break;
    case 1:
        RenderResolutionFixView(scale); // 当 currentTab 为 1 时调用
        break;
    case 2:
        RenderSettingsView(scale);
        break;
    default:
        break;
    }
}

void RenderHomeView(float scale) {
    RenderDashboardPage(g_hWnd, scale);
}

void RenderResolutionFixView(float scale) {
    // 2. 使用 static 保证对象生命周期贯穿整个程序，避免每帧重复创建与状态丢失
    static ResolutionPage g_ResolutionPage;

    // 3. 调用类的 Render() 方法完成界面与交互绘制
    g_ResolutionPage.Render();
}

void RenderSettingsView(float scale) {
    ImGui::Text("配置设置");
    ImGui::Separator();
}