#include "LocalizationPage.h"
#include "LocalizationStyle.h"

void LocalizationPage::Render( ) {
	// 绑定并渲染 Style UI
	LocalizationStyle::RenderPageUI(m_Model);
}