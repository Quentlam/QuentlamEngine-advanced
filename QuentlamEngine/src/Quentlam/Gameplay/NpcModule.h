#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Entity.h"
#include "Quentlam/World/WorldGridModule.h"
#include "Quentlam/World/TileMap.h"
#include "Quentlam/Gameplay/Simulation/SimulationModule.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Quentlam
{
	enum class ENpcDisposition : uint8_t
	{
		Neutral = 0,
		Happy = 1,
		Annoyed = 2,
		Afraid = 3,
		Minus = 4,
		Romantic = 5
	};

	enum class ENpcState : uint8_t
	{
		Idle = 0,
		Walking = 1,
		Working = 2,
		Socializing = 3,
		Sleeping = 4,
		Eating = 5,
		Festival = 6,
		Event = 7,
		Custom = 100
	};

	enum class ESchedulePriority : uint8_t
	{
		Low = 0,
		Normal = 1,
		High = 2
	};

	enum class EPathResult : uint8_t
	{
		Success = 0,
		Blocked = 1,
		Timeout = 2,
		NoPath = 3
	};

	struct RelationshipState
	{
		int32_t Friendship = 0;
		int32_t Hearts = 0;
		int32_t TalkedToday = 0;
		int32_t GaveGiftToday = 0;
		int32_t ReceivedGiftToday = 0;
		int32_t QuestCompletedFor = 0;
		int32_t DailyGiftsGiven = 0;
		int32_t DailyGiftsReceived = 0;
		std::vector<std::string> TalkedToToday;
		std::vector<std::string> GiftsGivenToday;
		std::unordered_map<std::string, int32_t> GiftTastes;
		std::unordered_map<std::string, int32_t> CustomFlags;
		std::string LastGiftItemId;
		int32_t LastGiftQuality = 0;
		time_t LastInteractionDate = 0;

		float GetHeartMultiplier() const;
		int32_t GetHeartsFromFriendship() const;
	};

	struct NpcMood
	{
		int32_t MoodValue = 50;
		std::string MoodReason;
		std::vector<std::string> RecentEvents;
		int32_t PositiveDayCount = 0;
		int32_t NegativeDayCount = 0;

		int32_t GetMoodLevel() const;
		std::string GetMoodString() const;
	};

	struct ScheduleEntry
	{
		int32_t StartHour = 6;
		int32_t StartMinute = 0;
		glm::ivec2 TargetPosition = { 0, 0 };
		std::string AnimationName;
		std::string TaskId;
		std::string TargetNpcId;
		ESchedulePriority Priority = ESchedulePriority::Normal;
		std::string Condition;
		bool CanModify = true;

		int32_t GetStartTimeMinutes() const { return StartHour * 60 + StartMinute; }
	};

	class Schedule
	{
	public:
		Schedule() = default;

		void Clear();
		void AddEntry(const ScheduleEntry& entry);
		void RemoveEntry(int32_t hour);
		void SetDefaultLocation(const glm::ivec2& pos) { m_DefaultLocation = pos; }

		const ScheduleEntry* GetEntryForTime(int32_t hour, int32_t minute) const;
		const std::vector<ScheduleEntry>& GetAllEntries() const { return m_Entries; }
		const glm::ivec2& GetDefaultLocation() const { return m_DefaultLocation; }

		void SortEntries();
		bool HasEntryAt(int32_t hour, int32_t minute) const;

	private:
		std::vector<ScheduleEntry> m_Entries;
		glm::ivec2 m_DefaultLocation = { 0, 0 };
	};

	struct PathGoal
	{
		glm::ivec2 Target;
		int32_t MaxSteps = 1000;
		float Timeout = 5.0f;
		bool AllowDiagonal = true;
		std::function<bool(const glm::ivec2&)> CanTraverse;
		std::function<float(const glm::ivec2&)> CostFunction;

		EPathResult Result = EPathResult::Success;
		std::vector<glm::ivec2> Path;
		int32_t StepsTaken = 0;
		float TimeElapsed = 0.0f;
	};

	class PathFinder
	{
	public:
		static bool FindPath(PathGoal& goal);
		static std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& pos, bool allowDiagonal);
		static bool IsWalkable(const glm::ivec2& pos, const PathGoal& goal);
	};

	struct NpcRuntimeState
	{
		const ScheduleEntry* CurrentEntry = nullptr;
		std::vector<glm::ivec2> CurrentPath;
		int32_t PathIndex = 0;
		float MoveProgress = 0.0f;
		bool bPathComplete = true;
		glm::vec2 CurrentWorldPos = { 0.0f, 0.0f };
	};

	class NpcModule
	{
	public:
		struct NpcData
		{
			std::string Id;
			std::string DisplayName;
			std::string SpritePath;
			std::string PortraitPath;
			std::string ScheduleTableName;
			std::vector<std::string> Birthday;
			ESeason BirthdaySeason = ESeason::Spring;
			int32_t BirthdayDay = 0;
			std::vector<std::string> LikedItems;
			std::vector<std::string> DislikedItems;
			std::vector<std::string> LikedGifts;
			std::vector<std::string> HatedGifts;
			std::vector<std::string> NeutralGifts;
			std::unordered_map<std::string, int32_t> GiftReactionThresholds;
			ENpcDisposition BaseDisposition = ENpcDisposition::Neutral;
			int32_t Age = 30;
			bool IsGay = false;
			std::string CustomData;
		};

		NpcModule() = default;
		static NpcModule& Get();

		void RegisterNpc(const NpcData& data);
		void UnregisterNpc(const std::string& npcId);
		const NpcData* GetNpcData(const std::string& npcId) const;
		const std::unordered_map<std::string, NpcData>& GetAllNpcData() const { return m_NpcData; }

		RelationshipState* GetRelationship(const std::string& npcId);
		const RelationshipState* GetRelationship(const std::string& npcId) const;
		void SetRelationship(const std::string& npcId, const RelationshipState& state);
		void ModifyFriendship(const std::string& npcId, int32_t delta);
		void ModifyHearts(const std::string& npcId, int32_t delta);

		NpcMood* GetMood(const std::string& npcId);
		const NpcMood* GetMood(const std::string& npcId) const;
		void UpdateMood(const std::string& npcId, int32_t delta, const std::string& reason);

		const Schedule* GetSchedule(const std::string& npcId) const;
		Schedule* GetSchedule(const std::string& npcId);
		void SetSchedule(const std::string& npcId, const Schedule& schedule);

		void SetTileMap(TileMap* tileMap) { m_TileMap = tileMap; }
		TileMap* GetTileMap() const { return m_TileMap; }

		const glm::ivec2& GetNpcPosition(const std::string& npcId) const;
		void SetNpcPosition(const std::string& npcId, const glm::ivec2& pos);
		void MoveNpc(const std::string& npcId, const glm::ivec2& target);
		void NpcArrived(const std::string& npcId);

		ENpcState GetNpcState(const std::string& npcId) const;
		void SetNpcState(const std::string& npcId, ENpcState state);
		ENpcState GetNpcPreviousState(const std::string& npcId) const;

		void AdvanceDay();
		void ResetDailyFlags(const std::string& npcId);

		bool CanTalkTo(const std::string& npcId, const std::string& playerId) const;
		bool CanGiveGiftTo(const std::string& npcId, const std::string& playerId) const;
		int32_t GetGiftReaction(const std::string& npcId, const std::string& itemId) const;
		std::string GetGiftReactionString(const std::string& npcId, const std::string& itemId) const;

		int32_t GetHeartLevel(const std::string& npcId) const;
		bool IsBirthday(const std::string& npcId, ESeason season, int32_t day) const;

		std::vector<std::string> GetAllNpcIds() const;
		std::vector<std::string> GetNpcsAtLocation(const glm::ivec2& pos) const;

		void Update(float deltaTime);
		void OnDayStart();
		void OnDayEnd();

		std::function<void(const std::string& npcId, ENpcState newState, ENpcState oldState)> OnNpcStateChanged;
		std::function<void(const std::string& npcId, int32_t newHearts, int32_t oldHearts)> OnHeartLevelChanged;

	private:
		std::unordered_map<std::string, NpcData> m_NpcData;
		std::unordered_map<std::string, RelationshipState> m_Relationships;
		std::unordered_map<std::string, NpcMood> m_Moods;
		std::unordered_map<std::string, Schedule> m_Schedules;
		std::unordered_map<std::string, glm::ivec2> m_Positions;
		std::unordered_map<std::string, ENpcState> m_States;
		std::unordered_map<std::string, ENpcState> m_PreviousStates;
		std::unordered_map<std::string, NpcRuntimeState> m_RuntimeStates;
		TileMap* m_TileMap = nullptr;
	};

	inline float RelationshipState::GetHeartMultiplier() const
	{
		if (Hearts >= 12) return 1.5f;
		if (Hearts >= 8) return 1.25f;
		if (Hearts >= 4) return 1.0f;
		return 0.5f;
	}

	inline int32_t RelationshipState::GetHeartsFromFriendship() const
	{
		return Friendship / 200;
	}

	inline int32_t NpcMood::GetMoodLevel() const
	{
		if (MoodValue >= 80) return 4;
		if (MoodValue >= 60) return 3;
		if (MoodValue >= 40) return 2;
		if (MoodValue >= 20) return 1;
		return 0;
	}

	inline std::string NpcMood::GetMoodString() const
	{
		switch (GetMoodLevel())
		{
		case 4: return "ecstatic";
		case 3: return "happy";
		case 2: return "neutral";
		case 1: return "sad";
		default: return "miserable";
		}
	}

	inline void Schedule::Clear()
	{
		m_Entries.clear();
	}

	inline void Schedule::AddEntry(const ScheduleEntry& entry)
	{
		m_Entries.push_back(entry);
		SortEntries();
	}

	inline void Schedule::RemoveEntry(int32_t hour)
	{
		m_Entries.erase(
			std::remove_if(m_Entries.begin(), m_Entries.end(),
				[hour](const ScheduleEntry& e) { return e.StartHour == hour; }),
			m_Entries.end()
		);
	}

	inline const ScheduleEntry* Schedule::GetEntryForTime(int32_t hour, int32_t minute) const
	{
		if (m_Entries.empty()) return nullptr;
		int32_t time = hour * 60 + minute;
		const ScheduleEntry* best = nullptr;
		for (const auto& entry : m_Entries)
		{
			if (entry.GetStartTimeMinutes() <= time)
				best = &entry;
			else
				break;
		}
		return best;
	}

	inline void Schedule::SortEntries()
	{
		std::sort(m_Entries.begin(), m_Entries.end(),
			[](const ScheduleEntry& a, const ScheduleEntry& b) {
				return a.GetStartTimeMinutes() < b.GetStartTimeMinutes();
			});
	}

	inline bool Schedule::HasEntryAt(int32_t hour, int32_t minute) const
	{
		return GetEntryForTime(hour, minute) != nullptr;
	}

	inline NpcModule& NpcModule::Get()
	{
		static NpcModule instance;
		return instance;
	}

	inline void NpcModule::RegisterNpc(const NpcData& data)
	{
		m_NpcData[data.Id] = data;
		if (m_Relationships.find(data.Id) == m_Relationships.end())
			m_Relationships[data.Id] = RelationshipState{};
		if (m_Moods.find(data.Id) == m_Moods.end())
			m_Moods[data.Id] = NpcMood{};
		if (m_Schedules.find(data.Id) == m_Schedules.end())
			m_Schedules[data.Id] = Schedule{};
		if (m_Positions.find(data.Id) == m_Positions.end())
			m_Positions[data.Id] = glm::ivec2(0, 0);
		if (m_States.find(data.Id) == m_States.end())
			m_States[data.Id] = ENpcState::Idle;
	}

	inline void NpcModule::UnregisterNpc(const std::string& npcId)
	{
		m_NpcData.erase(npcId);
		m_Relationships.erase(npcId);
		m_Moods.erase(npcId);
		m_Schedules.erase(npcId);
		m_Positions.erase(npcId);
		m_States.erase(npcId);
	}

	inline const NpcModule::NpcData* NpcModule::GetNpcData(const std::string& npcId) const
	{
		auto it = m_NpcData.find(npcId);
		return it != m_NpcData.end() ? &it->second : nullptr;
	}

	inline RelationshipState* NpcModule::GetRelationship(const std::string& npcId)
	{
		return const_cast<RelationshipState*>(static_cast<const NpcModule*>(this)->GetRelationship(npcId));
	}

	inline const RelationshipState* NpcModule::GetRelationship(const std::string& npcId) const
	{
		auto it = m_Relationships.find(npcId);
		return it != m_Relationships.end() ? &it->second : nullptr;
	}

	inline void NpcModule::SetRelationship(const std::string& npcId, const RelationshipState& state)
	{
		m_Relationships[npcId] = state;
	}

	inline void NpcModule::ModifyFriendship(const std::string& npcId, int32_t delta)
	{
		auto* rel = GetRelationship(npcId);
		if (!rel) return;
		int32_t oldHearts = rel->GetHeartsFromFriendship();
		rel->Friendship = std::max(0, rel->Friendship + delta);
		int32_t newHearts = rel->GetHeartsFromFriendship();
		if (newHearts != oldHearts && OnHeartLevelChanged)
			OnHeartLevelChanged(npcId, newHearts, oldHearts);
	}

	inline void NpcModule::ModifyHearts(const std::string& npcId, int32_t delta)
	{
		auto* rel = GetRelationship(npcId);
		if (!rel) return;
		rel->Hearts = std::clamp(rel->Hearts + delta, 0, 25);
	}

	inline NpcMood* NpcModule::GetMood(const std::string& npcId)
	{
		return const_cast<NpcMood*>(static_cast<const NpcModule*>(this)->GetMood(npcId));
	}

	inline const NpcMood* NpcModule::GetMood(const std::string& npcId) const
	{
		auto it = m_Moods.find(npcId);
		return it != m_Moods.end() ? &it->second : nullptr;
	}

	inline void NpcModule::UpdateMood(const std::string& npcId, int32_t delta, const std::string& reason)
	{
		auto* mood = GetMood(npcId);
		if (!mood) return;
		mood->MoodValue = std::clamp(mood->MoodValue + delta, 0, 100);
		mood->MoodReason = reason;
		if (!reason.empty())
			mood->RecentEvents.push_back(reason);
		if (mood->RecentEvents.size() > 10)
			mood->RecentEvents.erase(mood->RecentEvents.begin());
	}

	inline const Schedule* NpcModule::GetSchedule(const std::string& npcId) const
	{
		auto it = m_Schedules.find(npcId);
		return it != m_Schedules.end() ? &it->second : nullptr;
	}

	inline Schedule* NpcModule::GetSchedule(const std::string& npcId)
	{
		return const_cast<Schedule*>(static_cast<const NpcModule*>(this)->GetSchedule(npcId));
	}

	inline void NpcModule::SetSchedule(const std::string& npcId, const Schedule& schedule)
	{
		m_Schedules[npcId] = schedule;
	}

	inline const glm::ivec2& NpcModule::GetNpcPosition(const std::string& npcId) const
	{
		static glm::ivec2 defaultPos = { 0, 0 };
		auto it = m_Positions.find(npcId);
		return it != m_Positions.end() ? it->second : defaultPos;
	}

	inline void NpcModule::SetNpcPosition(const std::string& npcId, const glm::ivec2& pos)
	{
		m_Positions[npcId] = pos;
	}

	inline void NpcModule::MoveNpc(const std::string& npcId, const glm::ivec2& target)
	{
		ENpcState oldState = GetNpcState(npcId);
		SetNpcState(npcId, ENpcState::Walking);
		m_Positions[npcId] = target;
	}

	inline void NpcModule::NpcArrived(const std::string& npcId)
	{
		ENpcState oldState = GetNpcState(npcId);
		SetNpcState(npcId, ENpcState::Idle);
	}

	inline ENpcState NpcModule::GetNpcState(const std::string& npcId) const
	{
		auto it = m_States.find(npcId);
		return it != m_States.end() ? it->second : ENpcState::Idle;
	}

	inline void NpcModule::SetNpcState(const std::string& npcId, ENpcState state)
	{
		if (m_States[npcId] != state)
		{
			m_PreviousStates[npcId] = m_States[npcId];
			m_States[npcId] = state;
			if (OnNpcStateChanged)
				OnNpcStateChanged(npcId, state, m_PreviousStates[npcId]);
		}
	}

	inline ENpcState NpcModule::GetNpcPreviousState(const std::string& npcId) const
	{
		auto it = m_PreviousStates.find(npcId);
		return it != m_PreviousStates.end() ? it->second : ENpcState::Idle;
	}

	inline void NpcModule::AdvanceDay()
	{
		OnDayEnd();
	}

	inline void NpcModule::ResetDailyFlags(const std::string& npcId)
	{
		auto* rel = GetRelationship(npcId);
		if (rel)
		{
			rel->TalkedToday = 0;
			rel->GaveGiftToday = 0;
			rel->ReceivedGiftToday = 0;
			rel->TalkedToToday.clear();
			rel->GiftsGivenToday.clear();
		}
	}

	inline bool NpcModule::CanTalkTo(const std::string& npcId, const std::string&) const
	{
		auto* rel = GetRelationship(npcId);
		if (!rel) return false;
		return rel->TalkedToday == 0;
	}

	inline bool NpcModule::CanGiveGiftTo(const std::string& npcId, const std::string&) const
	{
		auto* rel = GetRelationship(npcId);
		if (!rel) return false;
		return rel->GaveGiftToday < 2;
	}

	inline int32_t NpcModule::GetGiftReaction(const std::string& npcId, const std::string& itemId) const
	{
		auto* npc = GetNpcData(npcId);
		if (!npc) return 0;

		if (npc->HatedGifts.end() != std::find(npc->HatedGifts.begin(), npc->HatedGifts.end(), itemId))
			return -2;
		if (npc->DislikedItems.end() != std::find(npc->DislikedItems.begin(), npc->DislikedItems.end(), itemId))
			return -1;
		if (npc->LikedGifts.end() != std::find(npc->LikedGifts.begin(), npc->LikedGifts.end(), itemId))
			return 2;
		if (npc->LikedItems.end() != std::find(npc->LikedItems.begin(), npc->LikedItems.end(), itemId))
			return 1;
		return 0;
	}

	inline std::string NpcModule::GetGiftReactionString(const std::string& npcId, const std::string& itemId) const
	{
		int32_t reaction = GetGiftReaction(npcId, itemId);
		switch (reaction)
		{
		case 2: return "loved";
		case 1: return "liked";
		case 0: return "neutral";
		case -1: return "disliked";
		case -2: return "hated";
		default: return "neutral";
		}
	}

	inline int32_t NpcModule::GetHeartLevel(const std::string& npcId) const
	{
		auto* rel = GetRelationship(npcId);
		if (!rel) return 0;
		return rel->GetHeartsFromFriendship();
	}

	inline bool NpcModule::IsBirthday(const std::string& npcId, ESeason season, int32_t day) const
	{
		auto* npc = GetNpcData(npcId);
		if (!npc) return false;
		return npc->BirthdaySeason == season && npc->BirthdayDay == day;
	}

	inline std::vector<std::string> NpcModule::GetAllNpcIds() const
	{
		std::vector<std::string> ids;
		for (const auto& [id, _] : m_NpcData)
			ids.push_back(id);
		return ids;
	}

	inline std::vector<std::string> NpcModule::GetNpcsAtLocation(const glm::ivec2& pos) const
	{
		std::vector<std::string> result;
		for (const auto& [id, p] : m_Positions)
			if (p == pos) result.push_back(id);
		return result;
	}

	inline void NpcModule::OnDayStart()
	{
		for (const auto& [npcId, _] : m_NpcData)
			ResetDailyFlags(npcId);
	}

	inline void NpcModule::OnDayEnd()
	{
		for (auto& [npcId, mood] : m_Moods)
		{
			if (mood.PositiveDayCount > mood.NegativeDayCount)
				mood.MoodValue = std::min(100, mood.MoodValue + 5);
			else if (mood.NegativeDayCount > mood.PositiveDayCount)
				mood.MoodValue = std::max(0, mood.MoodValue - 5);
			mood.PositiveDayCount = 0;
			mood.NegativeDayCount = 0;
		}
	}
}
