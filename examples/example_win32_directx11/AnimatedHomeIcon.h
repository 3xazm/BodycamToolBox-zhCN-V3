#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

struct AnimatedHomeIconState {
    float currentT = 0.0f; // 0.0 = 未选中, 1.0 = 选中
};

inline void DrawAnimatedHomeIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float iconSize,
    bool isSelected,
    AnimatedHomeIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime,
    ImU32 tintColor = IM_COL32(255, 255, 255, 255)
) {
    if (!drawList || iconSize <= 0.0f) return;

    // 1. 平滑插值
    float dt = (deltaTime <= 0.0f) ? 0.016f : (deltaTime > 0.1f ? 0.1f : deltaTime);
    float targetT = isSelected ? 1.0f : 0.0f;
    state.currentT += (targetT - state.currentT) * dt * 6.0f;
    state.currentT = (std::min)((std::max)(state.currentT, 0.0f), 1.0f);
    float t = state.currentT;

    // 2. 关键：对坐标和尺寸进行像素对齐 (Pixel Snap)，防止半像素插值拉伸
    float snappedSize = std::floor(iconSize);
    if ((int)snappedSize % 2 != 0) snappedSize -= 1.0f; // 保证偶数尺寸以便精准中心对齐

    float halfSize = snappedSize * 0.5f;
    ImVec2 snappedCenter(std::floor(centerPos.x), std::floor(centerPos.y));

    // --------------------------------------------------------
    // A. 未选中图标 (NoSelect)
    // --------------------------------------------------------
    if (t < 0.99f && texNoSelect) {
        ImVec2 pMin(snappedCenter.x - halfSize, snappedCenter.y - halfSize);
        ImVec2 pMax(snappedCenter.x + halfSize, snappedCenter.y + halfSize);

        float alphaFactor = 1.0f - t;
        unsigned int alphaByte = static_cast<unsigned int>(((tintColor >> 24) & 0xFF) * alphaFactor);
        ImU32 colorNoSelect = (tintColor & 0x00FFFFFF) | (alphaByte << 24);

        drawList->AddImage(texNoSelect, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), colorNoSelect);
    }

    // --------------------------------------------------------
    // B. 选中铺满图标 (Select) - 中心扩散
    // --------------------------------------------------------
    if (t > 0.01f && texSelect) {
        float halfS = halfSize * t;

        ImVec2 pMin(snappedCenter.x - halfS, snappedCenter.y - halfS);
        ImVec2 pMax(snappedCenter.x + halfS, snappedCenter.y + halfS);

        float alphaFactor = t;
        unsigned int alphaByte = static_cast<unsigned int>(((tintColor >> 24) & 0xFF) * alphaFactor);
        ImU32 colorSelect = (tintColor & 0x00FFFFFF) | (alphaByte << 24);

        drawList->AddImage(texSelect, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), colorSelect);
    }
}