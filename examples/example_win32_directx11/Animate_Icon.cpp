#include "Animate_Icon.h"
#include <cmath>

static inline float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline ImVec2 LerpVec2(const ImVec2& a, const ImVec2& b, float t) {
    return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

// 二维点旋转
static inline ImVec2 RotateVec2(const ImVec2& v, float angleRad) {
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    return ImVec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

// 平滑贝塞尔圆角折线绘制
static void PathStrokeRounded(
    ImDrawList* drawList,
    const ImVec2* pts,
    int count,
    float radius,
    ImU32 color,
    float thickness,
    bool closed = false
) {
    if (count < 2) return;
    drawList->PathClear();

    for (int i = 0; i < count; ++i) {
        if (!closed && (i == 0 || i == count - 1)) {
            drawList->PathLineTo(pts[i]);
            continue;
        }

        ImVec2 pPrev = pts[(i - 1 + count) % count];
        ImVec2 pCurr = pts[i];
        ImVec2 pNext = pts[(i + 1) % count];

        ImVec2 d1 = ImVec2(pPrev.x - pCurr.x, pPrev.y - pCurr.y);
        ImVec2 d2 = ImVec2(pNext.x - pCurr.x, pNext.y - pCurr.y);
        float len1 = sqrtf(d1.x * d1.x + d1.y * d1.y);
        float len2 = sqrtf(d2.x * d2.x + d2.y * d2.y);

        if (len1 < 0.001f || len2 < 0.001f) {
            drawList->PathLineTo(pCurr);
            continue;
        }

        d1 = ImVec2(d1.x / len1, d1.y / len1);
        d2 = ImVec2(d2.x / len2, d2.y / len2);

        float r = radius;
        if (r > len1 * 0.45f) r = len1 * 0.45f;
        if (r > len2 * 0.45f) r = len2 * 0.45f;

        ImVec2 startPt = ImVec2(pCurr.x + d1.x * r, pCurr.y + d1.y * r);
        ImVec2 endPt = ImVec2(pCurr.x + d2.x * r, pCurr.y + d2.y * r);

        drawList->PathLineTo(startPt);
        drawList->PathBezierCubicCurveTo(
            LerpVec2(startPt, pCurr, 0.55f),
            LerpVec2(endPt, pCurr, 0.55f),
            endPt, 4
        );
    }

    drawList->PathStroke(color, closed, thickness);
}

void DrawMorphHomeIcon(
    ImDrawList* drawList,
    ImVec2 centerPos,
    float size,
    bool isSelected,
    MorphIconState& state,
    float deltaTime,
    ImU32 color
) {
    if (!drawList || size <= 0.0f) return;

    // 1. 动画插值 (0.0 = House, 1.0 = Arrow-Right)
    float targetT = isSelected ? 1.0f : 0.0f;
    float dt = (deltaTime > 0.1f) ? 0.1f : deltaTime;
    state.currentT = LerpFloat(state.currentT, targetT, dt * 10.0f);

    if (state.currentT < 0.0001f) state.currentT = 0.0f;
    if (state.currentT > 0.9999f) state.currentT = 1.0f;
    float t = state.currentT;

    float halfS = size * 0.5f;

    // 2. 旋转角度：从 89° (House) 旋转回 0° (Arrow-Right)
    float angleDeg = (1.0f - t) * 89.0f;
    float angleRad = angleDeg * (3.14159265f / 180.0f);

    // 3. 微调后的精准拓扑坐标 (让屋顶更高更贴合原图)
    ImVec2 houseOuter[5] = {
        ImVec2(0.65f, -0.62f), // 左下
        ImVec2(-0.08f, -0.62f), // 左墙顶
        ImVec2(-0.82f,  0.00f), // 屋顶尖（拉高到 -0.82，形成更漂亮的弧形拱顶）
        ImVec2(-0.08f,  0.62f), // 右墙顶
        ImVec2(0.65f,  0.62f)  // 右下
    };

    ImVec2 houseDoor[4] = {
        ImVec2(0.65f, -0.26f), // 门左下
        ImVec2(0.12f, -0.26f), // 门左上
        ImVec2(0.12f,  0.26f), // 门右上
        ImVec2(0.65f,  0.26f)  // 门右下
    };

    ImVec2 arrowOuter[5] = {
        ImVec2(-0.15f, -0.72f),
        ImVec2(0.25f, -0.32f),
        ImVec2(0.72f,  0.00f),
        ImVec2(0.25f,  0.32f),
        ImVec2(-0.15f,  0.72f)
    };

    ImVec2 arrowShaft[4] = {
        ImVec2(-0.72f,  0.00f),
        ImVec2(-0.20f,  0.00f),
        ImVec2(0.20f,  0.00f),
        ImVec2(0.72f,  0.00f)
    };

    // 4. 坐标插值与旋转
    ImVec2 pOuter[5];
    for (int i = 0; i < 5; ++i) {
        ImVec2 p = LerpVec2(houseOuter[i], arrowOuter[i], t);
        p = RotateVec2(p, angleRad);
        pOuter[i] = ImVec2(centerPos.x + p.x * halfS, centerPos.y + p.y * halfS);
    }

    ImVec2 pInner[4];
    for (int i = 0; i < 4; ++i) {
        ImVec2 p = LerpVec2(houseDoor[i], arrowShaft[i], t);
        p = RotateVec2(p, angleRad);
        pInner[i] = ImVec2(centerPos.x + p.x * halfS, centerPos.y + p.y * halfS);
    }

    // 5. 增大屋顶圆角半径，营造超圆润视觉
    float stroke = 2.0f;
    float houseCornerRadius = size * 0.14f; // 增大圆角，使屋顶变成完美的弯弯弧形

    // 6. 渲染外廓
    PathStrokeRounded(drawList, pOuter, 5, houseCornerRadius, color, stroke, false);

    // 7. 渲染 House 门框与底线 / Arrow 箭轴
    if (t < 0.8f) {
        float houseAlpha = (1.0f - t / 0.8f);
        unsigned int alphaByte = static_cast<unsigned int>(((color >> 24) & 0xFF) * houseAlpha);
        ImU32 houseColor = (color & 0x00FFFFFF) | (alphaByte << 24);

        // 门框圆角
        PathStrokeRounded(drawList, pInner, 4, houseCornerRadius * 0.5f, houseColor, stroke, false);

        // 底部连线
        drawList->PathClear();
        drawList->PathLineTo(pOuter[0]);
        drawList->PathLineTo(pOuter[4]);
        drawList->PathStroke(houseColor, false, stroke);
    }

    if (t > 0.2f) {
        float shaftAlpha = (t - 0.2f) / 0.8f;
        unsigned int alphaByte = static_cast<unsigned int>(((color >> 24) & 0xFF) * shaftAlpha);
        ImU32 shaftColor = (color & 0x00FFFFFF) | (alphaByte << 24);

        // 箭轴
        drawList->PathClear();
        drawList->PathLineTo(pInner[0]);
        drawList->PathLineTo(pInner[3]);
        drawList->PathStroke(shaftColor, false, stroke);
    }
}