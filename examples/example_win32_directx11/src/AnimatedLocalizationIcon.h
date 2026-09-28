#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

// 游戏汉化图标动画状态
struct AnimatedLocalizationIconState {
    float currentT = 0.0f; // 0.0f = 未选中, 1.0f = 选中
    float scale = 1.0f;    // 缩放比例
    float scaleVel = 0.0f; // 缩放速度 (弹簧阻尼)
};

// 渲染带有透明度渐变与轻微弹簧缩放效果的汉化图标
void DrawAnimatedLocalizationIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float iconSize,
    bool isSelected,
    AnimatedLocalizationIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime,
    ImU32 tintColor = IM_COL32(255, 255, 255, 255)
);