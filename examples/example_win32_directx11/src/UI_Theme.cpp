#include "UI_Theme.h"
#include "UI\Controls\UI_Controls.h"
#include <cmath>
#include <algorithm>

// 初始化与设置全局 ImGui 玻璃质感主题配色
void SetupAppleGlassTheme(float scale) {
	ImGuiStyle& style = ImGui::GetStyle( );

	// 1. 全局圆角与尺寸控制
	style.WindowRounding = 16.0f * scale; // 主窗口圆角
	style.ChildRounding = 12.0f * scale; // 内容卡片/子面板圆角
	style.FrameRounding = 8.0f * scale; // 按钮及输入框圆角
	style.PopupRounding = 10.0f * scale; // 弹窗圆角
	style.ScrollbarRounding = 10.0f * scale; // 滚动条圆角
	style.GrabRounding = 8.0f * scale; // 滑块圆角
	style.ScrollbarSize = 6.0f * scale; // 细长流线型滚动条宽度

	// 2. 边框细微度调整
	style.WindowBorderSize = 0.0f;           // 主窗口无重外框
	style.ChildBorderSize = 1.0f;           // 子面板开启 1px 细边框
	style.FrameBorderSize = 1.0f;           // 控件与按钮开启 1px 细边框

	ImVec4* colors = style.Colors;

	// 3. 基础面板与文本调色
	colors[ ImGuiCol_WindowBg ] = ImVec4(0.08f, 0.10f, 0.12f, 0.30f); // 极淡绿黑背景（配合 Windows 模糊）
	colors[ ImGuiCol_ChildBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.08f); // 玻璃卡片半透明背景
	colors[ ImGuiCol_Border ] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f); // 晶透微亮白色边框
	colors[ ImGuiCol_Text ] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f); // 预设冷白文本

	// 4. 输入框/选择框 (Frame) 配色
	colors[ ImGuiCol_FrameBg ] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
	colors[ ImGuiCol_FrameBgHovered ] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
	colors[ ImGuiCol_FrameBgActive ] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);

	// 5. 交互按钮 (Button) 玻璃调性配色
	colors[ ImGuiCol_Button ] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f); // 默认半透明白（取代默认亮蓝）
	colors[ ImGuiCol_ButtonHovered ] = ImVec4(1.00f, 1.00f, 1.00f, 0.22f); // 悬停微亮
	colors[ ImGuiCol_ButtonActive ] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f); // 点击高亮

	// 6. 极简滚动条 (Scrollbar) 配色
	colors[ ImGuiCol_ScrollbarBg ] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // 轨道全透明
	colors[ ImGuiCol_ScrollbarGrab ] = ImVec4(1.00f, 1.00f, 1.00f, 0.15f); // 默认微亮滑块
	colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f); // 悬停加亮
	colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f); // 拖拽高亮
}

