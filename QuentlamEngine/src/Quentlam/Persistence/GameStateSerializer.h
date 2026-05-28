#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Persistence/PersistenceModule.h"
#include "Quentlam/Gameplay/Simulation/SimulationModule.h"
#include "Quentlam/Gameplay/NpcModule.h"
#include "Quentlam/Gameplay/QuestEventModule.h"
#include "Quentlam/World/TileMap.h"
#include <string>

namespace Quentlam
{
	class GameStateSerializer
	{
	public:
		static constexpr int32_t CurrentSaveVersion = 1;

		GameStateSerializer() = default;
		~GameStateSerializer() = default;

		SaveSchema CreateSaveSchema();
		void PopulateSaveSchema(SaveSchema& schema);
		bool LoadFromSaveSchema(const SaveSchema& schema);

		void SetPlayerData(const std::string& playerId, int32_t money, int32_t health, int32_t maxHealth,
			float stamina, float maxStamina, const glm::vec2& position);

		bool Save(const std::string& slotId);
		bool Load(const std::string& slotId);

		std::function<void(const std::string& error)> OnSaveFailed;
		std::function<void(const std::string& error)> OnLoadFailed;
		std::function<void(float progress)> OnSaveProgress;
		std::function<void(float progress)> OnLoadProgress;

	private:
		void SerializeSimulation(SaveSchema& schema);
		void DeserializeSimulation(const SaveSchema& schema);

		void SerializeNpcModule(SaveSchema& schema);
		void DeserializeNpcModule(const SaveSchema& schema);

		void SerializeQuestModule(SaveSchema& schema);
		void DeserializeQuestModule(const SaveSchema& schema);

		void SerializeTileMap(SaveSchema& schema);
		void DeserializeTileMap(const SaveSchema& schema);

		void SerializePlayer(SaveSchema& schema);
		void DeserializePlayer(const SaveSchema& schema);

		float m_SaveProgress = 0.0f;
		float m_LoadProgress = 0.0f;
	};
}
