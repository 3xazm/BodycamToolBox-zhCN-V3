#include "LocalizationModel.h"
#include <windows.h>
#include <shlobj.h>

LocalizationModel::LocalizationModel( ) {
	// 模拟可用的汉化补丁版本列表
	m_AvailablePatches.push_back({
		"Bodycam 社区精校汉化包",
		"v2.1 (最新版)",
		"汉化组核心成员",
		"全面覆盖游戏菜单、UI、武器名称及提示文本，支持在线联机文本。",
		false
		});
	m_AvailablePatches.push_back({
		"Bodycam 经典完整汉化包",
		"v1.8 (稳定版)",
		"Bodycam 玩家社区",
		"兼容老版本游戏，修复部分字体模糊问题。",
		false
		});

	RefreshStatus( );
}

std::wstring LocalizationModel::GetLocalAppDataPath( ) {
	wchar_t path[ MAX_PATH ];
	if ( SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)) ) {
		return std::wstring(path);
	}
	return L"";
}

void LocalizationModel::RefreshStatus( ) {
	std::wstring path = GetLocalAppDataPath( ) + L"\\Bodycam\\Saved\\Paks";
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &path[ 0 ], ( int ) path.size( ), NULL, 0, NULL, NULL);
	std::string strPath(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &path[ 0 ], ( int ) path.size( ), &strPath[ 0 ], size_needed, NULL, NULL);

	m_GamePathText = "游戏 Pak 目录: " + strPath;
	m_StatusMessage = m_IsInstalled ? "已安装汉化补丁 ✅" : "未安装汉化补丁 ❌";
}

void LocalizationModel::InstallPatch( ) {
	m_IsInstalling = true;
	m_InstallProgress = 1.0f; // 演示设为完成状态
	m_IsInstalled = true;
	m_IsInstalling = false;

	RefreshStatus( );
	MessageBoxW(NULL, L"汉化补丁安装成功！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}

void LocalizationModel::UninstallPatch( ) {
	m_IsInstalled = false;
	m_InstallProgress = 0.0f;

	RefreshStatus( );
	MessageBoxW(NULL, L"已还原为原版语言！", L"Bodycam 工具箱", MB_OK | MB_ICONINFORMATION);
}