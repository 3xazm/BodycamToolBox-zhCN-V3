#pragma once
#include <string>

class SettingModel {
public:
    SettingModel();
    ~SettingModel() = default;

    // Getters & Setters
    bool IsAutoStartEnabled() const { return m_AutoStart; }
    void SetAutoStart(bool enable);

    bool IsAlwaysOnTopEnabled() const { return m_AlwaysOnTop; }
    void SetAlwaysOnTop(bool enable);

    bool IsCloseToTrayEnabled() const { return m_CloseToTray; }
    void SetCloseToTray(bool enable);

    int GetSelectedTheme() const { return m_SelectedTheme; }
    void SetSelectedTheme(int themeIndex);

    const std::string& GetAppVersion() const { return m_AppVersion; }
    const std::string& GetUpdateStatusText() const { return m_UpdateStatusText; }

    // 逻辑功能示例
    void CheckForUpdates();

private:
    bool m_AutoStart = false;
    bool m_AlwaysOnTop = false;
    bool m_CloseToTray = true;
    int m_SelectedTheme = 0; // 0: 暗黑亚克力, 1: 浅色明亮, 2: 赛博朋克

    std::string m_AppVersion = "v1.2.0";
    std::string m_UpdateStatusText = "当前已是最新版本";
};