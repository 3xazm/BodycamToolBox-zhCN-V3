#include "ThemeSettingsCardModel.h"
#include "imgui.h"
#include "../../src/curseFiles/Win32_API/Win32_API.h"
#include "../../src/curseFiles/AppRenderer/AppRenderer.h" // 获取 g_hWnd, g_Scale

ThemeSettingsCardModel::ThemeSettingsCardModel( ) {
	m_ThemeList = {
		{ "亚克力暗黑 (默认)", AppThemeType::AcrylicDark, "优雅深色玻璃质感，配合高级雨滴与系统亚克力背景。" },
		{ "清新明亮",       AppThemeType::FreshLight,  "高亮白透水晶风，适合白天办公与清爽视角。" },
		{ "赛博霓虹",       AppThemeType::CyberNeon,   "炫彩霓虹青粉调，极致科幻与高对比度交互体验。" }
	};
}

void ThemeSettingsCardModel::SetTheme(int themeIndex, float scale) {
	if ( themeIndex < 0 || themeIndex >= static_cast< int >(m_ThemeList.size( )) ) return;
	SetTheme(static_cast< AppThemeType >(themeIndex), scale);
}

void ThemeSettingsCardModel::SetTheme(AppThemeType themeType, float scale) {
	m_CurrentTheme = themeType;
	ApplyThemeStyle(m_CurrentTheme, scale);
}

void ThemeSettingsCardModel::ApplyThemeStyle(AppThemeType themeType, float scale) {
	ImGuiStyle& style = ImGui::GetStyle( );
	ImVec4* colors = style.Colors;

	// 基础圆角与尺寸控制
	style.WindowRounding = 16.0f * scale;
	style.ChildRounding = 12.0f * scale;
	style.FrameRounding = 8.0f * scale;
	style.PopupRounding = 10.0f * scale;
	style.ScrollbarRounding = 10.0f * scale;
	style.GrabRounding = 8.0f * scale;
	style.ScrollbarSize = 6.0f * scale;

	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	switch ( themeType ) {
	case AppThemeType::AcrylicDark: { // 1. 亚克力暗黑 (默认)
		// 【关键修改】：0xCC1A1008 (ABGR 格式，高 8 位 CC 代表 Alpha，后面是暗灰色调)
		// 这样既能触发系统的毛玻璃模糊，又不会变成纯黑！
		if ( g_hWnd ) EnableAcrylic(g_hWnd, 0xCC1A1008);

		// ImGui 窗口背景设为半透明暗色
		colors[ ImGuiCol_WindowBg ] = ImVec4(0.08f, 0.10f, 0.12f, 0.20f);
		colors[ ImGuiCol_ChildBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
		colors[ ImGuiCol_Border ] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ ImGuiCol_Text ] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);

		colors[ ImGuiCol_FrameBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
		colors[ ImGuiCol_Button ] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
		break;
	}
	case AppThemeType::FreshLight: { // 2. 清新明亮
		// 传入 0：触发 AccentState = 2 纯透明模式，无毛玻璃与灰色暗影
		if ( g_hWnd ) EnableAcrylic(g_hWnd, 0);

		colors[ ImGuiCol_WindowBg ] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ ImGuiCol_ChildBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
		colors[ ImGuiCol_Border ] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
		colors[ ImGuiCol_Text ] = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);

		colors[ ImGuiCol_FrameBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ ImGuiCol_Button ] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
		break;
	}
	case AppThemeType::CyberNeon: { // 3. 赛博霓虹
		if ( g_hWnd ) EnableAcrylic(g_hWnd, 0xCC2E0B1A);

		colors[ ImGuiCol_WindowBg ] = ImVec4(0.10f, 0.05f, 0.18f, 0.30f);
		colors[ ImGuiCol_ChildBg ] = ImVec4(0.18f, 0.10f, 0.28f, 0.30f);
		colors[ ImGuiCol_Border ] = ImVec4(0.00f, 0.95f, 1.00f, 0.45f);
		colors[ ImGuiCol_Text ] = ImVec4(0.98f, 0.98f, 1.00f, 1.00f);

		colors[ ImGuiCol_FrameBg ] = ImVec4(0.25f, 0.12f, 0.38f, 0.45f);
		colors[ ImGuiCol_Button ] = ImVec4(1.00f, 0.00f, 0.55f, 0.25f);
		break;
	}
	}
}