#include "UI_Sidebar.h"
#include "SidebarIcon_HomeSelect.h"
#include "SidebarIcon_HomeNoSelect.h"
#include "IconTextureLoader.h"
#include "AnimatedHomeIcon.h"

// 静态纹理句柄与图标动画状态
static ImTextureID g_TexHomeSelect = (ImTextureID)0;
static ImTextureID g_TexHomeNoSelect = (ImTextureID)0;
static bool g_TextureLoadedAttempted = false; // 防止重复无效加载
static AnimatedHomeIconState g_HomeIconState;

// 引用 main.cpp 中的全局 D3D 设备
extern ID3D11Device* g_pd3dDevice;

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
    // 0. 首次绘制时只加载一次 AOB 纹理数据
    if (!g_TextureLoadedAttempted && g_pd3dDevice) {
        g_TextureLoadedAttempted = true;
        g_TexHomeSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_HomeSelectData, SidebarIcon_HomeSelectDataSize, 0);
        g_TexHomeNoSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_HomeNoSelectData, SidebarIcon_HomeNoSelectDataSize, 3);
    }

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
    ImVec2 homeOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale);
    ImGui::SetCursorPos(homeOptPos);

    if (DrawSidebarOption("       首页", currentTab == 0, ImVec2(optItemW, optItemH), &optPositions[0])) {
        currentTab = 0;
    }

    // 安全调用图标绘制 (当纹理非空时)
    if (g_TexHomeSelect && g_TexHomeNoSelect) {
        float iconSize = 20.0f * scale; // 尝试适当微调大小
        ImVec2 homeIconCenter(homeOptPos.x + 22.0f * scale, homeOptPos.y + optItemH * 0.5f);

        DrawAnimatedHomeIcon(
            drawList,
            homeIconCenter,
            iconSize,
            currentTab == 0,
            g_HomeIconState,
            g_TexHomeNoSelect,
            g_TexHomeSelect,
            deltaTime
        );
    }

    // 选项 1：分辨率修复
    ImVec2 resOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + optItemH + 8.0f * scale);
    ImGui::SetCursorPos(resOptPos);
    if (DrawSidebarOption("   分辨率修复", currentTab == 1, ImVec2(optItemW, optItemH), &optPositions[1])) {
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