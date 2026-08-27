#pragma once
#include <string>
#include <windows.h>

class ResolutionPage {
public:
    ResolutionPage();
    ~ResolutionPage() = default;

    // 绘制该页面的 ImGui UI 界面
    void Render();

private:
    // 内部数据状态
    std::string m_CurrentResolutionText;
    std::string m_BodycamResolutionText;
    std::string m_ResolutionStatusText;

    int m_BodycamWidth = 0;
    int m_BodycamHeight = 0;
    int m_SystemWidth = 0;
    int m_SystemHeight = 0;

    // 逻辑状态枚举 (0: 异常/Red, 1: 正常/Green, 2: 不一致/Yellow)
    int m_ResolutionState = 0;

    // 功能逻辑私有函数 (对应 C# 中的逻辑)
    bool IsBodycamRunning();
    void GetSystemResolution(int& width, int& height);
    std::wstring GetLocalAppDataPath();
    std::wstring FindBodycamExe();
    std::wstring SearchFileInDir(const std::wstring& root, const std::wstring& fileName);

    void LoadResolution();
    void LoadBodycamResolution();
    void CheckResolutionStatus();

    // 核心按钮事件
    void FixResolution();
    void DestroyResolution();
    void RefreshResolution();
    void FixBlackScreen();
    void FixRainbowScreen();
};