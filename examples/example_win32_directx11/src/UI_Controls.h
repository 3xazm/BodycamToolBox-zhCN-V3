#pragma once
#include <windows.h>
#include <cmath>
#include <algorithm>
#include "imgui.h"
#include "imgui_internal.h"

// 缓动动画工具函数
static inline float CustomLerp(float a, float b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

// Mac 窗口控制按钮类型
enum MacBtnType {
    MAC_BTN_CLOSE,
    MAC_BTN_MAXIMIZE,
    MAC_BTN_MINIMIZE
};

// 自定义控件函数声明
bool DrawMacCircleButton(const char* id_str, const ImVec2& pos, float radius, MacBtnType type, bool isMaximized = false);
bool DrawSidebarOption(const char* label, bool selected, const ImVec2& size, ImVec2* outCenterPos = nullptr);