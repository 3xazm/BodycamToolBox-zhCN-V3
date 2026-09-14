#pragma once
#include "imgui.h"

struct AnimatedResolutionIconState {
    float selectAlpha = 0.0f; // 0.0f = 未选中, 1.0f = 选中
};

void DrawAnimatedResolutionIcon(
    ImDrawList* drawList,
    const ImVec2& centerPos,
    float iconSize,
    bool isSelected,
    AnimatedResolutionIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime
);