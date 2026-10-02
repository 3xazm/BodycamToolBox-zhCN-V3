#include "ResolutionStyle.h"
#include "ResolutionModel.h"
#include "imgui.h"

// 辅助：渲染卡片顶部标题与装饰条
static void RenderHeader(const char* title, ImVec4 accentColor) {
	ImDrawList* drawList = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );

	// 绘制左侧 3px 宽的小色条
	drawList->AddRectFilled(p, ImVec2(p.x + 3.0f, p.y + 18.0f), ImGui::GetColorU32(accentColor), 2.0f);

	ImGui::SetCursorPosX(ImGui::GetCursorPosX( ) + 10.0f);
	ImGui::TextColored(accentColor, "%s", title);
	ImGui::Spacing( );
}

// 辅助：渲染状态徽章 (Badge)
static void RenderStatusBadge(const char* text, int state) {
	ImU32 bgColor;
	switch ( state ) {
	case 1:  bgColor = IM_COL32(0, 180, 100, 160); break;  // 正常 - 绿
	case 2:  bgColor = IM_COL32(220, 150, 0, 160); break;  // 警告 - 黄
	default: bgColor = IM_COL32(200, 50, 50, 160); break;   // 错误 - 红
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImVec2 padding(8.0f, 3.0f);
	ImVec2 badgeSize(textSize.x + padding.x * 2, textSize.y + padding.y * 2);

	drawList->AddRectFilled(p, ImVec2(p.x + badgeSize.x, p.y + badgeSize.y), bgColor, 4.0f);
	drawList->AddText(ImVec2(p.x + padding.x, p.y + padding.y), IM_COL32(255, 255, 255, 255), text);

	ImGui::Dummy(badgeSize); // 占位
}

void ResolutionStyle::RenderPageUI(ResolutionModel& model) {
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));

	// 计算网格列数与卡片宽度
	float availWidth = ImGui::GetContentRegionAvail( ).x;
	float minCardWidth = 320.0f; // 单张卡片的最小理想宽度
	float spacing = 12.0f;       // 卡片之间的间距

	// 根据总宽度计算一行能容纳几列 (至少1列)
	int columns = ( int ) ((availWidth + spacing) / (minCardWidth + spacing));
	if ( columns < 1 ) columns = 1;

	// 计算实际卡片宽度，使其平分宽度占满整行
	float cardWidth = (availWidth - (columns - 1) * spacing) / columns;
	float cardHeight = 220.0f; // 统一卡片高度

	int cardIndex = 0;

	auto PrepareNextCard = [&] (int index) {
		if ( index > 0 ) {
			if ( index % columns != 0 ) {
				ImGui::SameLine(0.0f, spacing); // 同一行，接着画下一个卡片
			}
			else {
				ImGui::Spacing( ); // 换行
			}
		}
		};

	// ==========================================
	// 卡片 1：分辨率优化
	// ==========================================
	PrepareNextCard(cardIndex++);
	ImGui::BeginChild("Card_ResFix", ImVec2(cardWidth, cardHeight), true, 0);
	{
		RenderHeader("分辨率优化", ImVec4(0.0f, 0.75f, 0.85f, 1.0f));

		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", model.GetCurrentResolutionText( ).c_str( ));
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", model.GetBodycamResolutionText( ).c_str( ));

		ImGui::Spacing( );
		RenderStatusBadge(model.GetResolutionStatusText( ).c_str( ), model.GetResolutionState( ));
		ImGui::Spacing( );

		ImGui::PushTextWrapPos(ImGui::GetCursorPos( ).x + cardWidth - 28.0f);
		ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.75f, 1.0f), "• 解决笔记本或多显示器画面拉伸、黑边及点击失效。");
		ImGui::PopTextWrapPos( );

		// 底部操作按钮：占满全宽的单一一键修复按钮
		ImGui::SetCursorPosY(ImGui::GetWindowHeight( ) - 50.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.60f, 0.45f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.75f, 0.55f, 1.0f));
		if ( ImGui::Button("一键修复", ImVec2(cardWidth - 28.0f, 34)) ) {
			model.FixResolution( );
		}
		ImGui::PopStyleColor(2);
	}
	ImGui::EndChild( );

	// ==========================================
	// 卡片 2：黑屏修复
	// ==========================================
	PrepareNextCard(cardIndex++);
	ImGui::BeginChild("Card_BlackScreenFix", ImVec2(cardWidth, cardHeight), true, 0);
	{
		RenderHeader("黑屏/启动异常修复", ImVec4(0.0f, 0.75f, 0.85f, 1.0f));

		ImGui::PushTextWrapPos(ImGui::GetCursorPos( ).x + cardWidth - 28.0f);
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "解决游戏启动后有声音/音乐但画面黑屏的问题。");
		ImGui::Spacing( );
		ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.75f, 1.0f), "• 仅支持 Steam 正版 Bodycam。");
		ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.75f, 1.0f), "• 修复预计耗时约 6 秒。");
		ImGui::PopTextWrapPos( );

		// 底部按钮
		ImGui::SetCursorPosY(ImGui::GetWindowHeight( ) - 50.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.38f, 0.56f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.48f, 0.70f, 1.0f));
		if ( ImGui::Button("修复黑屏", ImVec2(cardWidth - 28.0f, 34)) ) {
			model.FixBlackScreen( );
		}
		ImGui::PopStyleColor(2);
	}
	ImGui::EndChild( );

	// ==========================================
	// 卡片 3：五颜六色彩虹修复
	// ==========================================
	PrepareNextCard(cardIndex++);
	ImGui::BeginChild("Card_RainbowFix", ImVec2(cardWidth, cardHeight), true, 0);
	{
		RenderHeader("彩虹屏 & 闪屏修复", ImVec4(0.0f, 0.75f, 0.85f, 1.0f));

		ImGui::PushTextWrapPos(ImGui::GetCursorPos( ).x + cardWidth - 28.0f);
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "修复因 HDR 或色彩渲染冲突导致的色彩异常及闪屏问题。");
		ImGui::Spacing( );
		ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.75f, 1.0f), "• 解决开局彩虹色异常。");
		ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.75f, 1.0f), "• 解决按 Tab 查看计分板时的画面闪烁。");
		ImGui::PopTextWrapPos( );

		// 底部按钮
		ImGui::SetCursorPosY(ImGui::GetWindowHeight( ) - 50.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.22f, 0.52f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.28f, 0.68f, 1.0f));
		if ( ImGui::Button("修复彩虹屏", ImVec2(cardWidth - 28.0f, 34)) ) {
			model.FixRainbowScreen( );
		}
		ImGui::PopStyleColor(2);
	}
	ImGui::EndChild( );

	ImGui::PopStyleVar(2);
}