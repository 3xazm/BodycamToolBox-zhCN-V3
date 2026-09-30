#include "ResolutionModel.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

ResolutionModel::ResolutionModel() {
    RefreshResolution();
}

bool ResolutionModel::IsBodycamRunning() {
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

void ResolutionModel::GetSystemResolution(int& width, int& height) {
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}

std::wstring ResolutionModel::GetLocalAppDataPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

std::wstring ResolutionModel::SearchFileInDir(const std::wstring& root, const std::wstring& fileName) {
    std::wstring searchPath = root + L"\\" + fileName;
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return root + L"\\" + fileName;
    }

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

std::wstring ResolutionModel::FindBodycamExe() {
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

void ResolutionModel::LoadResolution() {
    GetSystemResolution(m_SystemWidth, m_SystemHeight);
    m_CurrentResolutionText = "当前你的系统分辨率为: " + std::to_string(m_SystemWidth) + " x " + std::to_string(m_SystemHeight);
}

void ResolutionModel::LoadBodycamResolution() {
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
            m_ResolutionState = 1;
        }
        else {
            m_ResolutionState = 2;
        }
    }
    else {
        m_BodycamResolutionText = "当前你的Bodycam分辨率为: Null";
        m_ResolutionState = 0;
    }
}

void ResolutionModel::CheckResolutionStatus() {
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

void ResolutionModel::RefreshResolution() {
    LoadResolution();
    LoadBodycamResolution();
    CheckResolutionStatus();
}

void ResolutionModel::FixResolution() {
    if (IsBodycamRunning()) {
        MessageBoxW(NULL, L"检测到Bodycam正在运行，请先关闭！", L"Bodycam 工具箱", MB_OK | MB_ICONWARNING);
        return;
    }

    GetSystemResolution(m_SystemWidth, m_SystemHeight);
    std::wstring localAppData = GetLocalAppDataPath();
    std::wstring iniPath = localAppData + L"\\Bodycam\\Saved\\Config\\Windows\\GameUserSettings.ini";
    std::wstring jsonPath = localAppData + L"\\Bodycam\\Saved\\SaveGames\\SystemConfig.json";

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

void ResolutionModel::DestroyResolution() {
    if (IsBodycamRunning()) {
        MessageBoxW(NULL, L"检测到 Bodycam 正在运行，请先关闭！", L"Bodycam 工具箱", MB_OK | MB_ICONWARNING);
        return;
    }

    const int width = 1568;
    const int height = 680;

    std::wstring localAppData = GetLocalAppDataPath();
    std::wstring iniPath = localAppData + L"\\Bodycam\\Saved\\Config\\Windows\\GameUserSettings.ini";

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

void ResolutionModel::FixBlackScreen() {
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

void ResolutionModel::FixRainbowScreen() {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\VideoSettings", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        DWORD val = 0;
        RegSetValueExW(hKey, L"EnableHDRForPlayback", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"open";
    sei.lpFile = L"displayswitch.exe";
    sei.lpParameters = L"/extend";
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);

    MessageBoxW(NULL, L"彩色/闪屏异常已修复", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}