#include "Win32_API.h"

// 通过动态加载 user32.dll 调用未公开的 SetWindowCompositionAttribute 函数实现毛玻璃背景
void EnableAcrylic(HWND hwnd, COLORREF colorWithAlpha) {
	HMODULE hUser = GetModuleHandleA("user32.dll");
	if ( hUser ) {
		pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
			( pfnSetWindowCompositionAttribute ) GetProcAddress(hUser, "SetWindowCompositionAttribute");
		if ( SetWindowCompositionAttribute ) {
			// AccentState = 4 表示使用系统级模糊效果 (Acrylic/Blur)
			ACCENT_POLICY policy = { 4, 0, ( int ) colorWithAlpha, 0 };
			WINDOWCOMPOSITIONATTRIB_DATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
			SetWindowCompositionAttribute(hwnd, &data);
		}
	}
}