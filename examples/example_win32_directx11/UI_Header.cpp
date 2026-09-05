#include "UI_Header.h"
#include "UI_Controls.h"

void RenderHeader(
    HWND hwnd,
    ImDrawList* drawList,
    const ImVec2& windowSize,
    float scale,
    float headerH,
    char* searchBuffer,
    size_t searchBufferSize,
    ImTextureID iconTexture
) {
    (void)headerH; // 显式标记未引用参数，彻底消除 C4100 警告

    // 1. 左侧标题胶囊
    ImVec2 titleCapsuleSize(220.0f * scale, 34.0f * scale); // 加长一点胶囊容纳 Icon
    ImVec2 titlePos(16.0f * scale, 12.0f * scale);
    drawList->AddRectFilled(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 25), 17.0f * scale);
    drawList->AddRect(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 80), 17.0f * scale);

    // 计算排版：Icon 大小与间距
    float iconSize = 20.0f * scale;
    float spacing = 8.0f * scale;
    const char* titleText = "Bodycam工具箱V3";
    ImVec2 titleTextSize = ImGui::CalcTextSize(titleText);

    // 计算整体 (Icon + 间距 + 文字) 的总宽度，实现胶囊居中
    float totalContentW = (iconTexture ? (iconSize + spacing) : 0.0f) + titleTextSize.x;
    float startX = titlePos.x + (titleCapsuleSize.x - totalContentW) * 0.5f;
    float contentCenterY = titlePos.y + titleCapsuleSize.y * 0.5f;

    // A. 绘制 Icon 图标
    if (iconTexture) {
        ImVec2 iconMin(startX, contentCenterY - iconSize * 0.5f);
        ImVec2 iconMax(startX + iconSize, contentCenterY + iconSize * 0.5f);
        drawList->AddImage(iconTexture, iconMin, iconMax);
        startX += iconSize + spacing; // 绘制完 Icon 后将 X 坐标右移
    }

    // B. 绘制标题文字
    drawList->AddText(
        ImVec2(startX, contentCenterY - titleTextSize.y * 0.5f),
        IM_COL32(255, 255, 255, 230),
        titleText
    );

    // 2. 中间搜索框
    float searchW = 300.0f * scale;
    float searchX = (windowSize.x - searchW) * 0.5f;
    ImGui::SetCursorPos(ImVec2(searchX, 12.0f * scale));
    ImGui::PushItemWidth(searchW);
    ImGui::InputTextWithHint("##Search", "搜索...", searchBuffer, searchBufferSize);
    ImGui::PopItemWidth();

    // 3. 标题栏区域已由 WndProc 中的 WM_NCHITTEST (HTCAPTION) 原生支持
    // 此处无需任何代码，系统会自动处理标题栏拖拽与双击最大化

    // 4. 右侧 Mac 控制按钮
    float circleR = 13.0f * scale;
    float btnY = 24.0f * scale;
    float rightBaseX = windowSize.x - 24.0f * scale;
    float btnSpacing = 30.0f * scale;

    bool isMaximized = ::IsZoomed(hwnd);

    if (DrawMacCircleButton("CloseBtn", ImVec2(rightBaseX, btnY), circleR, MAC_BTN_CLOSE)) {
        ::PostQuitMessage(0);
    }
    if (DrawMacCircleButton("MaxBtn", ImVec2(rightBaseX - btnSpacing, btnY), circleR, MAC_BTN_MAXIMIZE, isMaximized)) {
        if (isMaximized) ::SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        else ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
    if (DrawMacCircleButton("MinBtn", ImVec2(rightBaseX - btnSpacing * 2.0f, btnY), circleR, MAC_BTN_MINIMIZE)) {
        ::SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }
}