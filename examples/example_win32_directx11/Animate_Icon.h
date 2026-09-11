#pragma once
#include "imgui.h"

// 图标变形控制结构体
struct MorphIconState {
    float currentT = 0.0f; // 当前插值进度 (0.0 = House, 1.0 = Arrow)
};

// 绘制并更新 MorphIcon (House <-> Arrow-Right)
void DrawMorphHomeIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float size,
    bool isSelected,
    MorphIconState& state,
    float deltaTime,
    ImU32 color = IM_COL32(255, 255, 255, 230)
);