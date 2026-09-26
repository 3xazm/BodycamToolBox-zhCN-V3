#include "AnimatedLocalizationIcon.h"

void DrawAnimatedLocalizationIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float iconSize,
    bool isSelected,
    AnimatedLocalizationIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime,
    ImU32 tintColor
) {
    if (!drawList || iconSize <= 0.0f) return;

    float dt = (deltaTime <= 0.0f) ? 0.016f : (deltaTime > 0.1f ? 0.1f : deltaTime);

    // 1. 透明度平滑插值 (Lerp)
    float targetT = isSelected ? 1.0f : 0.0f;
    state.currentT += (targetT - state.currentT) * dt * 8.0f;
    state.currentT = (std::min)((std::max)(state.currentT, 0.0f), 1.0f);
    float t = state.currentT;

    // 2. 选中/未选中弹簧阻尼缩放动画
    float targetScale = isSelected ? 1.10f : 1.0f; // 选中时放大 10%
    float springStiffness = 180.0f;
    float springDamping = 12.0f;
    float scaleDisp = state.scale - targetScale;
    float scaleAcc = -springStiffness * scaleDisp - springDamping * state.scaleVel;
    state.scaleVel += scaleAcc * dt;
    state.scale += state.scaleVel * dt;

    // 3. 像素坐标对齐，防止半像素渲染模糊
    float scaledSize = std::floor(iconSize * state.scale);
    if ((int)scaledSize % 2 != 0) scaledSize -= 1.0f;
    float halfSize = scaledSize * 0.5f;
    ImVec2 snappedCenter(std::floor(centerPos.x), std::floor(centerPos.y));

    ImVec2 pMin(snappedCenter.x - halfSize, snappedCenter.y - halfSize);
    ImVec2 pMax(snappedCenter.x + halfSize, snappedCenter.y + halfSize);

    // A. 绘制未选中态 (淡出)
    if (t < 0.99f && texNoSelect) {
        float alphaFactor = 1.0f - t;
        unsigned int alphaByte = static_cast<unsigned int>(((tintColor >> 24) & 0xFF) * alphaFactor);
        ImU32 colorNoSelect = (tintColor & 0x00FFFFFF) | (alphaByte << 24);

        drawList->AddImage(texNoSelect, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), colorNoSelect);
    }

    // B. 绘制选中态 (淡入)
    if (t > 0.01f && texSelect) {
        float alphaFactor = t;
        unsigned int alphaByte = static_cast<unsigned int>(((tintColor >> 24) & 0xFF) * alphaFactor);
        ImU32 colorSelect = (tintColor & 0x00FFFFFF) | (alphaByte << 24);

        drawList->AddImage(texSelect, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), colorSelect);
    }
}