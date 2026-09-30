#pragma once
#include <string>
#include <vector>

struct LanguagePatchInfo {
	std::string name;
	std::string version;
	std::string author;
	std::string description;
	bool isInstalled = false;
};

class LocalizationModel {
public:
	LocalizationModel( );
	~LocalizationModel( ) = default;

	// 状态获取
	const std::string& GetGamePathText( ) const { return m_GamePathText; }
	const std::string& GetStatusMessage( ) const { return m_StatusMessage; }
	bool IsPatchInstalled( ) const { return m_IsInstalled; }
	float GetInstallProgress( ) const { return m_InstallProgress; }
	bool IsInstalling( ) const { return m_IsInstalling; }
	int GetSelectedPatchIndex( ) const { return m_SelectedPatchIndex; }

	const std::vector<LanguagePatchInfo>& GetAvailablePatches( ) const { return m_AvailablePatches; }

	// 逻辑功能
	void SetSelectedPatchIndex(int index) { m_SelectedPatchIndex = index; }
	void RefreshStatus( );
	void InstallPatch( );
	void UninstallPatch( );

private:
	std::string m_GamePathText;
	std::string m_StatusMessage;
	bool m_IsInstalled = false;
	bool m_IsInstalling = false;
	float m_InstallProgress = 0.0f;
	int m_SelectedPatchIndex = 0;

	std::vector<LanguagePatchInfo> m_AvailablePatches;

	std::wstring GetLocalAppDataPath( );
};