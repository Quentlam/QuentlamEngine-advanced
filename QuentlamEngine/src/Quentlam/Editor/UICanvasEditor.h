#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/UI/UIGameModule.h"
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace Quentlam
{
	class UICanvasEditor
	{
	public:
		UICanvasEditor();
		~UICanvasEditor() = default;

		void SetScreenStack(ScreenStack* stack);
		void OnImGuiRender();

		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }

		void SetSelectedScreen(Ref<UIScreen> screen) { m_SelectedScreen = screen; }
		Ref<UIScreen> GetSelectedScreen() const { return m_SelectedScreen.lock(); }

		std::function<void(Ref<UIScreen>)> OnScreenSelected;

	private:
		void RenderCanvas();
		void RenderPropertiesPanel();
		void RenderScreenList();
		void RenderScreenOnCanvas(Ref<UIScreen> screen, float canvasX, float canvasY, float canvasW, float canvasH);
		void HandleCanvasInteraction(float canvasX, float canvasY, float canvasW, float canvasH);

		ScreenStack* m_ScreenStack = nullptr;
		bool m_Visible = false;
		std::weak_ptr<UIScreen> m_SelectedScreen;
		glm::vec2 m_CanvasOffset = { 0.0f, 0.0f };
		float m_CanvasZoom = 1.0f;
		bool m_IsDraggingCanvas = false;
		glm::vec2 m_LastMousePos = { 0.0f, 0.0f };

		Ref<UIScreen> m_HoveredScreen;
		glm::vec2 m_WidgetDragStart = { 0.0f, 0.0f };
		glm::vec2 m_WidgetOffset = { 0.0f, 0.0f };
		bool m_IsDraggingWidget = false;
	};
}
