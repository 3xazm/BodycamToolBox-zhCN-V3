#pragma once
#include <string>
#include <windows.h>

class ResolutionModel {
public:
    ResolutionModel();
    ~ResolutionModel() = default;

    // 状态获取
    const std::string& GetCurrentResolutionText() const { return m_CurrentResolutionText; }
    const std::string& GetBodycamResolutionText() const { return m_BodycamResolutionText; }
    const std::string& GetResolutionStatusText() const { return m_ResolutionStatusText; }
    int GetResolutionState() const { return m_ResolutionState; }

    // 核心业务功能
    void RefreshResolution();
    void FixResolution();
    void DestroyResolution();
    void FixBlackScreen();
    void FixRainbowScreen();

private:
    std::string m_CurrentResolutionText;
    std::string m_BodycamResolutionText;
    std::string m_ResolutionStatusText;

    int m_BodycamWidth = 0;
    int m_BodycamHeight = 0;
    int m_SystemWidth = 0;
    int m_SystemHeight = 0;
    int m_ResolutionState = 0; // 0: Null/Red, 1: 一致/Green, 2: 不一致/Yellow

    // 逻辑辅助函数
    bool IsBodycamRunning();
    void GetSystemResolution(int& width, int& height);
    std::wstring GetLocalAppDataPath();
    std::wstring FindBodycamExe();
    std::wstring SearchFileInDir(const std::wstring& root, const std::wstring& fileName);

    void LoadResolution();
    void LoadBodycamResolution();
    void CheckResolutionStatus();
};