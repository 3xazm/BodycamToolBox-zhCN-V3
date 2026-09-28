#include "AnimatedResolutionIcon.h"
#include "imgui_internal.h"

void DrawAnimatedResolutionIcon(
    ImDrawList* drawList,
    const ImVec2& centerPos,
    float iconSize,
    bool isSelected,
    AnimatedResolutionIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime
) {
    // 平滑插值更新选中透明度
    float targetAlpha = isSelected ? 1.0f : 0.0f;
    float lerpSpeed = 6.0f;
    state.selectAlpha += (targetAlpha - state.selectAlpha) * lerpSpeed * deltaTime;

    // 使用 ImGui 内部的 ImClamp
    state.selectAlpha = ImClamp(state.selectAlpha, 0.0f, 1.0f);

    float halfSize = iconSize * 0.5f;
    ImVec2 pMin(centerPos.x - halfSize, centerPos.y - halfSize);
    ImVec2 pMax(centerPos.x + halfSize, centerPos.y + halfSize);

    // 1. 绘制未选中态 (纯白/暗灰)
    if (state.selectAlpha < 1.0f) {
        float unselectAlpha = (1.0f - state.selectAlpha) * 255.0f;
        drawList->AddImage(
            texNoSelect,
            pMin, pMax,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, static_cast<int>(unselectAlpha))
        );
    }

    // 2. 绘制选中态 (渐变色)
    if (state.selectAlpha > 0.0f) {
        float selectAlpha = state.selectAlpha * 255.0f;
        drawList->AddImage(
            texSelect,
            pMin, pMax,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, static_cast<int>(selectAlpha))
        );
    }
}