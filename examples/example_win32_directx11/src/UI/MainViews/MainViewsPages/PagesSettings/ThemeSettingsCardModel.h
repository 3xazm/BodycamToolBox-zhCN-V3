#pragma once
#include <string>
#include <vector>

// 主题枚举类型
enum class AppThemeType {
	AcrylicDark = 0, // 亚克力暗黑 (默认)
	FreshLight = 1, // 清新明亮
	CyberNeon = 2  // 赛博霓虹
};

// 单个主题数据配置
struct ThemeItem {
	std::string name;
	AppThemeType type;
	std::string description;
};

class ThemeSettingsCardModel {
public:
	ThemeSettingsCardModel( );
	~ThemeSettingsCardModel( ) = default;

	// 获取主题列表
	const std::vector<ThemeItem>& GetThemeList( ) const { return m_ThemeList; }

	// 当前选中的主题索引/类型
	int GetCurrentThemeIndex( ) const { return static_cast< int >(m_CurrentTheme); }
	AppThemeType GetCurrentTheme( ) const { return m_CurrentTheme; }

	// 设置并即时应用主题
	void SetTheme(int themeIndex, float scale = 1.0f);
	void SetTheme(AppThemeType themeType, float scale = 1.0f);

	// 真正的 ImGui & D3D 主题样式应用函数
	static void ApplyThemeStyle(AppThemeType themeType, float scale);

private:
	AppThemeType m_CurrentTheme = AppThemeType::AcrylicDark;
	std::vector<ThemeItem> m_ThemeList;
};