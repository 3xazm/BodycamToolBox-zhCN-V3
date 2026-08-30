#pragma once
#include "imgui.h"

// 渲染右侧主内容视图
void RenderMainViews(int currentTab, float scale);

// 各个子页面的绘制函数
void RenderHomeView(float scale);
void RenderResolutionFixView(float scale);
void RenderSettingsView(float scale);