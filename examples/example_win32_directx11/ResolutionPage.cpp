#include "ResolutionPage.h"
#include "imgui.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

ResolutionPage::ResolutionPage() {
    // 页面初始化时即刻加载状态
    RefreshResolution();
}

// -----------------------------------------------------------------------------
// 逻辑辅助函数 (Native C++ 实现)
// -----------------------------------------------------------------------------

bool ResolutionPage::IsBodycamRunning() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    bool isRunning = false;

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Bodycam.exe") == 0) {
                isRunning = true;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return isRunning;
}

void ResolutionPage::GetSystemResolution(int& width, int& height) {
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}

std::wstring ResolutionPage::GetLocalAppDataPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

std::wstring ResolutionPage::SearchFileInDir(const std::wstring& root, const std::wstring& fileName) {
    std::wstring searchPath = root + L"\\" + fileName;
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return root + L"\\" + fileName;
    }

    // 递归子目录
    std::wstring subSearch = root + L"\\*";
    hFind = FindFirstFileW(subSearch.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return L"";

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                std::wstring subDir = root + L"\\" + findData.cFileName;
                std::wstring result = SearchFileInDir(subDir, fileName);
                if (!result.empty()) {
                    FindClose(hFind);
                    return result;
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return L"";
}

std::wstring ResolutionPage::FindBodycamExe() {
    DWORD drives = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (drives & 1) {
            std::wstring root = { letter, L':', L'\\' };
            UINT type = GetDriveTypeW(root.c_str());
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                std::wstring result = SearchFileInDir(root, L"Bodycam.exe");
                if (!result.empty()) return result;
            }
        }
        drives >>= 1;
    }
    return L"";
}

void ResolutionPage::LoadResolution() {
    GetSystemResolution(m_SystemWidth, m_SystemHeight);
    m_CurrentResolutionText = "当前你的系统分辨率为: " + std::to_string(m_SystemWidth) + " x " + std::to_string(m_SystemHeight);
}

