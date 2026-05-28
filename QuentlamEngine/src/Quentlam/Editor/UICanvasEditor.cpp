#include "qlpch.h"
#include "UICanvasEditor.h"
#include <imgui.h>

namespace Quentlam
{
	UICanvasEditor::UICanvasEditor()
	{
	}

	void UICanvasEditor::SetScreenStack(ScreenStack* stack)
	{
		m_ScreenStack = stack;
	}

	void UICanvasEditor::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::SetNextWindowSize(ImVec2(900.0f, 600.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("UI 画布编辑器##UICanvasEditor", &m_Visible))
		{
			ImGui::End();
			return;
		}

		static bool showProperties = true;
		static bool showScreenList = true;

		if (ImGui::Button(showScreenList ? "<<" : ">>"))
			showScreenList = !showScreenList;

		ImGui::SameLine();

		if (ImGui::Button(showProperties ? ">>" : "<<"))
			showProperties = !showProperties;

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		ImGui::Text("缩放:");
		ImGui::SameLine();
		ImGui::PushItemWidth(80.0f);
		ImGui::DragFloat("##zoom", &m_CanvasZoom, 0.01f, 0.1f, 5.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("1:1"))
			m_CanvasZoom = 1.0f;
		ImGui::SameLine();
		if (ImGui::Button("Fit"))
			m_CanvasZoom = 1.0f;

		ImGui::Separator();

		if (showScreenList)
		{
			ImGui::BeginChild("ScreenList", ImVec2(180.0f, 0), true);
			RenderScreenList();
			ImGui::EndChild();
			ImGui::SameLine();
		}

		ImGui::BeginChild("CanvasArea", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		RenderCanvas();
		ImGui::EndChild();

		ImGui::SameLine();

		if (showProperties)
		{
			ImGui::BeginChild("Properties", ImVec2(220.0f, 0), true);
			RenderPropertiesPanel();
			ImGui::EndChild();
		}

		ImGui::End();
	}

	void UICanvasEditor::RenderScreenList()
	{
		ImGui::Text("Screen 列表");
		ImGui::Separator();

		if (!m_ScreenStack)
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No screen stack");
			return;
		}

		const auto& screens = m_ScreenStack->GetAll();
		for (const auto& screen : screens)
		{
			if (!screen) continue;

			bool selected = !m_SelectedScreen.expired() && m_SelectedScreen.lock() == screen;
			ImGui::PushID(screen->GetId().c_str());

			ImU32 iconCol = screen->IsShowing() ? IM_COL32(100, 200, 100, 255) : IM_COL32(100, 100, 100, 255);
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 iconMin = ImGui::GetCursorScreenPos();
			ImVec2 iconMax(iconMin.x + 8.0f, iconMin.y + 8.0f);
			drawList->AddCircleFilled(ImVec2(iconMin.x + 4.0f, iconMin.y + 4.0f), 3.0f, iconCol);

			ImGui::Indent(14.0f);
			if (ImGui::Selectable(screen->GetId().c_str(), selected, 0, ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0)))
			{
				m_SelectedScreen = screen;
				if (OnScreenSelected)
					OnScreenSelected(screen);
			}
			ImGui::Unindent(14.0f);

			ImGui::PopID();
		}
	}

	void UICanvasEditor::RenderCanvas()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasMin = ImGui::GetCursorScreenPos();
		ImVec2 canvasMax = ImVec2(canvasMin.x + ImGui::GetContentRegionAvail().x,
			canvasMin.y + ImGui::GetContentRegionAvail().y);
		float canvasW = canvasMax.x - canvasMin.x;
		float canvasH = canvasMax.y - canvasMin.y;

		drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(20, 20, 22, 255));

		float gridSize = 20.0f * m_CanvasZoom;
		ImU32 gridCol = IM_COL32(40, 40, 45, 255);
		for (float x = 0; x < canvasW; x += gridSize)
		{
			drawList->AddLine(ImVec2(canvasMin.x + x, canvasMin.y),
				ImVec2(canvasMin.x + x, canvasMax.y), gridCol);
		}
		for (float y = 0; y < canvasH; y += gridSize)
		{
			drawList->AddLine(ImVec2(canvasMin.x, canvasMin.y + y),
				ImVec2(canvasMax.x, canvasMin.y + y), gridCol);
		}

		float aspectRatio = 16.0f / 9.0f;
		float screenW = canvasW * 0.8f;
		float screenH = screenW / aspectRatio;
		if (screenH > canvasH * 0.8f)
		{
			screenH = canvasH * 0.8f;
			screenW = screenH * aspectRatio;
		}
		float screenX = canvasMin.x + (canvasW - screenW) * 0.5f + m_CanvasOffset.x;
		float screenY = canvasMin.y + (canvasH - screenH) * 0.5f + m_CanvasOffset.y;

		ImU32 borderCol = IM_COL32(80, 120, 180, 255);
		drawList->AddRect(ImVec2(screenX, screenY),
			ImVec2(screenX + screenW, screenY + screenH),
			borderCol, 2.0f);

		ImU32 headerCol = IM_COL32(60, 80, 120, 200);
		drawList->AddRectFilled(ImVec2(screenX, screenY),
			ImVec2(screenX + screenW, screenY + 28.0f), headerCol);

		float headerTextSize = ImGui::CalcTextSize("Preview Canvas").x;
		drawList->AddText(ImVec2(screenX + (screenW - headerTextSize) * 0.5f, screenY + 6.0f),
			IM_COL32(180, 200, 220, 255), "Preview Canvas");

		if (m_ScreenStack)
		{
			const auto& screens = m_ScreenStack->GetAll();
			for (const auto& screen : screens)
			{
				if (screen)
					RenderScreenOnCanvas(screen, screenX, screenY, screenW, screenH);
			}
		}

		HandleCanvasInteraction(screenX, screenY, screenW, screenH);
	}

	void UICanvasEditor::RenderScreenOnCanvas(Ref<UIScreen> screen, float canvasX, float canvasY, float canvasW, float canvasH)
	{
		if (!screen || !screen->IsShowing()) return;

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		bool isSelected = !m_SelectedScreen.expired() && m_SelectedScreen.lock() == screen;
		bool isHovered = m_HoveredScreen == screen;

		ImU32 screenBg = IM_COL32(50, 45, 35, 180);
		ImU32 screenBorder = IM_COL32(100, 80, 40, 200);
		if (isSelected)
		{
			screenBg = IM_COL32(60, 55, 40, 200);
			screenBorder = IM_COL32(200, 160, 80, 255);
		}
		else if (isHovered)
		{
			screenBg = IM_COL32(55, 50, 38, 190);
			screenBorder = IM_COL32(150, 120, 60, 220);
		}

		float w = canvasW * 0.6f;
		float h = canvasH * 0.3f;
		float x = canvasX + (canvasW - w) * 0.2f;
		float y = canvasY + canvasH * 0.35f;

		drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), screenBg, 4.0f);
		drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), screenBorder, 4.0f, 0, 2.0f);

		float labelSize = ImGui::CalcTextSize(screen->GetId().c_str()).x;
		float labelX = x + (w - labelSize) * 0.5f;
		float labelY = y + (h - 14.0f) * 0.5f;
		drawList->AddText(ImVec2(labelX, labelY),
			isSelected ? IM_COL32(255, 220, 150, 255) : IM_COL32(200, 190, 170, 255),
			screen->GetId().c_str());
	}

	void UICanvasEditor::HandleCanvasInteraction(float canvasX, float canvasY, float canvasW, float canvasH)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 mousePos = io.MousePos;
		ImVec2 mouseDelta = ImVec2(io.MouseDelta.x, io.MouseDelta.y);

		bool isOverCanvas = mousePos.x >= canvasX && mousePos.x <= canvasX + canvasW &&
			mousePos.y >= canvasY && mousePos.y <= canvasY + canvasH;

		m_HoveredScreen.reset();

		if (isOverCanvas && m_ScreenStack)
		{
			const auto& screens = m_ScreenStack->GetAll();
			for (const auto& screen : screens)
			{
				if (!screen || !screen->IsShowing()) continue;

				float w = canvasW * 0.6f;
				float h = canvasH * 0.3f;
				float x = canvasX + (canvasW - w) * 0.2f;
				float y = canvasY + canvasH * 0.35f;

				if (mousePos.x >= x && mousePos.x <= x + w &&
					mousePos.y >= y && mousePos.y <= y + h)
				{
					m_HoveredScreen = screen;

					if (ImGui::IsMouseClicked(0))
					{
						m_SelectedScreen = screen;
						if (OnScreenSelected)
							OnScreenSelected(screen);
					}
					break;
				}
			}
		}

		if (ImGui::IsMouseDown(1) && isOverCanvas)
		{
			m_CanvasOffset.x += mouseDelta.x;
			m_CanvasOffset.y += mouseDelta.y;
		}

		if (ImGui::IsMouseClicked(2))
		{
			m_CanvasOffset = { 0.0f, 0.0f };
		}
	}

	void UICanvasEditor::RenderPropertiesPanel()
	{
		ImGui::Text("属性");
		ImGui::Separator();

		Ref<UIScreen> selected = m_SelectedScreen.lock();
		if (!selected)
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "选择 Screen 进行编辑");
			return;
		}

		ImGui::Text("ID: %s", selected->GetId().c_str());

		ImGui::Spacing();
		ImGui::Text("显示状态");
		ImGui::Separator();
		bool isShowing = selected->IsShowing();
		if (ImGui::Checkbox("显示 (Showing)", &isShowing))
		{
			if (isShowing) selected->Show();
			else selected->Hide();
		}

		bool blocking = selected->IsBlocking();
		if (ImGui::Checkbox("阻塞 (Blocking)", &blocking))
			selected->SetBlocking(blocking);

		bool focusable = selected->IsFocusable();
		if (ImGui::Checkbox("可聚焦 (Focusable)", &focusable))
			selected->SetFocusable(focusable);

		ImGui::Spacing();
		ImGui::Text("输入上下文");
		ImGui::Separator();

		int ctxIndex = static_cast<int>(selected->GetInputContext());
		const char* ctxNames[] = { "Gameplay", "Menu", "Dialog", "Inventory", "Dialogue", "Fishing", "Busy", "Blocked" };
		if (ImGui::Combo("Context", &ctxIndex, ctxNames, 8))
		{
			selected->SetInputContext(static_cast<EInputContext>(ctxIndex));
		}

		ImGui::Spacing();
		ImGui::Text("层级信息");
		ImGui::Separator();

		ImGui::Text("Z-Order: %d", selected->GetWidgetTree().GetZOrder());
		bool visible = selected->GetWidgetTree().IsVisible();
		if (ImGui::Checkbox("可见 (Visible)", &visible))
			selected->GetWidgetTree().SetVisible(visible);

		bool modal = selected->GetWidgetTree().IsModal();
		if (ImGui::Checkbox("模态 (Modal)", &modal))
			selected->GetWidgetTree().SetModal(modal);

		const auto& children = selected->GetWidgetTree().GetChildren();
		ImGui::Spacing();
		ImGui::Text("子元素");
		ImGui::Separator();
		ImGui::Text("子元素数量: %zu", children.size());

		for (const auto& child : children)
		{
			if (!child) continue;
			ImGui::BulletText("%s", child->GetId().c_str());
		}
	}
}