// 渲染侧边栏液态胶囊、动态波浪轮廓与选中提示条
void RenderLiquidCapsule(
	ImDrawList* drawList,
	LiquidAnimationState& state,
	int currentTab,
	const ImVec2& sidebarPos,
	float optItemW,
	float optItemH,
	const ImVec2 optPositions[ ],
	float scale,
	float deltaTime
) {
	// 1. 计算当前选中 Tab 项的目标 Y 坐标
	float targetY = optPositions[ currentTab ].y - optItemH * 0.5f;

	// 2. 物理弹性平滑跟随与水痕拖尾逻辑
	if ( state.isFirstFrame ) {
		state.currentY = targetY;
		state.lastY = targetY;
		state.isFirstFrame = false;
	}
	else {
		float dist = targetY - state.currentY;
		float stiffness = (dist > 0.0f) ? 200.0f : 230.0f;
		float damping = (std::abs(dist) < 12.0f) ? 6.0f : 12.0f;

		float force = dist * stiffness;
		state.velY += force * deltaTime;
		state.velY -= state.velY * damping * deltaTime;
		state.currentY += state.velY * deltaTime;

		// 计算高速移动时的形变拉伸系数
		float targetStretch = (std::abs(state.velY) / 500.0f);
		targetStretch = ( std::min ) (targetStretch, 0.5f);
		state.stretch = CustomLerp(state.stretch, targetStretch, deltaTime * 18.0f);

		// 产生蒸发拖尾颗粒
		if ( std::abs(state.currentY - state.lastY) > 4.0f * scale ) {
			TrailSegment seg;
			seg.y = (state.currentY + state.lastY) * 0.5f + optItemH * 0.5f;
			seg.alpha = 0.45f;
			seg.width = (optItemW - 30.0f * scale) * (1.0f - state.stretch * 0.2f);
			seg.height = std::abs(state.currentY - state.lastY) + 6.0f * scale;
			state.trails.push_back(seg);
			state.lastY = state.currentY;
		}
	}

	// 3. 渲染并更新拖尾水痕的渐隐逻辑
	for ( auto it = state.trails.begin( ); it != state.trails.end( ); ) {
		it->alpha -= deltaTime * 1.2f;

		if ( it->alpha <= 0.0f ) {
			it = state.trails.erase(it);
		}
		else {
			float trailCenterX = sidebarPos.x + 10.0f * scale + optItemW * 0.5f;
			ImVec2 tMin(trailCenterX - it->width * 0.5f, it->y - it->height * 0.5f);
			ImVec2 tMax(trailCenterX + it->width * 0.5f, it->y + it->height * 0.5f);

			int fillAlpha = static_cast< int >(it->alpha * 90.0f);
			int borderAlpha = static_cast< int >(it->alpha * 140.0f);

			drawList->AddRectFilled(tMin, tMax, IM_COL32(255, 255, 255, fillAlpha), 8.0f * scale);
			drawList->AddRect(tMin, tMax, IM_COL32(255, 255, 255, borderAlpha), 0, 1.0f);
			++it;
		}
	}

	// 4. 鼠标悬停检测与流体扰动权重计算
	ImVec2 liquidMin(sidebarPos.x + 10.0f * scale, state.currentY);
	ImVec2 liquidMax(liquidMin.x + optItemW, liquidMin.y + optItemH);

	ImVec2 mousePos = ImGui::GetMousePos( );
	ImVec2 selectedOptMin(sidebarPos.x + 10.0f * scale, optPositions[ currentTab ].y - optItemH * 0.5f);
	ImVec2 selectedOptMax(selectedOptMin.x + optItemW, selectedOptMin.y + optItemH);

	bool isHoveringLiquid = (mousePos.x >= selectedOptMin.x && mousePos.x <= selectedOptMax.x &&
		mousePos.y >= selectedOptMin.y && mousePos.y <= selectedOptMax.y);

	float targetWeight = isHoveringLiquid ? 2.0f : 0.0f;
	state.fluidWeight = CustomLerp(state.fluidWeight, targetWeight, deltaTime * 4.0f);

	// 5. 采样多项式与正弦波计算有机液态轮廓顶点
	const int numSegments = 64;
	ImVec2 wavePoints[ 64 ];
	float time = static_cast< float >(ImGui::GetTime( )) * 2.8f;

	ImVec2 center(liquidMin.x + optItemW * 0.5f, liquidMin.y + optItemH * 0.5f);
	float rx = optItemW * 0.49f;
	float ry = optItemH * 0.49f;

	float stretchY = 1.0f + state.stretch * 0.45f;
	float stretchX = 1.0f - state.stretch * 0.20f;

	for ( int i = 0; i < numSegments; ++i ) {
		float a = (static_cast< float >(i) / static_cast< float >(numSegments)) * 2.0f * 3.14159265f;

		float wave1 = std::sin(a * 2.0f + time) * 2.0f;
		float wave2 = std::cos(a * 3.0f - time * 0.8f) * 1.2f;

		float sinA = std::sin(a);
		float cosA = std::cos(a);

		float moveDir = (state.velY >= 0.0f) ? 1.0f : -1.0f;

		float dropletAsymmetry = (sinA * moveDir < 0.0f) ? (1.0f - std::abs(sinA) * 0.38f * state.stretch)
			: (1.0f + std::abs(sinA) * 0.18f * state.stretch);

		float organicOffset = (wave1 + wave2) * state.fluidWeight;

		float power = CustomLerp(8.5f, 6.2f, state.fluidWeight);
		float pX = std::pow(std::abs(cosA), 2.0f / power) * (cosA >= 0 ? 1.0f : -1.0f);
		float pY = std::pow(std::abs(sinA), 2.0f / power) * (sinA >= 0 ? 1.0f : -1.0f);

		wavePoints[ i ] = ImVec2(
			center.x + (pX * rx * stretchX * 0.98f) + cosA * organicOffset,
			center.y + (pY * ry * stretchY * dropletAsymmetry * 0.85f) + sinA * organicOffset
		);
	}

	float motionAlphaExtra = ( std::min ) (state.stretch * 90.0f, 50.0f);
	int bgAlpha = static_cast< int >(CustomLerp(35.0f, 55.0f, state.fluidWeight) + motionAlphaExtra);
	int borderAlpha = static_cast< int >(CustomLerp(90.0f, 220.0f, state.fluidWeight) + motionAlphaExtra);

	drawList->AddConvexPolyFilled(wavePoints, numSegments, IM_COL32(255, 255, 255, bgAlpha));
	drawList->AddPolyline(wavePoints, numSegments, IM_COL32(255, 255, 255, borderAlpha), ImDrawFlags_Closed, 1.5f);

	// 6. 渲染最左侧蓝色指示条（含悬停呼吸缩放动画）
	float blueOffsetY = 0.0f;
	float blueHeightExpand = 0.0f;
	int blueAlpha = 230;

	if ( isHoveringLiquid ) {
		float breathTime = static_cast< float >(ImGui::GetTime( )) * 5.0f;
		blueOffsetY = std::sin(breathTime) * 2.5f * scale;
		blueHeightExpand = std::cos(breathTime * 0.8f) * 1.5f * scale;
		blueAlpha = static_cast< int >(225.0f + std::sin(breathTime * 0.5f) * 30.0f);
	}

	// 靠紧侧边栏最左边缘定位 (Left = 3px, Width = 4px)
	ImVec2 barMin(
		sidebarPos.x + 3.0f * scale,
		liquidMin.y + (8.0f * scale) + blueOffsetY - blueHeightExpand
	);
	ImVec2 barMax(
		sidebarPos.x + 7.0f * scale,
		liquidMax.y - (8.0f * scale) + blueOffsetY + blueHeightExpand
	);

	drawList->AddRectFilled(
		barMin, barMax,
		IM_COL32(0, 122, 255, blueAlpha), 2.0f * scale
	);
}