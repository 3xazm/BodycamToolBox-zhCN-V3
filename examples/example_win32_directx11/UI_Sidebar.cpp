#include "UI_Sidebar.h"
#include "SidebarIcon_HomeSelect.h"
#include "SidebarIcon_HomeNoSelect.h"
#include "SidebarIcon_ResolutionSelect.h"
#include "SidebarIcon_ResolutionNoSelect.h"
#include "SidebarIcon_SettingsSelect.h"
#include "SidebarIcon_SettingsNoSelect.h"
#include "IconTextureLoader.h"
#include "AnimatedHomeIcon.h"
#include "AnimatedResolutionIcon.h"
#include "AnimatedSettingsIcon.h"

// 静态纹理句柄与图标动画状态
static ImTextureID g_TexHomeSelect = (ImTextureID)0;
static ImTextureID g_TexHomeNoSelect = (ImTextureID)0;
static ImTextureID g_TexResSelect = (ImTextureID)0;
static ImTextureID g_TexResNoSelect = (ImTextureID)0;
static ImTextureID g_TexSettingsSelect = (ImTextureID)0;
static ImTextureID g_TexSettingsNoSelect = (ImTextureID)0;
static bool g_TextureLoadedAttempted = false; // 防止重复加载

static AnimatedHomeIconState g_HomeIconState;
static AnimatedResolutionIconState g_ResIconState;
static AnimatedSettingsIconState g_SettingsIconState;

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
    // 0. 首次绘制时加载所有侧边栏纹理数据
    if (!g_TextureLoadedAttempted && g_pd3dDevice) {
        g_TextureLoadedAttempted = true;
        // Home 图标纹理
        g_TexHomeSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_HomeSelectData, SidebarIcon_HomeSelectDataSize, 0);
        g_TexHomeNoSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_HomeNoSelectData, SidebarIcon_HomeNoSelectDataSize, 3);
        // Resolution 图标纹理
        g_TexResSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_ResolutionSelectData, SidebarIcon_ResolutionSelectDataSize, 0);
        g_TexResNoSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_ResolutionNoSelectData, SidebarIcon_ResolutionNoSelectDataSize, 3);
        // Settings 图标纹理
        g_TexSettingsSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_SettingsSelectData, SidebarIcon_SettingsSelectDataSize, 0);
        g_TexSettingsNoSelect = LoadTextureFromMemory(g_pd3dDevice, SidebarIcon_SettingsNoSelectData, SidebarIcon_SettingsNoSelectDataSize, 3);
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
    ImVec2 optPositions[5];

    // ----------------------------------------------------
    // 选项 0：首页
    // ----------------------------------------------------
    ImVec2 homeOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale);
    ImGui::SetCursorPos(homeOptPos);

    if (DrawSidebarOption("       首页", currentTab == 0, ImVec2(optItemW, optItemH), &optPositions[0])) {
        currentTab = 0;
    }

    if (g_TexHomeSelect && g_TexHomeNoSelect) {
        float iconSize = 20.0f * scale;
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

    // ----------------------------------------------------
    // 选项 1：分辨率修复
    // ----------------------------------------------------
    ImVec2 resOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + optItemH + 8.0f * scale);
    ImGui::SetCursorPos(resOptPos);
    if (DrawSidebarOption("       分辨率修复", currentTab == 1, ImVec2(optItemW, optItemH), &optPositions[1])) {
        currentTab = 1;
    }

    if (g_TexResSelect && g_TexResNoSelect) {
        float iconSize = 20.0f * scale;
        ImVec2 resIconCenter(resOptPos.x + 22.0f * scale, resOptPos.y + optItemH * 0.5f);

        DrawAnimatedResolutionIcon(
            drawList,
            resIconCenter,
            iconSize,
            currentTab == 1,
            g_ResIconState,
            g_TexResNoSelect,
            g_TexResSelect,
            deltaTime
        );
    }

    // ----------------------------------------------------
    // 选项 2：存档备份
    // ----------------------------------------------------
    ImVec2 backupOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + (optItemH + 8.0f * scale) * 2);
    ImGui::SetCursorPos(backupOptPos);
    if (DrawSidebarOption("       存档备份", currentTab == 2, ImVec2(optItemW, optItemH), &optPositions[2])) {
        currentTab = 2;
    }

    // ----------------------------------------------------
    // 【新增】选项 3：游戏汉化
    // ----------------------------------------------------
    ImVec2 locOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + 12.0f * scale + (optItemH + 8.0f * scale) * 3);
    ImGui::SetCursorPos(locOptPos);
    if (DrawSidebarOption("       游戏汉化", currentTab == 3, ImVec2(optItemW, optItemH), &optPositions[3])) {
        currentTab = 3;
    }

    // ----------------------------------------------------
    // 选项 4：设置（固定在底部）
    // ----------------------------------------------------
    ImVec2 settingsOptPos(sidebarPos.x + 10.0f * scale, sidebarPos.y + contentH - optItemH - 12.0f * scale);
    ImGui::SetCursorPos(settingsOptPos);
    if (DrawSidebarOption("       设置", currentTab == 4, ImVec2(optItemW, optItemH), &optPositions[4])) {
        currentTab = 4;
    }

    if (g_TexSettingsSelect && g_TexSettingsNoSelect) {
        float iconSize = 24.0f * scale;
        ImVec2 settingsIconCenter(settingsOptPos.x + 22.0f * scale, settingsOptPos.y + optItemH * 0.5f);

        DrawAnimatedSettingsIcon(
            drawList,
            settingsIconCenter,
            iconSize,
            currentTab == 4,
            g_SettingsIconState,
            g_TexSettingsNoSelect,
            g_TexSettingsSelect,
            deltaTime
        );
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