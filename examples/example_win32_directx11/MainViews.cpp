#include "MainViews.h"

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
    ImGui::Text("右侧内容面板");
    ImGui::Separator();
    ImGui::Text("欢迎使用全新的液态玻璃界面系统。");
}

void RenderResolutionFixView(float scale) {
    ImGui::Text("分辨率修复设置模块");
    ImGui::Separator();
}

void RenderSettingsView(float scale) {
    ImGui::Text("配置设置");
    ImGui::Separator();
}