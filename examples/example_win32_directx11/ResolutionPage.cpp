#include "ResolutionPage.h"
#include "ResolutionStyle.h"

void ResolutionPage::Render() {
    // 调度 Style 渲染 UI，并传入 Model 实例
    ResolutionStyle::RenderPageUI(m_Model);
}