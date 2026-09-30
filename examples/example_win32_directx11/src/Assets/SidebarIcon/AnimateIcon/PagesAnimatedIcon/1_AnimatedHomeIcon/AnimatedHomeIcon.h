#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

// 首页图标的动画状态结构
struct AnimatedHomeIconState {
	float currentT = 0.0f; // 0.0 = 未选中, 1.0 = 选中
};

// 绘制首页动画图标（声明）
void DrawAnimatedHomeIcon(
	ImDrawList* drawList,
	ImVec2 centerPos,
	float iconSize,
	bool isSelected,
	AnimatedHomeIconState& state,
	ImTextureID texNoSelect,
	ImTextureID texSelect,
	float deltaTime,
	ImU32 tintColor = IM_COL32(255, 255, 255, 255)
);