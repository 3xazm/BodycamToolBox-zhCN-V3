#pragma once
#include "imgui.h"
#include "..\..\src\UI\Theme\UI_Theme.h"
#include "..\..\src\UI\Controls\UI_Controls.h"

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