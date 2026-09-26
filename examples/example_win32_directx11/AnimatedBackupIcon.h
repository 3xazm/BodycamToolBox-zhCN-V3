#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

// 存档备份图标的动画状态结构
struct AnimatedBackupIconState {
    float currentT = 0.0f; // 0.0f = 未选中, 1.0f = 选中
    float scale = 1.0f;    // 缩放比例
    float scaleVel = 0.0f; // 缩放速度 (弹簧动画)
};

// 渲染带有渐变淡入淡出与弹簧缩放效果的备份图标
void DrawAnimatedBackupIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float iconSize,
    bool isSelected,
    AnimatedBackupIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime,
    ImU32 tintColor = IM_COL32(255, 255, 255, 255)
);