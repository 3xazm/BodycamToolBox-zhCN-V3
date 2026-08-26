#include "DashboardPage.h"
#include "imgui.h"

void RenderDashboardPage(HWND hwnd, float main_scale)
{
    // WPF / Lepoco 风格 Header 结构
    ImGui::TextDisabled("仪表盘与状态");
    ImGui::SetWindowFontScale(1.25f);
    ImGui::Text("系统概览与一键优化");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("查看当前系统就绪状态及 Bodycam 游戏根路径配置。");

    ImGui::Spacing();
    ImGui::Spacing();

    // 经典 Card 背景框
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f * main_scale);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));

    if (ImGui::BeginChild("DashboardCard", ImVec2(0, 220.0f * main_scale), true, ImGuiWindowFlags_None))
    {
        ImGui::TextColored(ImVec4(0.38f, 0.65f, 0.98f, 1.0f), "运行环境状态");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("系统状态：已就绪");
        ImGui::Spacing();

        static char game_path[256] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Bodycam";
        ImGui::SetNextItemWidth(360.0f * main_scale);
        ImGui::InputText("游戏根目录", game_path, sizeof(game_path));

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("一键优化配置", ImVec2(150.0f * main_scale, 38.0f * main_scale))) {
            MessageBoxW(hwnd, L"配置优化完成！", L"提示", MB_OK);
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}