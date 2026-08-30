#pragma once
#include "imgui.h"
#include "UI_Theme.h"
#include "UI_Controls.h"

// 渲染侧边栏背景、选项列表及动态液态胶囊
void RenderSidebar(
    ImDrawList* drawList,
    int& currentTab,
    LiquidAnimationState& liquidState,
    const ImVec2& sidebarPos,
    float sidebarW,
    float contentH,
    float scale,
    float deltaTime
);