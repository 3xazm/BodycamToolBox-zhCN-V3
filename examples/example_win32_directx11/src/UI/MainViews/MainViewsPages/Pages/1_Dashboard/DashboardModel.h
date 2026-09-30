#pragma once
#include <string>
#include <windows.h>

class DashboardModel {
public:
	DashboardModel( );
	~DashboardModel( ) = default;

	// 状态与数据获取
	const std::string& GetGamePath( ) const { return m_GamePath; }
	const std::string& GetStatusMessage( ) const { return m_StatusMessage; }
	bool IsSystemReady( ) const { return m_IsSystemReady; }

	// 设置与业务逻辑
	void SetGamePath(const std::string& path) { m_GamePath = path; }
	void CheckSystemStatus( );
	void ExecuteOptimization(HWND hwnd);

private:
	std::string m_GamePath;
	std::string m_StatusMessage;
	bool m_IsSystemReady;
};