void ResolutionPage::LoadBodycamResolution() {
    std::wstring iniPath = GetLocalAppDataPath() + L"\\Bodycam\\Saved\\Config\\Windows\\GameUserSettings.ini";

    std::ifstream file(iniPath);
    if (!file.is_open()) {
        m_BodycamResolutionText = "当前你的Bodycam分辨率为: Null";
        m_ResolutionState = 0;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    file.close();

    std::smatch matchX, matchY;
    std::regex regX("ResolutionSizeX=(\\d+)");
    std::regex regY("ResolutionSizeY=(\\d+)");

    if (std::regex_search(text, matchX, regX) && std::regex_search(text, matchY, regY)) {
        m_BodycamWidth = std::stoi(matchX[1].str());
        m_BodycamHeight = std::stoi(matchY[1].str());

        m_BodycamResolutionText = "当前你的Bodycam分辨率为: " + std::to_string(m_BodycamWidth) + " x " + std::to_string(m_BodycamHeight);

        if (m_BodycamWidth == m_SystemWidth && m_BodycamHeight == m_SystemHeight) {
            m_ResolutionState = 1; // 一致 (绿色)
        }
        else {
            m_ResolutionState = 2; // 不一致 (黄色)
        }
    }
    else {
        m_BodycamResolutionText = "当前你的Bodycam分辨率为: Null";
        m_ResolutionState = 0;
    }
}

void ResolutionPage::CheckResolutionStatus() {
    if (m_ResolutionState == 1) {
        m_ResolutionStatusText = "当前状态：正常（系统与Bodycam分辨率一致）✅";
    }
    else if (m_ResolutionState == 2) {
        m_ResolutionStatusText = "当前状态：需要修复（系统与Bodycam分辨率不一致）⚠️";
    }
    else {
        m_ResolutionStatusText = "当前状态：无法检测（未找到Bodycam配置）❌";
    }
}

void ResolutionPage::RefreshResolution() {
    LoadResolution();
    LoadBodycamResolution();
    CheckResolutionStatus();
}

// -----------------------------------------------------------------------------
// 功能逻辑响应
// -----------------------------------------------------------------------------

void ResolutionPage::FixResolution() {
    if (IsBodycamRunning()) {
        MessageBoxW(NULL, L"检测到Bodycam正在运行，请先关闭！", L"Bodycam 工具箱", MB_OK | MB_ICONWARNING);
        return;
    }

    GetSystemResolution(m_SystemWidth, m_SystemHeight);
    std::wstring localAppData = GetLocalAppDataPath();
    std::wstring iniPath = localAppData + L"\\Bodycam\\Saved\\Config\\Windows\\GameUserSettings.ini";
    std::wstring jsonPath = localAppData + L"\\Bodycam\\Saved\\SaveGames\\SystemConfig.json";

    // 修改 ini
    std::ifstream inFile(iniPath);
    if (inFile.is_open()) {
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string text = buffer.str();
        inFile.close();

        std::string strWidth = std::to_string(m_SystemWidth);
        std::string strHeight = std::to_string(m_SystemHeight);

        text = std::regex_replace(text, std::regex("ResolutionSizeX=\\d+"), "ResolutionSizeX=" + strWidth);
        text = std::regex_replace(text, std::regex("ResolutionSizeY=\\d+"), "ResolutionSizeY=" + strHeight);
        text = std::regex_replace(text, std::regex("LastUserConfirmedResolutionSizeX=\\d+"), "LastUserConfirmedResolutionSizeX=" + strWidth);
        text = std::regex_replace(text, std::regex("LastUserConfirmedResolutionSizeY=\\d+"), "LastUserConfirmedResolutionSizeY=" + strHeight);
        text = std::regex_replace(text, std::regex("DesiredScreenWidth=\\d+"), "DesiredScreenWidth=" + strWidth);
        text = std::regex_replace(text, std::regex("DesiredScreenHeight=\\d+"), "DesiredScreenHeight=" + strHeight);
        text = std::regex_replace(text, std::regex("LastUserConfirmedDesiredScreenWidth=\\d+"), "LastUserConfirmedDesiredScreenWidth=" + strWidth);
        text = std::regex_replace(text, std::regex("LastUserConfirmedDesiredScreenHeight=\\d+"), "LastUserConfirmedDesiredScreenHeight=" + strHeight);

        std::ofstream outFile(iniPath);
        outFile << text;
        outFile.close();
    }

    // 修改 json
    std::ifstream inJson(jsonPath);
    if (inJson.is_open()) {
        std::stringstream buffer;
        buffer << inJson.rdbuf();
        std::string jsonText = buffer.str();
        inJson.close();

        std::string resStr = "\"DisplayResolution\":\"" + std::to_string(m_SystemWidth) + " x " + std::to_string(m_SystemHeight) + "\"";
        jsonText = std::regex_replace(jsonText, std::regex("\"DisplayResolution\":\"\\d+\\s*x\\s*\\d+\""), resStr);

        std::ofstream outJson(jsonPath);
        outJson << jsonText;
        outJson.close();
    }

    RefreshResolution();
    MessageBoxW(NULL, L"分辨率修复成功！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}

void ResolutionPage::DestroyResolution() {
    if (IsBodycamRunning()) {
        MessageBoxW(NULL, L"检测到 Bodycam 正在运行，请先关闭！", L"Bodycam 工具箱", MB_OK | MB_ICONWARNING);
        return;
    }

    const int width = 1568;
    const int height = 680;

    std::wstring localAppData = GetLocalAppDataPath();
    std::wstring iniPath = localAppData + L"\\Bodycam\\Saved\\Config\\Windows\\GameUserSettings.ini";
    std::wstring jsonPath = localAppData + L"\\Bodycam\\Saved\\SaveGames\\SystemConfig.json";

    std::ifstream inFile(iniPath);
    if (inFile.is_open()) {
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string text = buffer.str();
        inFile.close();

        text = std::regex_replace(text, std::regex("ResolutionSizeX=\\d+"), "ResolutionSizeX=" + std::to_string(width));
        text = std::regex_replace(text, std::regex("ResolutionSizeY=\\d+"), "ResolutionSizeY=" + std::to_string(height));
        text = std::regex_replace(text, std::regex("LastUserConfirmedResolutionSizeX=\\d+"), "LastUserConfirmedResolutionSizeX=" + std::to_string(width));
        text = std::regex_replace(text, std::regex("LastUserConfirmedResolutionSizeY=\\d+"), "LastUserConfirmedResolutionSizeY=" + std::to_string(height));
        text = std::regex_replace(text, std::regex("DesiredScreenWidth=\\d+"), "DesiredScreenWidth=" + std::to_string(width));
        text = std::regex_replace(text, std::regex("DesiredScreenHeight=\\d+"), "DesiredScreenHeight=" + std::to_string(height));
        text = std::regex_replace(text, std::regex("LastUserConfirmedDesiredScreenWidth=\\d+"), "LastUserConfirmedDesiredScreenWidth=" + std::to_string(width));
        text = std::regex_replace(text, std::regex("LastUserConfirmedDesiredScreenHeight=\\d+"), "LastUserConfirmedDesiredScreenHeight=" + std::to_string(height));

        std::ofstream outFile(iniPath);
        outFile << text;
        outFile.close();
    }

    RefreshResolution();
    MessageBoxW(NULL, L" ~ ·分辨率· 已被 ·破坏· ~ ", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}

void ResolutionPage::FixBlackScreen() {
    if (IsBodycamRunning()) {
        MessageBoxW(NULL, L"当前检测到 Bodycam 正在运行。\n修改失败。\n必须关闭游戏才能修改配置。", L"Bodycam 工具箱", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring exePath = FindBodycamExe();
    if (exePath.empty()) {
        MessageBoxW(NULL, L"未找到 Bodycam.exe", L"黑屏修复", MB_OK | MB_ICONERROR);
        return;
    }

    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        const wchar_t* val = L"~ DISABLEDXMAXIMIZEDWINDOWEDMODE";
        RegSetValueExW(hKey, exePath.c_str(), 0, REG_SZ, (const BYTE*)val, (DWORD)((wcslen(val) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        MessageBoxW(NULL, L"黑屏修复成功！\n", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBoxW(NULL, L"无法打开注册表！", L"错误", MB_OK | MB_ICONERROR);
    }
}

void ResolutionPage::FixRainbowScreen() {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\VideoSettings", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        DWORD val = 0;
        RegSetValueExW(hKey, L"EnableHDRForPlayback", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }

    // 刷新 DisplaySwitch
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"open";
    sei.lpFile = L"displayswitch.exe";
    sei.lpParameters = L"/extend";
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);

    MessageBoxW(NULL, L"彩色/闪屏异常已修复", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}

// -----------------------------------------------------------------------------
// UI 绘制层 (对应 XAML 视窗界面)
// -----------------------------------------------------------------------------

void ResolutionPage::Render() {
    // 1. 在所有 Child 开始前，统一压入样式 (圆角 + 内边距)
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

    // ==========================================
    // 卡片 1：分辨率修复
    // ==========================================
    ImGui::BeginChild("ResolutionFixCard", ImVec2(0, 310), true, 0);

    // 左侧内容与右侧按钮分栏
    ImGui::Columns(2, "ResCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    // 标题
    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "分辨率修复 1");
    ImGui::Separator();
    ImGui::Spacing();

    // 动态信息展示
    ImGui::TextColored(ImVec4(0.0f, 0.66f, 0.58f, 1.0f), "%s", m_CurrentResolutionText.c_str());

    // 根据不同状态切换字体颜色
    if (m_ResolutionState == 1) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_BodycamResolutionText.c_str());
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_ResolutionStatusText.c_str());
    }
    else if (m_ResolutionState == 2) {
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", m_BodycamResolutionText.c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", m_ResolutionStatusText.c_str());
    }
    else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_BodycamResolutionText.c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_ResolutionStatusText.c_str());
    }

    ImGui::Spacing();
    ImGui::TextWrapped("1- 修复笔记本 或 多显示器环境下导致Bodycam的分辨率异常问题。");
    ImGui::TextWrapped("2- 如画面不完整等 以及 无法对画面正常点击等。");
    ImGui::TextWrapped("3- 画面下面出现黑边，画面移出屏幕外等。");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.74f, 0.56f, 0.56f, 1.0f), "--*如果Bodycam工具箱加载不出来当前分辨率可点击刷新一下按钮。");

    // 右侧按钮栏
    ImGui::NextColumn();

    // 0. 破坏分辨率按钮
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.04f, 0.03f, 1.0f));
    if (ImGui::Button("0.破坏分辨率", ImVec2(130, 30))) {
        DestroyResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 15)); // 纵向间距

    // 1. 修复分辨率按钮
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    if (ImGui::Button("1.修复分辨率", ImVec2(130, 45))) {
        FixResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 15));

    // 2. 刷新一下按钮
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.56f, 0.56f, 1.0f));
    if (ImGui::Button("2.刷新一下", ImVec2(130, 40))) {
        RefreshResolution();
    }
    ImGui::PopStyleColor();

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 2：黑屏修复
    // ==========================================
    ImGui::BeginChild("BlackScreenFixCard", ImVec2(0, 180), true, 0);

    ImGui::Columns(2, "BlackCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "黑屏修复 2");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("1- 本修复方案针对特定兼容性黑屏有效 如黑屏有声音/音乐等");
    ImGui::TextWrapped("2- 仅支持正版Bodycam版本，非官方版本 或 假入库无法保证修复效果。");
    ImGui::TextWrapped("3- 如黑屏嘟嘟的响，可能需检查Watt Toolkit或第三方加速器等");
    ImGui::TextWrapped("4- 修复过程预计耗时6 秒。");

    ImGui::NextColumn();
    ImGui::Dummy(ImVec2(0, 30));
    if (ImGui::Button("修复##BlackScreen", ImVec2(120, 40))) {
        FixBlackScreen();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // ==========================================
    // 卡片 3：五颜六色彩虹修复
    // ==========================================
    ImGui::BeginChild("RainbowFixCard", ImVec2(0, 160), true, 0);

    ImGui::Columns(2, "RainbowCols", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 170.0f);

    ImGui::TextColored(ImVec4(0.15f, 0.60f, 0.72f, 1.0f), "五颜六色彩虹修复 3");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("1- 五颜六色彩虹修复是指游戏中出现的彩虹色异常问题 或 闪屏");
    ImGui::TextWrapped("2- 按Tab键的玩家数据面板出现闪屏 或 彩色异常。");
    ImGui::TextWrapped("3- 开局时出现彩虹色异常。");

    ImGui::NextColumn();
    ImGui::Dummy(ImVec2(0, 25));
    if (ImGui::Button("修复##Rainbow", ImVec2(120, 40))) {
        FixRainbowScreen();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    // 2. 页面渲染完毕，弹出最开头的 2 个 PushStyleVar
    ImGui::PopStyleVar(2);
}