#pragma once
#include "ResolutionModel.h"

class ResolutionPage {
public:
    ResolutionPage() = default;
    ~ResolutionPage() = default;

    // 绘制该页面的 ImGui UI 界面
    void Render();

private:
    ResolutionModel m_Model;
};