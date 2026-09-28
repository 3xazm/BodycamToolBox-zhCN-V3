#include "SettingStyle.h"
#include "SettingModel.h"
#include "imgui.h"

void SettingStyle::RenderPageUI(SettingModel& model) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

    // ==========================================
    // 卡片 1：通用偏好设置
    // ==========================================
    ImGui::BeginChild("GeneralSettingsCard", ImVec2(0, 180), true, 0);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "通用设置");
    ImGui::Separator();
    ImGui::Spacing();

    // 1. 开机自启开关
    bool autoStart = model.IsAutoStartEnabled();
    if (ImGui::Checkbox("开机自动启动工具箱", &autoStart)) {
        model.SetAutoStart(autoStart);
    }

    // 2. 窗口置顶开关
    bool alwaysOnTop = model.IsAlwaysOnTopEnabled();
    if (ImGui::Checkbox("保持工具箱窗口最前置顶", &alwaysOnTop)) {
        model.SetAlwaysOnTop(alwaysOnTop);
    }

    // 3. 关闭主窗口逻辑
    bool closeToTray = model.IsCloseToTrayEnabled();
    if (ImGui::Checkbox("点击关闭按钮时最小化到系统托盘", &closeToTray)) {
        model.SetCloseToTray(closeToTray);
    }

    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 2：个性化与主题设置
    // ==========================================
    ImGui::BeginChild("ThemeSettingsCard", ImVec2(0, 140), true, 0);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "个性化");
    ImGui::Separator();
    ImGui::Spacing();

    int currentTheme = model.GetSelectedTheme();
    const char* themes[] = { "亚克力暗黑 (默认)", "清新明亮", "赛博霓虹" };

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::Combo("界面主题样式", &currentTheme, themes, IM_ARRAYSIZE(themes))) {
        model.SetSelectedTheme(currentTheme);
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "提示：更换主题样式将即时作用于工具箱整体视图。");

    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 3：关于与版本信息
    // ==========================================
    ImGui::BeginChild("AboutSettingsCard", ImVec2(0, 150), true, 0);

    ImGui::Columns(2, "AboutCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "关于工具箱");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("软件版本: %s", model.GetAppVersion().c_str());
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.4f, 1.0f), "%s", model.GetUpdateStatusText().c_str());

    ImGui::NextColumn();

    ImGui::Dummy(ImVec2(0, 15));
    if (ImGui::Button("检查更新##Update", ImVec2(130, 38))) {
        model.CheckForUpdates();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
}