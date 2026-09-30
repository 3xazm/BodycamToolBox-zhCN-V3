#pragma once
#include <windows.h>

// 未公开的 Win32 API SetWindowCompositionAttribute 相关枚举与结构体
typedef enum _WINDOWCOMPOSITIONATTRIB {
	WCA_ACCENT_POLICY = 19 // 用于控制窗口背景特性的属性枚举值
} WINDOWCOMPOSITIONATTRIB;

typedef struct _ACCENT_POLICY {
	int AccentState;    // 3 = Fluent Acrylic ( Win10 光感亚克力 ), 4 = Win11 亚克力 / 亚态材质
	int AccentFlags;    // 标识位
	int GradientColor;  // ABGR 格式的背景颜色与透明度
	int AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIB_DATA {
	WINDOWCOMPOSITIONATTRIB Attribute;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIB_DATA;

// 未公开 API 的函数指针原型定义
typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIB_DATA*);

// 开启窗口 Fluent 亚克力/毛玻璃透明效果
void EnableAcrylic(HWND hwnd, COLORREF colorWithAlpha);