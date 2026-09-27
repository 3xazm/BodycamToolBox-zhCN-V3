#include "SettingModel.h"
#include <windows.h>

SettingModel::SettingModel() {
    // 可以在此处读取配置文件或注册表初始化状态
}

void SettingModel::SetAutoStart(bool enable) {
    m_AutoStart = enable;
    // TODO: 后续写入开机自启注册表 HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
}

void SettingModel::SetAlwaysOnTop(bool enable) {
    m_AlwaysOnTop = enable;
    // TODO: 后续调用 SetWindowPos 设置 HWND_TOPMOST / HWND_NOTOPMOST
}

void SettingModel::SetCloseToTray(bool enable) {
    m_CloseToTray = enable;
}

void SettingModel::SetSelectedTheme(int themeIndex) {
    m_SelectedTheme = themeIndex;
    // TODO: 后续动态切换 ImGui 主题或背景着色器
}

void SettingModel::CheckForUpdates() {
    m_UpdateStatusText = "正在检查更新...";
    // TODO: 后续发起 HTTP 请求请求最新版本号
    m_UpdateStatusText = "当前已是最新版本 (v1.2.0)";
}