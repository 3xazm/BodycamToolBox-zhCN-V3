#include "BackupPage.h"

void BackupPage::Render() {
    ImGui::Text("存档备份与还原");
    ImGui::Separator();

    if (ImGui::Button("立即备份存档")) {
        // 执行备份逻辑...
    }
}