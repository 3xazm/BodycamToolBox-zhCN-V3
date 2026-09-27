#include "SettingPage.h"
#include "SettingStyle.h"

void SettingPage::Render() {
    // 绑定并渲染 Style UI
    SettingStyle::RenderPageUI(m_Model);
}