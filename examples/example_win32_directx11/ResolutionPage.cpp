#include "ResolutionPage.h"
#include "imgui.h"

void RenderResolutionControls(float main_scale)
{
    // 1. 分辨率比例控制
    static int render_scale = 100;
    ImGui::SetNextItemWidth(260.0f * main_scale);
    ImGui::SliderInt("渲染分辨率比例 (%)", &render_scale, 50, 200);

    ImGui::Spacing();

    // 2. 画幅比例选择
    static int aspect_ratio_idx = 0;
    const char* aspect_ratios[] = { "原生 (16:9)", "超宽屏 (21:9)", "带状影院画幅 (32:9)", "经典 (4:3)" };
    ImGui::SetNextItemWidth(260.0f * main_scale);
    ImGui::Combo("画幅比例", &aspect_ratio_idx, aspect_ratios, IM_ARRAYSIZE(aspect_ratios));

    ImGui::Spacing();
    ImGui::Spacing();

    // 3. 高级开关
    static bool enable_vsync = true;
    static bool enable_hdr = false;
    ImGui::Checkbox("开启垂直同步 (V-Sync)", &enable_vsync);
    ImGui::SameLine(0, 20.0f * main_scale);
    ImGui::Checkbox("开启 HDR 高动态范围", &enable_hdr);

    ImGui::Spacing();
    ImGui::Spacing();

    // 4. WPF 风格按钮：高彩主色调按钮
    if (ImGui::Button("保存并应用设置", ImVec2(150.0f * main_scale, 36.0f * main_scale))) {
        // 保存配置逻辑
    }
}