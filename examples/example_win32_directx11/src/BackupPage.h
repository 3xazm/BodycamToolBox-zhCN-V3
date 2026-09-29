#pragma once
#include "BackupModel.h"

class BackupPage {
public:
    BackupPage() = default;
    ~BackupPage() = default;

    void Render();

private:
    BackupModel m_Model;
};