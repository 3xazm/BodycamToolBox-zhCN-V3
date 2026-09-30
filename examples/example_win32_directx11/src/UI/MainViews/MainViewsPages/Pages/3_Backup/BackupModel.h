#pragma once
#include <string>
#include <vector>

struct BackupItem {
    std::string name;
    std::string date;
    std::string size;
};

class BackupModel {
public:
    BackupModel();
    ~BackupModel() = default;

    // 状态与数据获取
    const std::string& GetSavePathText() const { return m_SavePathText; }
    const std::string& GetStatusMessage() const { return m_StatusMessage; }
    const std::vector<BackupItem>& GetBackupList() const { return m_BackupList; }

    // 核心业务功能
    void RefreshSavePath();
    void CreateBackup();
    void RestoreBackup(int index);
    void DeleteBackup(int index);

private:
    std::string m_SavePathText;
    std::string m_StatusMessage;
    std::vector<BackupItem> m_BackupList;

    std::wstring GetLocalAppDataPath();
};