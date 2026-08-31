#include "UI_Theme.h"
#include "UI_Controls.h"
#include <cmath>
#include <algorithm>

// 设置苹果玻璃感主题样式
void SetupAppleGlassTheme(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 16.0f * scale;      // 主窗口圆角
    style.ChildRounding = 12.0f * scale;       // 子窗口圆角
    style.FrameRounding = 8.0f * scale;        // 控件圆角
    style.PopupRounding = 10.0f * scale;       // 弹窗圆角
    style.ScrollbarRounding = 10.0f * scale;   // 滚动条圆角
    style.GrabRounding = 8.0f * scale;        // 抓手圆角
    style.WindowBorderSize = 0.0f;             // 无主边框
    style.ChildBorderSize = 1.0f;              // 子窗口 1px 半透明边框

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.12f, 0.30f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
}

// 渲染液态流体胶囊与水痕系统 (含蓝色提示条上下呼吸动画)
void RenderLiquidCapsule(
    ImDrawList* drawList,
    LiquidAnimationState& state,
    int currentTab,
    const ImVec2& sidebarPos,
    float optItemW,
    float optItemH,
    const ImVec2 optPositions[],
    float scale,
    float deltaTime
) {
    // 计算当前选中项的目标 Y 轴位置
    float targetY = optPositions[currentTab].y - optItemH * 0.5f;

    // 1. 液态胶囊移动物理弹性逻辑
    if (state.isFirstFrame) {
        state.currentY = targetY;
        state.lastY = targetY;
        state.isFirstFrame = false;
    }
    else {
        float dist = targetY - state.currentY;
        float stiffness = (dist > 0.0f) ? 200.0f : 230.0f;
        float damping = (std::abs(dist) < 12.0f) ? 6.0f : 12.0f;

        float force = dist * stiffness;
        state.velY += force * deltaTime;
        state.velY -= state.velY * damping * deltaTime;
        state.currentY += state.velY * deltaTime;

        // 计算形变拉伸系数
        float targetStretch = (std::abs(state.velY) / 500.0f);
        targetStretch = (std::min)(targetStretch, 0.5f);
        state.stretch = CustomLerp(state.stretch, targetStretch, deltaTime * 18.0f);

        // 快速移动时产生拖尾蒸发水痕
        if (std::abs(state.currentY - state.lastY) > 4.0f * scale) {
            TrailSegment seg;
            seg.y = (state.currentY + state.lastY) * 0.5f + optItemH * 0.5f;
            seg.alpha = 0.45f;
            seg.width = (optItemW - 30.0f * scale) * (1.0f - state.stretch * 0.2f);
            seg.height = std::abs(state.currentY - state.lastY) + 6.0f * scale;
            state.trails.push_back(seg);
            state.lastY = state.currentY;
        }
    }

    // 2. 绘制并更新蒸发水痕
    for (auto it = state.trails.begin(); it != state.trails.end(); ) {
        it->alpha -= deltaTime * 1.2f;

        if (it->alpha <= 0.0f) {
            it = state.trails.erase(it);
        }
        else {
            float trailCenterX = sidebarPos.x + 10.0f * scale + optItemW * 0.5f;
            ImVec2 tMin(trailCenterX - it->width * 0.5f, it->y - it->height * 0.5f);
            ImVec2 tMax(trailCenterX + it->width * 0.5f, it->y + it->height * 0.5f);

            int fillAlpha = static_cast<int>(it->alpha * 90.0f);
            int borderAlpha = static_cast<int>(it->alpha * 140.0f);

            drawList->AddRectFilled(tMin, tMax, IM_COL32(255, 255, 255, fillAlpha), 8.0f * scale);
            drawList->AddRect(tMin, tMax, IM_COL32(255, 255, 255, borderAlpha), 0, 1.0f);
            ++it;
        }
    }

    // 3. 计算胶囊与悬停状态
    ImVec2 liquidMin(sidebarPos.x + 10.0f * scale, state.currentY);
    ImVec2 liquidMax(liquidMin.x + optItemW, liquidMin.y + optItemH);

    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 selectedOptMin(sidebarPos.x + 10.0f * scale, optPositions[currentTab].y - optItemH * 0.5f);
    ImVec2 selectedOptMax(selectedOptMin.x + optItemW, selectedOptMin.y + optItemH);

    // 判断鼠标是否悬停在当前选中的 Tab 项上
    bool isHoveringLiquid = (mousePos.x >= selectedOptMin.x && mousePos.x <= selectedOptMax.x &&
        mousePos.y >= selectedOptMin.y && mousePos.y <= selectedOptMax.y);

    float targetWeight = isHoveringLiquid ? 2.0f : 0.0f;
    state.fluidWeight = CustomLerp(state.fluidWeight, targetWeight, deltaTime * 4.0f);

    // 4. 绘制液态胶囊波浪轮廓
    const int numSegments = 64;
    ImVec2 wavePoints[64];
    float time = static_cast<float>(ImGui::GetTime()) * 2.8f;

    ImVec2 center(liquidMin.x + optItemW * 0.5f, liquidMin.y + optItemH * 0.5f);
    float rx = optItemW * 0.49f;
    float ry = optItemH * 0.49f;

    float stretchY = 1.0f + state.stretch * 0.45f;
    float stretchX = 1.0f - state.stretch * 0.20f;

    for (int i = 0; i < numSegments; ++i) {
        float a = (static_cast<float>(i) / static_cast<float>(numSegments)) * 2.0f * 3.14159265f;

        float wave1 = std::sin(a * 2.0f + time) * 2.0f;
        float wave2 = std::cos(a * 3.0f - time * 0.8f) * 1.2f;

        float sinA = std::sin(a);
        float cosA = std::cos(a);

        float moveDir = (state.velY >= 0.0f) ? 1.0f : -1.0f;

        float dropletAsymmetry = (sinA * moveDir < 0.0f) ? (1.0f - std::abs(sinA) * 0.38f * state.stretch)
            : (1.0f + std::abs(sinA) * 0.18f * state.stretch);

        float organicOffset = (wave1 + wave2) * state.fluidWeight;

        float power = CustomLerp(8.5f, 6.2f, state.fluidWeight);
        float pX = std::pow(std::abs(cosA), 2.0f / power) * (cosA >= 0 ? 1.0f : -1.0f);
        float pY = std::pow(std::abs(sinA), 2.0f / power) * (sinA >= 0 ? 1.0f : -1.0f);

        wavePoints[i] = ImVec2(
            center.x + (pX * rx * stretchX * 0.98f) + cosA * organicOffset,
            center.y + (pY * ry * stretchY * dropletAsymmetry * 0.85f) + sinA * organicOffset
        );
    }

    float motionAlphaExtra = (std::min)(state.stretch * 90.0f, 50.0f);
    int bgAlpha = static_cast<int>(CustomLerp(35.0f, 55.0f, state.fluidWeight) + motionAlphaExtra);
    int borderAlpha = static_cast<int>(CustomLerp(90.0f, 220.0f, state.fluidWeight) + motionAlphaExtra);

    drawList->AddConvexPolyFilled(wavePoints, numSegments, IM_COL32(255, 255, 255, bgAlpha));
    drawList->AddPolyline(wavePoints, numSegments, IM_COL32(255, 255, 255, borderAlpha), ImDrawFlags_Closed, 1.5f);

    // ------------------------------------------------------------------------
    // 5. 蓝色选中提示条 (增加悬停上下呼吸与伸缩动画)
    // ------------------------------------------------------------------------
    float blueOffsetY = 0.0f;  // Y轴呼吸平移偏移
    float blueHeightExpand = 0.0f; // 高度扩张/收缩
    int blueAlpha = 230;       // 基础透明度

    if (isHoveringLiquid) {
        // 当鼠标悬停时，利用正弦波 (sin) 计算上下上下浮动的呼吸效果
        float breathTime = static_cast<float>(ImGui::GetTime()) * 5.0f; // 呼吸频率
        blueOffsetY = std::sin(breathTime) * 2.5f * scale; // 上下平移 ±2.5px
        blueHeightExpand = std::cos(breathTime * 0.8f) * 1.5f * scale; // 上下高度伸缩

        // 透明度在 200 ~ 255 之间呼吸微调
        blueAlpha = static_cast<int>(225.0f + std::sin(breathTime * 0.5f) * 30.0f);
    }

    // 计算蓝色提示条动态坐标
    ImVec2 barMin(
        liquidMin.x + 6.0f * scale,
        liquidMin.y + (8.0f * scale) + blueOffsetY - blueHeightExpand
    );
    ImVec2 barMax(
        liquidMin.x + 11.0f * scale,
        liquidMax.y - (8.0f * scale) + blueOffsetY + blueHeightExpand
    );

    // 绘制蓝色提示条
    drawList->AddRectFilled(
        barMin, barMax,
        IM_COL32(0, 122, 255, blueAlpha), 2.0f * scale
    );
}