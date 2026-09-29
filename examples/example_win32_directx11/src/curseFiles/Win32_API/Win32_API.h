#pragma once
#include <windows.h>

// Win32 API 模糊透明支撑结构体定义
typedef enum _WINDOWCOMPOSITIONATTRIB { WCA_ACCENT_POLICY = 19 } WINDOWCOMPOSITIONATTRIB;

typedef struct _ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIB_DATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIB_DATA;

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIB_DATA*);

// 函数声明
void EnableAcrylic(HWND hwnd, COLORREF colorWithAlpha);