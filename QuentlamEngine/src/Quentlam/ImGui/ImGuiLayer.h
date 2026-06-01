#pragma once
#include "qlpch.h"
#include "../Core/Layer.h"


#include "Quentlam/Events/ApplicationEvent.h"
#include "Quentlam/Events/KeyEvent.h"
#include "Quentlam/Events/MouseEvent.h"


namespace Quentlam
{

	class QUENTLAM_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach()override;
		void OnDetach()override;
		void OnEvent(Event& e)override;
		void Begin();
		void End();


		void BlockEvents(bool block) { m_BlockEvents = block; }
		void SetGameModeActive(bool active) { m_GameModeActive = active; }
		bool IsGameModeActive() const { return m_GameModeActive; }
	private:
		bool m_BlockEvents = false;
		float m_Time = 0.0f;
		bool m_GameModeActive = false;

	};

}