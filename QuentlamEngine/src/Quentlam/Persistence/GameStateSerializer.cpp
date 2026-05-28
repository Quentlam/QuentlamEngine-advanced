#include "qlpch.h"
#include "Quentlam/Persistence/GameStateSerializer.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Core/Application.h"
#include <chrono>
#include <sstream>

namespace Quentlam
{
	SaveSchema GameStateSerializer::CreateSaveSchema()
	{
		SaveSchema schema(CurrentSaveVersion, "QuentlamGameSave");

		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
		schema.SetMetadata("engine_version", std::string("1.0.0"));
		schema.SetMetadata("save_timestamp", ss.str());

		SerializePlayer(schema);
		SerializeSimulation(schema);
		SerializeNpcModule(schema);
		SerializeQuestModule(schema);
		SerializeTileMap(schema);

		return schema;
	}

	void GameStateSerializer::PopulateSaveSchema(SaveSchema& schema)
	{
		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
		schema.SetMetadata("save_timestamp", ss.str());

		SerializePlayer(schema);
		SerializeSimulation(schema);
		SerializeNpcModule(schema);
		SerializeQuestModule(schema);
		SerializeTileMap(schema);
	}

	bool GameStateSerializer::LoadFromSaveSchema(const SaveSchema& schema)
	{
		int32_t version = schema.GetVersion();
		if (version != CurrentSaveVersion)
		{
			QL_CORE_WARN("Save version mismatch: expected {0}, got {1}", CurrentSaveVersion, version);
		}

		DeserializePlayer(schema);
		DeserializeSimulation(schema);
		DeserializeNpcModule(schema);
		DeserializeQuestModule(schema);
		DeserializeTileMap(schema);

		return true;
	}

	void GameStateSerializer::SetPlayerData(const std::string&, int32_t, int32_t, int32_t,
		float, float, const glm::vec2&)
	{
	}

	bool GameStateSerializer::Save(const std::string& slotId)
	{
		QL_CORE_INFO("Saving game to slot: {0}", slotId);

		SaveSchema schema = CreateSaveSchema();
		PopulateSaveSchema(schema);

		auto& manager = SaveManager::Get();
		ESaveStatus status = manager.SaveToFile(manager.GetSlotFilePath(slotId), schema);

		if (status != ESaveStatus::Success)
		{
			std::string errorMsg;
			switch (status)
			{
			case ESaveStatus::Failure: errorMsg = "Failed to save game"; break;
			case ESaveStatus::PermissionDenied: errorMsg = "Permission denied"; break;
			case ESaveStatus::Corrupted: errorMsg = "Save data corrupted"; break;
			default: errorMsg = "Unknown error"; break;
			}
			QL_CORE_ERROR("Save failed: {0}", errorMsg);
			if (OnSaveFailed)
				OnSaveFailed(errorMsg);
			return false;
		}

		QL_CORE_INFO("Game saved successfully to slot: {0}", slotId);
		return true;
	}

	bool GameStateSerializer::Load(const std::string& slotId)
	{
		QL_CORE_INFO("Loading game from slot: {0}", slotId);

		SaveSchema schema(CurrentSaveVersion, "");
		auto& manager = SaveManager::Get();
		ESaveStatus status = manager.LoadFromFile(manager.GetSlotFilePath(slotId), schema);

		if (status != ESaveStatus::Success)
		{
			std::string errorMsg;
			switch (status)
			{
			case ESaveStatus::FileNotFound: errorMsg = "Save file not found"; break;
			case ESaveStatus::Corrupted: errorMsg = "Save file corrupted"; break;
			case ESaveStatus::VersionMismatch: errorMsg = "Version mismatch"; break;
			default: errorMsg = "Unknown error"; break;
			}
			QL_CORE_ERROR("Load failed: {0}", errorMsg);
			if (OnLoadFailed)
				OnLoadFailed(errorMsg);
			return false;
		}

		if (!LoadFromSaveSchema(schema))
		{
			if (OnLoadFailed)
				OnLoadFailed("Failed to deserialize save data");
			return false;
		}

		QL_CORE_INFO("Game loaded successfully from slot: {0}", slotId);
		return true;
	}

	void GameStateSerializer::SerializePlayer(SaveSchema& schema)
	{
		SaveSection section("Player");
		SaveObject player("player_data", "Player");

		player.SetValue("player_id", std::string("player"));
		player.SetValue("money", int32_t(500));
		player.SetValue("health", int32_t(100));
		player.SetValue("max_health", int32_t(100));

		section.AddObject(player);
		schema.AddSection(section);
	}

	void GameStateSerializer::DeserializePlayer(const SaveSchema& schema)
	{
		auto* section = schema.GetSection("Player");
		if (!section) return;

		auto* player = section->GetObject("player_data");
		if (!player) return;

		QL_CORE_TRACE("Player data loaded from save");
	}

	void GameStateSerializer::SerializeSimulation(SaveSchema& schema)
	{
		SaveSection section("Simulation");
		SaveObject sim("simulation_data", "Simulation");

		auto& clock = SimulationModule::Get().GetClock();
		sim.SetValue("hour", clock.GetHour());
		sim.SetValue("minute", clock.GetMinute());
		sim.SetValue("day_of_year", clock.GetDayOfYear());
		sim.SetValue("year", clock.GetYear());
		sim.SetValue("time_speed", clock.GetTimeSpeed());
		sim.SetValue("paused", clock.IsPaused());
		sim.SetValue("weather", static_cast<int32_t>(SimulationModule::Get().GetCurrentWeather()));

		section.AddObject(sim);
		schema.AddSection(section);
	}

