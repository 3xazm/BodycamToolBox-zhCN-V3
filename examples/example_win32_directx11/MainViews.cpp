#include "MainViews.h"
#include "DashboardPage.h"
#include "ResolutionPage.h" 
#include "BackupPage.h"

extern HWND g_hWnd;

void RenderMainViews(int currentTab, float scale) {
    switch (currentTab) {
    case 0:
		RenderHomeView(scale); //首页
        break;
    case 1:
        RenderResolutionFixView(scale); //分辨率
        break;
    case 2:
		RenderBackupView(scale); //备份
        break;
    case 3:
        RenderSettingsView(scale); //设置
        break;
    default:
        break;
    }
}

//渲染--首页
void RenderHomeView(float scale) {
    RenderDashboardPage(g_hWnd, scale);
}

//渲染--分辨率修复
void RenderResolutionFixView(float scale) {
    // 2. 使用 static 保证对象生命周期贯穿整个程序，避免每帧重复创建与状态丢失
    static ResolutionPage g_ResolutionPage;
    // 3. 调用类的 Render() 方法完成界面与交互绘制
    g_ResolutionPage.Render();
}

//渲染--备份
void RenderBackupView(float scale) {
    static BackupPage g_BackupPage;
    g_BackupPage.Render();
}

//渲染--设置
void RenderSettingsView(float scale) {
    ImGui::Text("配置设置");
    ImGui::Separator();
}