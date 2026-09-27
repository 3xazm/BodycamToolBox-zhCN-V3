#pragma once

class ResolutionModel; // 前置声明

class ResolutionStyle {
public:
    // 将模型以引用方式传入 UI 渲染模块
    static void RenderPageUI(ResolutionModel& model);
};