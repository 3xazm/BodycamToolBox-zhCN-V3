#include "UI_Header.h"
#include "UI_Controls.h"

void RenderHeader(
    HWND hwnd,
    ImDrawList* drawList,
    const ImVec2& windowSize,
    float scale,
    float headerH,
    char* searchBuffer,
    size_t searchBufferSize
) {
    // 1. 左侧标题胶囊
    ImVec2 titleCapsuleSize(200.0f * scale, 34.0f * scale);
    ImVec2 titlePos(16.0f * scale, 12.0f * scale);
    drawList->AddRectFilled(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 25), 17.0f * scale);
    drawList->AddRect(titlePos, ImVec2(titlePos.x + titleCapsuleSize.x, titlePos.y + titleCapsuleSize.y), IM_COL32(255, 255, 255, 80), 17.0f * scale);

    ImVec2 titleTextSize = ImGui::CalcTextSize("Bodycam工具箱V3");
    drawList->AddText(ImVec2(titlePos.x + (titleCapsuleSize.x - titleTextSize.x) * 0.5f, titlePos.y + (titleCapsuleSize.y - titleTextSize.y) * 0.5f), IM_COL32(255, 255, 255, 230), "Bodycam工具箱V3");

    // 2. 中间搜索框
    float searchW = 300.0f * scale;
    float searchX = (windowSize.x - searchW) * 0.5f;
    ImGui::SetCursorPos(ImVec2(searchX, 12.0f * scale));
    ImGui::PushItemWidth(searchW);
    ImGui::InputTextWithHint("##Search", "搜索...", searchBuffer, searchBufferSize);
    ImGui::PopItemWidth();

    // 3. 标题栏无边框拖拽响应区
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##TitleDrag", ImVec2(windowSize.x - 120.0f * scale, headerH));
    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ::ReleaseCapture();
        ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

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