	void GameStateSerializer::DeserializeSimulation(const SaveSchema& schema)
	{
		auto* section = schema.GetSection("Simulation");
		if (!section) return;

		auto* sim = section->GetObject("simulation_data");
		if (!sim) return;

		int32_t hour = sim->GetValueAs<int32_t>("hour", 6);
		int32_t minute = sim->GetValueAs<int32_t>("minute", 0);
		int32_t dayOfYear = sim->GetValueAs<int32_t>("day_of_year", 1);
		int32_t year = sim->GetValueAs<int32_t>("year", 1);
		bool paused = sim->GetValueAs<bool>("paused", false);

		auto& clock = SimulationModule::Get().GetClock();
		clock.SetTime(hour, minute);
		clock.SetDayOfYear(dayOfYear);
		clock.SetYear(year);
		clock.SetPaused(paused);

		int32_t weatherVal = sim->GetValueAs<int32_t>("weather", 0);
		SimulationModule::Get().SetWeather(static_cast<EWeather>(weatherVal));

		QL_CORE_TRACE("Simulation data loaded from save");
	}

	void GameStateSerializer::SerializeNpcModule(SaveSchema& schema)
	{
		SaveSection section("NPCs");
		auto npcIds = NpcModule::Get().GetAllNpcIds();

		for (const auto& npcId : npcIds)
		{
			SaveObject npc(npcId, "NPC");

			auto* rel = NpcModule::Get().GetRelationship(npcId);
			if (rel)
			{
				npc.SetValue("friendship", rel->Friendship);
				npc.SetValue("hearts", rel->Hearts);
			}

			auto* mood = NpcModule::Get().GetMood(npcId);
			if (mood)
			{
				npc.SetValue("mood_value", mood->MoodValue);
				npc.SetValue("mood_reason", mood->MoodReason);
			}

			const glm::ivec2& pos = NpcModule::Get().GetNpcPosition(npcId);
			npc.SetValue("pos_x", pos.x);
			npc.SetValue("pos_y", pos.y);

			section.AddObject(npc);
		}

		schema.AddSection(section);
	}

	void GameStateSerializer::DeserializeNpcModule(const SaveSchema& schema)
	{
		auto* section = schema.GetSection("NPCs");
		if (!section) return;

		for (const auto& obj : section->GetAllObjects())
		{
			const std::string& npcId = obj.GetId();

			int32_t friendship = obj.GetValueAs<int32_t>("friendship", 0);
			int32_t hearts = obj.GetValueAs<int32_t>("hearts", 0);

			RelationshipState rel;
			rel.Friendship = friendship;
			rel.Hearts = hearts;
			NpcModule::Get().SetRelationship(npcId, rel);

			int32_t moodValue = obj.GetValueAs<int32_t>("mood_value", 50);
			auto* mood = NpcModule::Get().GetMood(npcId);
			if (mood)
			{
				mood->MoodValue = moodValue;
				mood->MoodReason = obj.GetValueAs<std::string>("mood_reason", "");
			}

			int32_t posX = obj.GetValueAs<int32_t>("pos_x", 0);
			int32_t posY = obj.GetValueAs<int32_t>("pos_y", 0);
			NpcModule::Get().SetNpcPosition(npcId, { posX, posY });
		}

		QL_CORE_TRACE("NPC data loaded from save ({0} NPCs)", section->GetAllObjects().size());
	}

	void GameStateSerializer::SerializeQuestModule(SaveSchema& schema)
	{
		SaveSection section("Quests");

		auto& questState = QuestEventModule::Get().GetQuestState();
		const auto& allQuests = questState.GetAllQuests();

		for (const auto& [questId, quest] : allQuests)
		{
			SaveObject qObj(questId, "Quest");
			qObj.SetValue("title", quest.GetTitle());
			qObj.SetValue("state", static_cast<int32_t>(quest.GetState()));
			qObj.SetValue("description", quest.GetDescription());

			for (const auto& obj : quest.GetObjectives())
			{
				qObj.SetValue("obj_" + obj.Id + "_current", obj.CurrentCount);
				qObj.SetValue("obj_" + obj.Id + "_complete", obj.IsComplete);
			}

			section.AddObject(qObj);
		}

		schema.AddSection(section);
	}

	void GameStateSerializer::DeserializeQuestModule(const SaveSchema& schema)
	{
		auto* section = schema.GetSection("Quests");
		if (!section) return;

		for (const auto& obj : section->GetAllObjects())
		{
			const std::string& questId = obj.GetId();
			auto* quest = QuestEventModule::Get().GetQuestState().GetQuest(questId);
			if (!quest) continue;

			int32_t stateVal = obj.GetValueAs<int32_t>("state", 0);
			quest->SetState(static_cast<EQuestState>(stateVal));

			for (const auto& objDef : quest->GetObjectives())
			{
				int32_t countVal = obj.GetValueAs<int32_t>("obj_" + objDef.Id + "_current", 0);
				quest->AddObjectiveProgress(objDef.Id, countVal - objDef.CurrentCount);
			}
		}

		QL_CORE_TRACE("Quest data loaded from save ({0} quests)", section->GetAllObjects().size());
	}

	void GameStateSerializer::SerializeTileMap(SaveSchema& schema)
	{
		SaveSection section("TileMap");
		QL_CORE_TRACE("TileMap serialization placeholder - use TileMapSerializer for full save");
		schema.AddSection(section);
	}

	void GameStateSerializer::DeserializeTileMap(const SaveSchema& schema)
	{
		auto* section = schema.GetSection("TileMap");
		if (!section) return;
		QL_CORE_TRACE("TileMap deserialization placeholder");
	}
}
