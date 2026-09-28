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
    (void)headerH;

    // 1. 左侧标题胶囊排版参数
    float iconSize = 28.0f * scale;      // Icon 尺寸
    float spacing = 12.0f * scale;       // Icon 与文字的间距
    float paddingX = 16.0f * scale;     // 胶囊左右内边距
    const char* titleText = "Bodycam工具箱V3.0.0";
    ImVec2 titleTextSize = ImGui::CalcTextSize(titleText);

    // 动态计算胶囊总宽度
    float contentW = (iconTexture ? (iconSize + spacing) : 0.0f) + titleTextSize.x;
    ImVec2 titleCapsuleSize(contentW + paddingX * 2.2f, 34.0f * scale);
    ImVec2 titlePos(16.0f * scale, 12.0f * scale);

    // 绘制左侧胶囊背景与边框
    drawList->AddRectFilled(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 25), 17.0f * scale);
    drawList->AddRect(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 80), 17.0f * scale);

    // 确定起点：保证 Icon 紧靠左侧
    float startX = titlePos.x + paddingX;
    float contentCenterY = titlePos.y + titleCapsuleSize.y * 0.5f;

    // A. 绘制 Icon 图标
    if (iconTexture) {
        ImVec2 iconMin(startX, contentCenterY - iconSize * 0.5f);
        ImVec2 iconMax(startX + iconSize, contentCenterY + iconSize * 0.5f);
        drawList->AddImage(iconTexture, iconMin, iconMax);
        startX += iconSize + spacing;
    }

    // B. 绘制标题文字
    drawList->AddText(
        ImVec2(startX, contentCenterY - titleTextSize.y * 0.5f),
        IM_COL32(255, 255, 255, 230),
        titleText
    );

    // 2. 中间自定义玻璃胶囊搜索框
    float searchW = 300.0f * scale;
    float searchH = 30.0f * scale; // 与左侧胶囊高度保持一致
    float searchX = (windowSize.x - searchW) * 0.5f;
    ImVec2 searchPos(searchX, 12.0f * scale);

    // 绘制与左边完全一致的玻璃背景和高亮边框
    drawList->AddRectFilled(searchPos, ImVec2(searchPos.x + searchW, searchPos.y + searchH), IM_COL32(255, 255, 255, 25), 17.0f * scale);
    drawList->AddRect(searchPos, ImVec2(searchPos.x + searchW, searchPos.y + searchH), IM_COL32(255, 255, 255, 80), 17.0f * scale);

    // 设置输入框无背景、无边框，完美嵌入自定义胶囊中
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f * scale, 7.0f * scale));

    ImGui::SetCursorPos(searchPos);
    ImGui::PushItemWidth(searchW);
    ImGui::InputTextWithHint("##Search", "搜索...", searchBuffer, searchBufferSize);
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // 3. 右侧 Mac 控制按钮
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