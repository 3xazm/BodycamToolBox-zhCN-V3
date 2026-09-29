#include "DashboardStyle.h"
#include "DashboardModel.h"
#include "imgui.h"

void DashboardStyle::RenderPageUI(DashboardModel& model, HWND hwnd, float scale) {
	// 1. Header 说明区域
	ImGui::TextDisabled("仪表盘与状态");
	ImGui::SetWindowFontScale(1.25f);
	ImGui::Text("系统概览与一键优化");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::TextDisabled("查看当前系统就绪状态及 Bodycam 游戏根路径配置。");

	ImGui::Spacing( );
	ImGui::Spacing( );

	// 2. 压入卡片风格样式
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));

	// 3. 绘制核心面板卡片
	if ( ImGui::BeginChild("DashboardCard", ImVec2(0, 220.0f * scale), true, ImGuiWindowFlags_None) ) {
		ImGui::TextColored(ImVec4(0.38f, 0.65f, 0.98f, 1.0f), "运行环境状态");
		ImGui::Separator( );
		ImGui::Spacing( );

		ImGui::Text("系统状态：%s", model.GetStatusMessage( ).c_str( ));
		ImGui::Spacing( );

		// 绑定路径 InputText（双向更新）
		char pathBuf[ 256 ];
		strncpy_s(pathBuf, model.GetGamePath( ).c_str( ), sizeof(pathBuf));

		ImGui::SetNextItemWidth(360.0f * scale);
		if ( ImGui::InputText("游戏根目录", pathBuf, sizeof(pathBuf)) ) {
			model.SetGamePath(pathBuf);
		}

		ImGui::Spacing( );
		ImGui::Spacing( );

		// 高亮一键优化按钮
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.48f, 1.00f, 0.60f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.48f, 1.00f, 0.80f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.00f, 0.48f, 1.00f, 1.00f));

		if ( ImGui::Button("一键优化配置", ImVec2(150.0f * scale, 38.0f * scale)) ) {
			model.ExecuteOptimization(hwnd);
		}

		ImGui::PopStyleColor(3);
	}
	ImGui::EndChild( );

	ImGui::PopStyleColor( );
	ImGui::PopStyleVar( );
}