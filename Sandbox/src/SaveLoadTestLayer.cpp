#include "SaveLoadTestLayer.h"
#include "Quentlam/Events/ApplicationEvent.h"
#include "Quentlam/Gameplay/Simulation/SimulationModule.h"
#include "Quentlam/Gameplay/NpcModule.h"
#include "Quentlam/World/TileMap.h"
#include "Quentlam/Persistence/PersistenceModule.h"
#include "imgui/imgui.h"

SaveLoadTestLayer::SaveLoadTestLayer()
	: Layer("SaveLoadTest")
{
}

void SaveLoadTestLayer::OnAttach()
{
	m_Serializer.SetPlayerData("player1", 5000, 100, 100, 50.0f, 100.0f, { 5.0f, 3.0f });

	m_Serializer.OnSaveFailed = [this](const std::string& err) {
		m_LastMessage = "Save failed: " + err;
		m_MessageTimer = 5.0f;
	};

	m_Serializer.OnLoadFailed = [this](const std::string& err) {
		m_LastMessage = "Load failed: " + err;
		m_MessageTimer = 5.0f;
	};
}

void SaveLoadTestLayer::OnDetach()
{
}

void SaveLoadTestLayer::OnUpdate(Quentlam::Timestep ts)
{
	if (m_MessageTimer > 0.0f)
		m_MessageTimer -= ts;
}

void SaveLoadTestLayer::OnEvent(Quentlam::Event& event)
{
}

void SaveLoadTestLayer::OnImGuiLayer()
{
	QL_PROFILE_FUNCTION();

	ImGui::Begin("Save/Load Test Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::CollapsingHeader("Game State", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto& sim = Quentlam::SimulationModule::Get().GetClock();
		ImGui::Text("Time: Day %d, %02d:%02d",
			Quentlam::SimulationModule::Get().GetCurrentDay(),
			sim.GetHour(), sim.GetMinute());
		ImGui::Text("Season: %d", (int)Quentlam::SimulationModule::Get().GetCurrentSeason());
		ImGui::Text("Weather: %d", (int)Quentlam::SimulationModule::Get().GetCurrentWeather());
	}

	if (ImGui::CollapsingHeader("Time Controls", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Advance 1 Day"))
		{
			Quentlam::SimulationModule::Get().GetClock().AdvanceDay();
			m_LastMessage = "Advanced to next day";
			m_MessageTimer = 2.0f;
		}

		ImGui::SameLine();

		if (ImGui::Button("Advance 1 Hour"))
		{
			Quentlam::SimulationModule::Get().GetClock().AddTime(1.0f);
			m_LastMessage = "Advanced 1 hour";
			m_MessageTimer = 2.0f;
		}

		ImGui::SameLine();

		if (ImGui::Button("Toggle Pause"))
		{
			auto& clock = Quentlam::SimulationModule::Get().GetClock();
			clock.SetPaused(!clock.IsPaused());
			m_LastMessage = clock.IsPaused() ? "Time paused" : "Time running";
			m_MessageTimer = 2.0f;
		}
	}

	if (ImGui::CollapsingHeader("Save / Load", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char slotBuf[64] = {};
		strncpy_s(slotBuf, m_SlotId.c_str(), sizeof(slotBuf) - 1);
		if (ImGui::InputText("Slot ID", slotBuf, sizeof(slotBuf)))
		{
			m_SlotId = slotBuf;
		}

		ImGui::Spacing();

		if (ImGui::Button("Save Game"))
		{
			if (m_Serializer.Save(m_SlotId))
			{
				m_LastMessage = "Saved to slot: " + m_SlotId;
				m_MessageTimer = 3.0f;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Load Game"))
		{
			if (m_Serializer.Load(m_SlotId))
			{
				m_LastMessage = "Loaded from slot: " + m_SlotId;
				m_MessageTimer = 3.0f;
			}
			else
			{
				m_LastMessage = "Slot not found or load failed";
				m_MessageTimer = 3.0f;
			}
		}
	}

	if (ImGui::CollapsingHeader("Registered NPCs", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& npcId : Quentlam::NpcModule::Get().GetAllNpcIds())
		{
			auto* mood = Quentlam::NpcModule::Get().GetMood(npcId);
			ImGui::BulletText("%s (Mood: %s, Hearts: %d)",
				npcId.c_str(),
				mood ? mood->GetMoodString().c_str() : "unknown",
				Quentlam::NpcModule::Get().GetHeartLevel(npcId));
		}
		if (Quentlam::NpcModule::Get().GetAllNpcIds().empty())
		{
			ImGui::TextDisabled("No NPCs registered");
		}
	}

	if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextWrapped(
			"Saves are stored in the 'saves/' directory.\n"
			"The save system includes:\n"
			"  - Game clock (day/season/weather)\n"
			"  - NPC relationships and schedules\n"
			"  - Quest progress\n"
			"  - TileMap chunks\n"
			"  - Player state"
		);
	}

	if (m_MessageTimer > 0.0f && !m_LastMessage.empty())
	{
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "%s", m_LastMessage.c_str());
	}

	ImGui::End();
}
