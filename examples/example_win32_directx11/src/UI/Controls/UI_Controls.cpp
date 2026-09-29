#include "UI_Controls.h"

// 绘制 Mac 风格的圆点控制按钮（红/黄/绿：关闭、最大化、最小化）
bool DrawMacCircleButton(const char* id_str, const ImVec2& pos, float radius, MacBtnType type, bool isMaximized) {
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	if ( window->SkipItems ) return false;

	// 注册 ImGui 控件交互区域
	ImGuiID id = window->GetID(id_str);
	ImRect bb(ImVec2(pos.x - radius, pos.y - radius), ImVec2(pos.x + radius, pos.y + radius));

	ImGui::ItemSize(bb);
	if ( !ImGui::ItemAdd(bb, id) ) return false;

	// 获取按钮交互状态（悬停/点击）
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

	// 计算平滑悬停动画状态 (0.0f -> 1.0f)
	ImGuiStorage* storage = window->DC.StateStorage;
	float anim = storage->GetFloat(id, 0.0f);
	float dt = ImGui::GetIO( ).DeltaTime;

	anim = CustomLerp(anim, hovered ? 1.0f : 0.0f, dt * 12.0f);
	storage->SetFloat(id, anim);

	ImDrawList* drawList = ImGui::GetWindowDrawList( );
	ImVec2 mousePos = ImGui::GetMousePos( );

	// 根据类型匹配基准颜色（红/黄/绿）
	ImU32 baseThemeColor;
	if ( type == MAC_BTN_CLOSE )      baseThemeColor = IM_COL32(255, 75, 75, 255);
	else if ( type == MAC_BTN_MAXIMIZE ) baseThemeColor = IM_COL32(255, 185, 45, 255);
	else                            baseThemeColor = IM_COL32(50, 205, 80, 255);

	// 悬停时按钮微幅放大
	float currentRadius = radius + (radius * 0.26f * anim);

	// 绘制圆形玻璃感背景与高亮边框
	int bgAlpha = static_cast< int >(85.0f + 160.0f * anim);
	ImU32 glassBgCol = (baseThemeColor & 0x00FFFFFF) | (static_cast< unsigned int >(bgAlpha) << 24);
	drawList->AddCircleFilled(pos, currentRadius, glassBgCol);
	drawList->AddCircle(pos, currentRadius, IM_COL32(255, 255, 255, static_cast< int >(80.0f + 100.0f * anim)), 0, 1.2f);

	// 悬停动画：绘制跟随鼠标偏移的光斑与扩散发光效果
	if ( anim > 0.01f ) {
		ImVec2 delta(mousePos.x - pos.x, mousePos.y - pos.y);
		float distSq = delta.x * delta.x + delta.y * delta.y;
		float maxOffset = currentRadius * 0.45f;

		ImVec2 lightOffset(0, 0);
		if ( distSq > 0.0001f ) {
			float dist = std::sqrt(distSq);
			float clampDist = ( std::min ) (dist, maxOffset);
			lightOffset = ImVec2((delta.x / dist) * clampDist, (delta.y / dist) * clampDist);
		}

		// 绘制跟随光斑
		ImVec2 spotCenter(pos.x + lightOffset.x, pos.y + lightOffset.y);
		drawList->AddCircleFilled(spotCenter, currentRadius * 0.55f, IM_COL32(255, 255, 255, static_cast< int >(165.0f * anim)));

		// 绘制多层同心辉光
		for ( int i = 1; i <= 3; i++ ) {
			float alpha = 32.0f * anim / static_cast< float >(i);
			ImU32 glowCol = (baseThemeColor & 0x00FFFFFF) | (static_cast< unsigned int >(alpha) << 24);
			drawList->AddCircleFilled(spotCenter, currentRadius + (static_cast< float >(i) * 3.0f * anim), glowCol);
		}
	}

	// 绘制按钮内部图标 ( - / □ / X )
	ImU32 iconColor = IM_COL32(255, 255, 255, static_cast< int >(160 + 95 * anim));
	float iconScale = currentRadius * 0.42f;

	if ( type == MAC_BTN_MINIMIZE ) {
		// 最小化：横线
		drawList->AddLine(ImVec2(pos.x - iconScale, pos.y), ImVec2(pos.x + iconScale, pos.y), iconColor, 1.8f);
	}
	else if ( type == MAC_BTN_MAXIMIZE ) {
		if ( isMaximized ) {
			// 已最大化：绘制还原双框
			drawList->AddRect(ImVec2(pos.x - iconScale * 0.3f, pos.y - iconScale * 0.9f), ImVec2(pos.x + iconScale * 0.9f, pos.y + iconScale * 0.3f), iconColor, 0.0f, 0, 1.2f);
			drawList->AddRect(ImVec2(pos.x - iconScale * 0.9f, pos.y - iconScale * 0.3f), ImVec2(pos.x + iconScale * 0.3f, pos.y + iconScale * 0.9f), iconColor, 0.0f, 0, 1.2f);
		}
		else {
			// 普通状态：绘制正方形框
			drawList->AddRect(ImVec2(pos.x - iconScale * 0.8f, pos.y - iconScale * 0.8f), ImVec2(pos.x + iconScale * 0.8f, pos.y + iconScale * 0.8f), iconColor, 0.0f, 0, 1.5f);
		}
	}
	else if ( type == MAC_BTN_CLOSE ) {
		// 关闭：叉号
		drawList->AddLine(ImVec2(pos.x - iconScale * 0.75f, pos.y - iconScale * 0.75f), ImVec2(pos.x + iconScale * 0.75f, pos.y + iconScale * 0.75f), iconColor, 1.8f);
		drawList->AddLine(ImVec2(pos.x + iconScale * 0.75f, pos.y - iconScale * 0.75f), ImVec2(pos.x - iconScale * 0.75f, pos.y + iconScale * 0.75f), iconColor, 1.8f);
	}

	return pressed;
}

// 绘制侧边栏选项（处理隐形按键点击、悬停平滑右移与传递中心坐标）
bool DrawSidebarOption(const char* label, bool selected, const ImVec2& size, ImVec2* outCenterPos) {
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	if ( window->SkipItems ) return false;

	// 创建透明点击区域
	ImVec2 pos = ImGui::GetCursorScreenPos( );
	bool pressed = ImGui::InvisibleButton(label, size);
	bool hovered = ImGui::IsItemHovered( );

	// 更新悬停动画插值
	ImGuiStorage* storage = ImGui::GetStateStorage( );
	ImGuiID id = window->GetID(label);
	float anim = storage->GetFloat(id, 0.0f);
	float dt = ImGui::GetIO( ).DeltaTime;

	anim = CustomLerp(anim, hovered ? 6.0f : 0.0f, dt * 12.0f);
	storage->SetFloat(id, anim);

	// 输出该选项中心坐标（供侧边栏流体水滴胶囊吸附对齐）
	if ( outCenterPos ) {
		*outCenterPos = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList( );

	// 计算文字颜色（选中高亮，未选中根据悬停渐变）
	ImU32 textCol = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 205, 215, static_cast< int >(180.0f + 75.0f * anim));

	// 计算文字绘制位置（选中或悬停时向右微移）
	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImVec2 textPos(
		pos.x + 24.0f + (selected ? 4.0f : (anim * 3.0f)),
		pos.y + (size.y - textSize.y) * 0.5f
	);

	drawList->AddText(textPos, textCol, label);

	return pressed;
}