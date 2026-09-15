#include "AnimatedSettingsIcon.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void DrawAnimatedSettingsIcon(
    ImDrawList* drawList,
    const ImVec2& center,
    float iconSize,
    bool isSelected,
    AnimatedSettingsIconState& state,
    ImTextureID texNoSelect,
    ImTextureID texSelect,
    float deltaTime
) {
    // 限制物理 deltaTime 步长，防止卡顿闪烁
    float dt = (deltaTime > 0.1f) ? 0.1f : deltaTime;

    // 1. 目标状态设定
    // 选中时：顺时针（向右）多转两圈 + 90度 = (2 * 2π) + (0.5 * π) = 4.5 * π
    // 未选中时：回到 0 度
    float targetAngle = isSelected ? (M_PI * 4.5f) : 0.0f;
    float targetScale = isSelected ? 1.15f : 1.0f;         // 选中时放大 15%

    // 2. 角度弹簧阻尼运动 (Spring-Damper Animation)
    float springStiffness = 180.0f;
    float springDamping = 12.0f;

    float angleDisp = state.angle - targetAngle;
    float angleAcc = -springStiffness * angleDisp - springDamping * state.angularVel;
    state.angularVel += angleAcc * dt;
    state.angle += state.angularVel * dt;

    // 3. 缩放弹簧阻尼运动
    float scaleDisp = state.scale - targetScale;
    float scaleAcc = -springStiffness * scaleDisp - springDamping * state.scaleVel;
    state.scaleVel += scaleAcc * dt;
    state.scale += state.scaleVel * dt;

    // 4. 计算旋转与亚像素微调
    float halfSize = (iconSize * 0.5f) * state.scale;

    // 微调渲染角度，静止时保留 0.001 弧度偏移，强制 GPU 保持双线性抗锯齿插值
    float renderAngle = state.angle;
    if (fabsf(renderAngle) < 0.001f) {
        renderAngle = 0.001f;
    }

    float cosA = cosf(renderAngle);
    float sinA = sinf(renderAngle);

    // 标准未旋转的前景/背景四顶点局部坐标
    ImVec2 localOffsets[4] = {
        ImVec2(-halfSize, -halfSize),
        ImVec2(halfSize, -halfSize),
        ImVec2(halfSize,  halfSize),
        ImVec2(-halfSize,  halfSize)
    };

    ImVec2 quadPts[4];
    for (int i = 0; i < 4; ++i) {
        quadPts[i] = ImVec2(
            center.x + localOffsets[i].x * cosA - localOffsets[i].y * sinA,
            center.y + localOffsets[i].x * sinA + localOffsets[i].y * cosA
        );
    }

    ImTextureID texToDraw = isSelected ? texSelect : texNoSelect;

    if (texToDraw) {
        drawList->AddImageQuad(
            texToDraw,
            quadPts[0], quadPts[1], quadPts[2], quadPts[3],
            ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1),
            IM_COL32_WHITE
        );
    }
}