#pragma once
#include "SettingModel.h"

class SettingPage {
public:
    SettingPage() = default;
    ~SettingPage() = default;

    // 渲染 Setting 页面
    void Render();

private:
    SettingModel m_Model;
};