#pragma once
#include <vector>
#include "imgui.h"

// 蒸发水痕结构体
struct TrailSegment {
    float y;
    float alpha;
    float width;
    float height;
};

// 液态胶囊动画状态管理器
struct LiquidAnimationState {
    float currentY = -1.0f;
    float lastY = -1.0f;
    float velY = 0.0f;
    float stretch = 0.0f;
    float fluidWeight = 6.0f;
    bool isFirstFrame = true;
    std::vector<TrailSegment> trails;
};

// 设置苹果玻璃感主题样式
void SetupAppleGlassTheme(float scale);

// 渲染液态流体胶囊与水痕系统
void RenderLiquidCapsule(
    ImDrawList* drawList,
    LiquidAnimationState& state,
    int currentTab,
    const ImVec2& sidebarPos,
    float optItemW,
    float optItemH,
    const ImVec2 optPositions[],
    float scale,
    float deltaTime
);