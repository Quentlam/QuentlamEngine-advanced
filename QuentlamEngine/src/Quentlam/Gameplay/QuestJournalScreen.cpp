#include "qlpch.h"
#include "Quentlam/Gameplay/QuestJournalScreen.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Core/Application.h"
#include <imgui.h>

namespace Quentlam
{
	QuestJournalScreen::QuestJournalScreen()
		: UIScreen("QuestJournal")
	{
		SetBlocking(true);
		SetFocusable(true);
	}

	void QuestJournalScreen::OnShow()
	{
		RefreshQuests();
	}

	void QuestJournalScreen::OnHide()
	{
		if (OnJournalClosed)
			OnJournalClosed();
	}

	void QuestJournalScreen::OnTick(float)
	{
	}

	void QuestJournalScreen::RefreshQuests()
	{
	}

	void QuestJournalScreen::OnRender()
	{
		if (ImGui::GetCurrentContext() == nullptr) return;

		float screenW = static_cast<float>(Application::Get().GetWindow().GetWidth());
		float screenH = static_cast<float>(Application::Get().GetWindow().GetHeight());
		float winW = 700.0f;
		float winH = 500.0f;
		float winX = (screenW - winW) * 0.5f;
		float winY = (screenH - winH) * 0.5f;

		ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));
		if (ImGui::Begin("##QuestJournalWindow", nullptr, flags))
		{
			RenderHeader();
			ImGui::Separator();
			RenderFilterTabs();
			ImGui::Spacing();

			float availW = ImGui::GetWindowWidth();
			float availH = ImGui::GetWindowHeight() - m_HeaderHeight - m_TabHeight - 50.0f;

			ImGui::BeginChild("##QuestList", ImVec2(m_ListWidth, availH), true);
			RenderQuestList();
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("##QuestDetail", ImVec2(availW - m_ListWidth - 10.0f, availH), true);
			RenderQuestDetail();
			ImGui::EndChild();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	void QuestJournalScreen::RenderHeader()
	{
		ImGui::SetCursorPosY(8.0f);
		ImGui::SetCursorPosX(15.0f);

		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "Quest Journal");
		ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);

		if (ImGui::Button("X", ImVec2(20.0f, 20.0f)))
		{
			Hide();
		}
	}

	void QuestJournalScreen::RenderFilterTabs()
	{
		const char* tabs[] = { "Active", "Available", "Completed", "All" };
		int activeTab = static_cast<int>(m_Filter);

		ImGui::SetCursorPosX(15.0f);
		for (int i = 0; i < 4; ++i)
		{
			bool selected = (activeTab == i);
			ImGui::PushID(i);
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.25f, 0.15f, 1.0f));
			else
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));

			if (ImGui::Button(tabs[i], ImVec2(80.0f, 28.0f)))
			{
				m_Filter = static_cast<EQuestFilter>(i);
			}
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PopID();
		}
	}

	void QuestJournalScreen::RenderQuestList()
	{
		auto& questState = QuestEventModule::Get().GetQuestState();
		std::vector<const Quest*> filteredQuests;

		const auto& allQuests = questState.GetAllQuests();
		for (const auto& [id, quest] : allQuests)
		{
			EQuestState qState = quest.GetState();
			bool include = false;
			switch (m_Filter)
			{
			case EQuestFilter::Active:    include = (qState == EQuestState::Active || qState == EQuestState::Accepted); break;
			case EQuestFilter::Available:  include = (qState == EQuestState::Hidden); break;
			case EQuestFilter::Completed:  include = (qState == EQuestState::Complete); break;
			case EQuestFilter::All:       include = true; break;
			}
			if (include)
				filteredQuests.push_back(&quest);
		}

		if (filteredQuests.empty())
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No quests found");
			return;
		}

		for (const Quest* quest : filteredQuests)
		{
			bool selected = (quest->GetId() == m_SelectedQuestId);

			ImVec4 rowColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
			if (selected)
				rowColor = ImVec4(0.3f, 0.25f, 0.15f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Header, rowColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.2f, 0.15f, 1.0f));

			std::string label = "##quest_" + quest->GetId();
			if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(m_ListWidth - 20.0f, 0.0f)))
			{
				m_SelectedQuestId = quest->GetId();
				if (OnQuestSelected)
					OnQuestSelected(quest->GetId());
			}
			ImGui::PopStyleColor(2);

			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);

			const char* typeStr = "?";
			switch (quest->GetType())
			{
			case EQuestType::Main:    typeStr = "[M]"; break;
			case EQuestType::Side:    typeStr = "[S]"; break;
			case EQuestType::Story:   typeStr = "[ST]"; break;
			case EQuestType::Monster: typeStr = "[K]"; break;
			default:                  typeStr = "[Q]"; break;
			}

			ImVec4 typeColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
			if (quest->GetType() == EQuestType::Main) typeColor = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
			else if (quest->GetType() == EQuestType::Side) typeColor = ImVec4(0.4f, 0.7f, 0.9f, 1.0f);

			ImGui::TextColored(typeColor, "%s", typeStr);
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", quest->GetTitle().c_str());
		}
	}

	void QuestJournalScreen::RenderQuestDetail()
	{
		if (m_SelectedQuestId.empty())
		{
			ImGui::SetCursorPosX(m_DetailPadding);
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a quest to view details");
			return;
		}

		auto* quest = QuestEventModule::Get().GetQuestState().GetQuest(m_SelectedQuestId);
		if (!quest)
			return;

		ImGui::SetCursorPosX(m_DetailPadding);

		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "%s", quest->GetTitle().c_str());
		ImGui::Spacing();

		const char* stateStr = "Unknown";
		ImVec4 stateColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
		switch (quest->GetState())
		{
		case EQuestState::Hidden:   stateStr = "Hidden"; break;
		case EQuestState::Active:   stateStr = "Active"; stateColor = ImVec4(0.4f, 0.8f, 0.4f, 1.0f); break;
		case EQuestState::Accepted:  stateStr = "In Progress"; stateColor = ImVec4(0.4f, 0.6f, 0.9f, 1.0f); break;
		case EQuestState::Complete:  stateStr = "Completed"; stateColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f); break;
		case EQuestState::Failed:    stateStr = "Failed"; stateColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break;
		}

		ImGui::TextColored(stateColor, "Status: %s", stateStr);
		ImGui::Spacing();

		if (!quest->GetDescription().empty())
		{
			ImGui::TextWrapped("%s", quest->GetDescription().c_str());
			ImGui::Spacing();
		}

		const auto& objectives = quest->GetObjectives();
		if (!objectives.empty())
		{
			ImGui::Text("Objectives");
			ImGui::Separator();
			for (const auto& obj : objectives)
			{
				ImGui::Bullet();
				if (obj.IsComplete)
					ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "[x] %s (%d/%d)",
						obj.DisplayText.c_str(), obj.CurrentCount, obj.TargetCount);
				else
					ImGui::Text("[ ] %s (%d/%d)", obj.DisplayText.c_str(), obj.CurrentCount, obj.TargetCount);
			}
			ImGui::Spacing();
		}

		if (quest->GetRewardMoney() > 0)
		{
			ImGui::Text("Rewards");
			ImGui::Separator();
			ImGui::Text("Gold: %d", quest->GetRewardMoney());
		}
	}

	bool QuestJournalScreen::OnMouseClick(float x, float y, int32_t button, bool pressed)
	{
		if (!pressed) return false;

		float screenW = static_cast<float>(Application::Get().GetWindow().GetWidth());
		float screenH = static_cast<float>(Application::Get().GetWindow().GetHeight());
		float winW = 700.0f;
		float winH = 500.0f;
		float winX = (screenW - winW) * 0.5f;
		float winY = (screenH - winH) * 0.5f;

		if (x >= winX && x <= winX + winW && y >= winY && y <= winY + winH)
			return true;

		Hide();
		return false;
	}
}
