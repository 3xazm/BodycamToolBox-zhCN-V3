#include "MainViews.h"
#include "DashboardPage.h"
#include "ResolutionPage.h" 
#include "BackupPage.h"
#include "LocalizationPage.h" // 1. 包含汉化页面头文件

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
        RenderBackupView(scale);
        break;
    case 3:
        RenderLocalizationView(scale); // 2. Tab 3 映射到汉化页面
        break;
    case 4:
        RenderSettingsView(scale); // 3. Tab 4 映射到设置页面
        break;
    default:
        break;
    }
}

// 渲染--首页
void RenderHomeView(float scale) {
    RenderDashboardPage(g_hWnd, scale);
}

// 渲染--分辨率修复
void RenderResolutionFixView(float scale) {
    static ResolutionPage g_ResolutionPage;
    g_ResolutionPage.Render();
}

// 渲染--备份
void RenderBackupView(float scale) {
    static BackupPage g_BackupPage;
    g_BackupPage.Render();
}

// 渲染--游戏汉化
void RenderLocalizationView(float scale) {
    static LocalizationPage g_LocalizationPage;
    g_LocalizationPage.Render();
}

// 渲染--设置
void RenderSettingsView(float scale) {
    ImGui::Text("配置设置");
    ImGui::Separator();
}