#include "LocalizationStyle.h"
#include "LocalizationModel.h"
#include "imgui.h"

void LocalizationStyle::RenderPageUI(LocalizationModel& model) {
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

	// ==========================================
	// 卡片 1：汉化补丁安装控制台
	// ==========================================
	ImGui::BeginChild("LocalizationControlCard", ImVec2(0, 160), true, 0);

	ImGui::Columns(2, "LocCols", false);
	ImGui::SetColumnWidth(0, ImGui::GetWindowWidth( ) - 180.0f);

	ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "游戏汉化 / 语言补丁");
	ImGui::Separator( );
	ImGui::Spacing( );

	ImGui::TextWrapped("%s", model.GetGamePathText( ).c_str( ));
	ImGui::Spacing( );

	ImGui::Text("当前状态: ");
	ImGui::SameLine( );
	if ( model.IsPatchInstalled( ) ) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", model.GetStatusMessage( ).c_str( ));
	}
	else {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", model.GetStatusMessage( ).c_str( ));
	}

	// 右侧按钮栏
	ImGui::NextColumn( );
	ImGui::Dummy(ImVec2(0, 10));

	if ( !model.IsPatchInstalled( ) ) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
		if ( ImGui::Button("安装汉化补丁", ImVec2(140, 42)) ) {
			model.InstallPatch( );
		}
		ImGui::PopStyleColor( );
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.04f, 0.03f, 1.0f));
		if ( ImGui::Button("卸载汉化补丁", ImVec2(140, 42)) ) {
			model.UninstallPatch( );
		}
		ImGui::PopStyleColor( );
	}

	ImGui::Dummy(ImVec2(0, 10));
	if ( ImGui::Button("检测/刷新状态", ImVec2(140, 32)) ) {
		model.RefreshStatus( );
	}

	ImGui::Columns(1);
	ImGui::EndChild( );

	ImGui::Spacing( );

	// ==========================================
	// 卡片 2：可用补丁版本列表
	// ==========================================
	ImGui::BeginChild("PatchListCard", ImVec2(0, 280), true, 0);

	ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "选择汉化包版本");
	ImGui::Separator( );
	ImGui::Spacing( );

	const auto& patches = model.GetAvailablePatches( );
	int selectedIndex = model.GetSelectedPatchIndex( );

	for ( int i = 0; i < ( int ) patches.size( ); ++i ) {
		ImGui::PushID(i);
		bool isSelected = (selectedIndex == i);

		if ( ImGui::Selectable(patches[ i ].name.c_str( ), isSelected, 0, ImVec2(0, 60)) ) {
			model.SetSelectedPatchIndex(i);
		}

		// 自定义绘制列表项目细节
		ImGui::SameLine(ImGui::GetWindowWidth( ) - 150.0f);
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "版本: %s", patches[ i ].version.c_str( ));

		ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) - 35.0f);
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  作者: %s | %s", patches[ i ].author.c_str( ), patches[ i ].description.c_str( ));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) + 10.0f);

		ImGui::Separator( );
		ImGui::PopID( );
	}

	ImGui::EndChild( );

	ImGui::PopStyleVar(2);
}