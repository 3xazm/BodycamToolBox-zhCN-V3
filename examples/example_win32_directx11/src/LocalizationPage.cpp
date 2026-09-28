#include "LocalizationPage.h"

void LocalizationPage::Render() {
    ImGui::Text("游戏汉化 / 语言补丁");
    ImGui::Separator();

    if (ImGui::Button("安装汉化补丁")) {
        // 在这里编写汉化逻辑
    }
}