#pragma once
#include <windows.h>
#include "imgui.h"

// 渲染顶部 Header 区域（标题胶囊、搜索输入框、窗口拖拽区与 Mac 控制按钮）
void RenderHeader(
    HWND hwnd,
    ImDrawList* drawList,
    const ImVec2& windowSize,
    float scale,
    float headerH,
    char* searchBuffer,
    size_t searchBufferSize
);