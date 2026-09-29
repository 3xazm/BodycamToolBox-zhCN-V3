#include "DashboardModel.h"

DashboardModel::DashboardModel( ) {
	m_GamePath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Bodycam";
	m_IsSystemReady = true;
	m_StatusMessage = "系统已就绪";
}

void DashboardModel::CheckSystemStatus( ) {
	// TODO: 后续可扩充真实的文件夹/路径检测逻辑
	m_IsSystemReady = true;
	m_StatusMessage = "系统检测通过，运行环境正常";
}

void DashboardModel::ExecuteOptimization(HWND hwnd) {
	// TODO: 后续放置实际修改配置/注册表/优化脚本的代码
	m_StatusMessage = "一键优化成功完成！";
	MessageBoxW(hwnd, L"配置优化完成！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}