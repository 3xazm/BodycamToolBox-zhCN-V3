#pragma once
#include "imgui.h"

// 设置图标的动画状态数据结构
struct AnimatedSettingsIconState {
    float angle = 0.0f;         // 旋转角度 (弧度)
    float angularVel = 0.0f;    // 旋转角速度
    float scale = 1.0f;         // 缩放比例
    float scaleVel = 0.0f;      // 缩放速度
};

// 渲染带有旋转与缩放动画的设置图标
void DrawAnimatedSettingsIcon(
    ImDrawList* drawList,
    const ImVec2& center,
    float iconSize,
    bool isSelected,
    AnimatedSettingsIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime
);