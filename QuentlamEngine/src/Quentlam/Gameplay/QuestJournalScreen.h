#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Gameplay/QuestEventModule.h"
#include <string>
#include <vector>

namespace Quentlam
{
	class QuestJournalScreen : public UIScreen
	{
	public:
		QuestJournalScreen();
		~QuestJournalScreen() override = default;

		void OnShow() override;
		void OnHide() override;
		void OnTick(float deltaTime) override;
		void OnRender() override;

		bool OnMouseClick(float x, float y, int32_t button, bool pressed) override;

		enum class EQuestFilter
		{
			All,
			Active,
			Completed,
			Available
		};

		void SetFilter(EQuestFilter filter) { m_Filter = filter; }
		EQuestFilter GetFilter() const { return m_Filter; }

		void RefreshQuests();

		std::function<void(const std::string& questId)> OnQuestSelected;
		std::function<void()> OnJournalClosed;

	private:
		void RenderHeader();
		void RenderFilterTabs();
		void RenderQuestList();
		void RenderQuestDetail();

		EQuestFilter m_Filter = EQuestFilter::Active;
		std::string m_SelectedQuestId;
		float m_ListWidth = 250.0f;
		float m_HeaderHeight = 40.0f;
		float m_TabHeight = 35.0f;
		float m_DetailPadding = 15.0f;
	};
}
