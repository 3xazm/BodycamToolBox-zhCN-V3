#include "UI_Sidebar.h"

void RenderSidebar(
    ImDrawList* drawList,
    int& currentTab,
    LiquidAnimationState& liquidState,
    const ImVec2& sidebarPos,
    float sidebarW,
    float contentH,
    float scale,
    float deltaTime
) {
    // 1. 绘制侧边栏玻璃感背景与边框
    drawList->AddRectFilled(
        sidebarPos,
        ImVec2(sidebarPos.x + sidebarW, sidebarPos.y + contentH),
        IM_COL32(255, 255, 255, 15),
        16.0f * scale
    );
    drawList->AddRect(
        sidebarPos,
        ImVec2(sidebarPos.x + sidebarW, sidebarPos.y + contentH),
        IM_COL32(255, 255, 255, 50),
        16.0f * scale
    );

    // 2. 布局计算与选项渲染
    float optItemW = sidebarW - 20.0f * scale;
    float optItemH = 38.0f * scale;
    ImVec2 optPositions[3];

    // 选项 0：首页
    ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale));
    if (DrawSidebarOption("首页", currentTab == 0, ImVec2(optItemW, optItemH), &optPositions[0])) {
        currentTab = 0;
    }

    // 选项 1：分辨率修复
    ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + optItemH + 8.0f * scale));
    if (DrawSidebarOption("分辨率修复", currentTab == 1, ImVec2(optItemW, optItemH), &optPositions[1])) {
        currentTab = 1;
    }

    // 选项 2：设置（固定在底部）
    ImGui::SetCursorPos(ImVec2(sidebarPos.x + 10.0f * scale, sidebarPos.y + contentH - optItemH - 12.0f * scale));
    if (DrawSidebarOption("设置", currentTab == 2, ImVec2(optItemW, optItemH), &optPositions[2])) {
        currentTab = 2;
    }

    // 3. 渲染液态流体胶囊与水痕效果
    RenderLiquidCapsule(
        drawList,
        liquidState,
        currentTab,
        sidebarPos,
        optItemW,
        optItemH,
        optPositions,
        scale,
        deltaTime
    );
}