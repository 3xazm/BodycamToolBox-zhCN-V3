#include "ResolutionStyle.h"
#include "ResolutionModel.h"
#include "imgui.h"

void ResolutionStyle::RenderPageUI(ResolutionModel& model) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

    // ==========================================
    // 卡片 1：分辨率修复
    // ==========================================
    ImGui::BeginChild("ResolutionFixCard", ImVec2(0, 310), true, 0);

    ImGui::Columns(2, "ResCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "分辨率修复 1");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.0f, 0.66f, 0.58f, 1.0f), "%s", model.GetCurrentResolutionText().c_str());

    // 根据 Model 状态渲染不同颜色的状态文本
    int resState = model.GetResolutionState();
    ImVec4 statusColor = (resState == 1) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
        ((resState == 2) ? ImVec4(1.0f, 0.84f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

    ImGui::TextColored(statusColor, "%s", model.GetBodycamResolutionText().c_str());
    ImGui::TextColored(statusColor, "%s", model.GetResolutionStatusText().c_str());

    ImGui::Spacing();
    ImGui::TextWrapped("1- 修复笔记本 或 多显示器环境下导致Bodycam的分辨率异常问题。");
    ImGui::TextWrapped("2- 如画面不完整等 以及 无法对画面正常点击等。");
    ImGui::TextWrapped("3- 画面下面出现黑边，画面移出屏幕外等。");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.74f, 0.56f, 0.56f, 1.0f), "--*如果Bodycam工具箱加载不出来当前分辨率可点击刷新一下按钮。");

    // 右侧按钮栏
    ImGui::NextColumn();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.04f, 0.03f, 1.0f));
    if (ImGui::Button("0.破坏分辨率", ImVec2(130, 30))) {
        model.DestroyResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 15));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    if (ImGui::Button("1.修复分辨率", ImVec2(130, 45))) {
        model.FixResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 15));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.56f, 0.56f, 1.0f));
    if (ImGui::Button("2.刷新一下", ImVec2(130, 40))) {
        model.RefreshResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 2：黑屏修复
    // ==========================================
    ImGui::BeginChild("BlackScreenFixCard", ImVec2(0, 180), true, 0);

    ImGui::Columns(2, "BlackCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "黑屏修复 2");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("1- 本修复方案针对特定兼容性黑屏有效 如黑屏有声音/音乐等");
    ImGui::TextWrapped("2- 仅支持正版Bodycam版本，非官方版本 或 假入库无法保证修复效果。");
    ImGui::TextWrapped("3- 如黑屏嘟嘟的响，可能需检查Watt Toolkit或第三方加速器等");
    ImGui::TextWrapped("4- 修复过程预计耗时6 秒。");

    ImGui::NextColumn();
    ImGui::Dummy(ImVec2(0, 30));
    if (ImGui::Button("修复##BlackScreen", ImVec2(120, 40))) {
        model.FixBlackScreen();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 3：五颜六色彩虹修复
    // ==========================================
    ImGui::BeginChild("RainbowFixCard", ImVec2(0, 160), true, 0);

    ImGui::Columns(2, "RainbowCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "五颜六色彩虹修复 3");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("1- 五颜六色彩虹修复是指游戏中出现的彩虹色异常问题 或 闪屏");
    ImGui::TextWrapped("2- 按Tab键的玩家数据面板出现闪屏 或 彩色异常。");
    ImGui::TextWrapped("3- 开局时出现彩虹色异常。");

    ImGui::NextColumn();
    ImGui::Dummy(ImVec2(0, 25));
    if (ImGui::Button("修复##Rainbow", ImVec2(120, 40))) {
        model.FixRainbowScreen();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
}