#include "BackupModel.h"
#include <windows.h>
#include <shlobj.h>

BackupModel::BackupModel() {
    RefreshSavePath();
    // 插入演示数据
    m_BackupList.push_back({ "Bodycam_AutoSave_20260920.zip", "2026-09-20 14:30", "12.4 MB" });
    m_BackupList.push_back({ "Bodycam_Manual_20260925.zip", "2026-09-25 20:15", "12.8 MB" });
}

std::wstring BackupModel::GetLocalAppDataPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

void BackupModel::RefreshSavePath() {
    std::wstring path = GetLocalAppDataPath() + L"\\Bodycam\\Saved\\SaveGames";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), NULL, 0, NULL, NULL);
    std::string strPath(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), &strPath[0], size_needed, NULL, NULL);

    m_SavePathText = "当前游戏存档路径: " + strPath;
    m_StatusMessage = "就绪";
}

void BackupModel::CreateBackup() {
    // TODO: 后续在此处实现真实的 ZIP 压缩或文件复制逻辑
    m_BackupList.push_back({ "Bodycam_Backup_New.zip", "2026-09-28 22:40", "13.1 MB" });
    m_StatusMessage = "存档备份成功！";
    MessageBoxW(NULL, L"存档备份成功！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}

void BackupModel::RestoreBackup(int index) {
    if (index >= 0 && index < (int)m_BackupList.size()) {
        // TODO: 后续在此处实现解压覆盖逻辑
        m_StatusMessage = "已还原存档: " + m_BackupList[index].name;
        MessageBoxW(NULL, L"存档还原成功！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
    }
}

void BackupModel::DeleteBackup(int index) {
    if (index >= 0 && index < (int)m_BackupList.size()) {
        m_BackupList.erase(m_BackupList.begin() + index);
        m_StatusMessage = "备份已删除";
    }
}