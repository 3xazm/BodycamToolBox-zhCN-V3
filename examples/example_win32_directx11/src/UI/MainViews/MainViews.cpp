#include "MainViews.h"
#include "..\..\src\UI\MainViews\MainViewsPages\Pages\1_Dashboard\DashboardPage.h"
#include "..\..\src\UI\MainViews\MainViewsPages\Pages\2_Resolution\ResolutionPage.h"
#include "..\..\src\UI\MainViews\MainViewsPages\Pages\3_Backup\BackupPage.h"
#include "..\..\src\UI\MainViews\MainViewsPages\Pages\4_Localization\LocalizationPage.h"
#include "..\..\src\UI\MainViews\MainViewsPages\PagesSettings\SettingPage.h"

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
        RenderLocalizationView(scale); 
        break;
    case 4:
        RenderSettingsView(scale); 
        break;
    default:
        break;
    }
}

// 渲染--首页
void RenderHomeView(float scale) {
	static DashboardPage g_DashboardPage;
	g_DashboardPage.Render(g_hWnd, scale);
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
    static SettingPage g_SettingPage;
    g_SettingPage.Render();
}