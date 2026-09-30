#include "BackupPage.h"
#include "BackupStyle.h"

void BackupPage::Render() {
    // 调用 Style 绘制并传入 Model
    BackupStyle::RenderPageUI(m_Model);
}