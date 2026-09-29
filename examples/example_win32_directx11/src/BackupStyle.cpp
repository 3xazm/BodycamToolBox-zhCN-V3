#include "BackupStyle.h"
#include "BackupModel.h"
#include "imgui.h"

void BackupStyle::RenderPageUI(BackupModel& model) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

    // ==========================================
    // 卡片 1：存档位置与快捷操作
    // ==========================================
    ImGui::BeginChild("BackupHeaderCard", ImVec2(0, 140), true, 0);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "存档管理与备份");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("%s", model.GetSavePathText().c_str());
    ImGui::Spacing();

    if (ImGui::Button("立即创建备份", ImVec2(140, 36))) {
        model.CreateBackup();
    }
    ImGui::SameLine();
    if (ImGui::Button("刷新存档状态", ImVec2(120, 36))) {
        model.RefreshSavePath();
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.4f, 1.0f), "  状态: %s", model.GetStatusMessage().c_str());

    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 2：备份历史列表
    // ==========================================
    ImGui::BeginChild("BackupListCard", ImVec2(0, 320), true, 0);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "历史备份列表");
    ImGui::Separator();
    ImGui::Spacing();

    const auto& list = model.GetBackupList();
    if (list.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "暂无备份文件，点击上方按钮创建新备份。");
    }
    else {
        // 表格展示历史备份
        if (ImGui::BeginTable("BackupTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("备份文件名", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("创建时间", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("大小", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)list.size(); ++i) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", list[i].name.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", list[i].date.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", list[i].size.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::PushID(i);
                if (ImGui::Button("还原")) {
                    model.RestoreBackup(i);
                }
                ImGui::SameLine();
                if (ImGui::Button("删除")) {
                    model.DeleteBackup(i);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleVar(2);
}