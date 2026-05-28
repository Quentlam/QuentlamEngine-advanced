#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>

namespace Quentlam
{
	enum class EQuestType : uint8_t
	{
		None = 0,
		Basic = 1,
		Main = 2,
		Side = 3,
		Secret = 4,
		Story = 5,
		Monster = 6,
		Fishing = 7,
		Building = 8,
		Crafting = 9,
		Social = 10,
		Custom = 100
	};

	enum class EQuestState : uint8_t
	{
		Hidden = 0,
		Active = 1,
		Complete = 2,
		Failed = 3,
		Accepted = 4
	};

	enum class EObjectiveType : uint8_t
	{
		Gather = 0,
		Kill = 1,
		Deliver = 2,
		Talk = 3,
		Visit = 4,
		Fish = 5,
		Build = 6,
		Craft = 7,
		Marry = 8,
		Custom = 100
	};

	enum class EQuestEventType : uint8_t
	{
		Dialogue = 0,
		Script = 1,
		SceneChange = 2,
		Sound = 3,
		Animation = 4,
		Condition = 5,
		Variable = 6,
		UI = 7,
		Custom = 100
	};

	struct EventCondition
	{
		std::string Key;
		std::string Operator;
		std::string Value;
		std::string CombineWith;
		std::vector<EventCondition> SubConditions;

		bool Evaluate(const std::unordered_map<std::string, std::string>& variables) const;
	};

	struct EventParameter
	{
		std::string Key;
		std::string StringValue;
		int32_t IntValue = 0;
		float FloatValue = 0.0f;
		bool BoolValue = false;
	};

	struct QuestObjective
	{
		std::string Id;
		std::string DisplayText;
		EObjectiveType Type = EObjectiveType::Gather;
		std::string TargetId;
		int32_t TargetCount = 1;
		int32_t CurrentCount = 0;
		bool IsComplete = false;
		std::string Description;
		std::vector<EventCondition> StartConditions;
		std::vector<EventCondition> FailConditions;

		float GetProgress() const { return TargetCount > 0 ? static_cast<float>(CurrentCount) / TargetCount : 0.0f; }
	};

	class Quest
	{
	public:
		Quest() = default;
		Quest(const std::string& id, const std::string& title, EQuestType type);

		const std::string& GetId() const { return m_Id; }
		const std::string& GetTitle() const { return m_Title; }
		void SetTitle(const std::string& title) { m_Title = title; }

		EQuestType GetType() const { return m_Type; }
		EQuestState GetState() const { return m_State; }
		void SetState(EQuestState state) { m_State = state; }

		const std::string& GetDescription() const { return m_Description; }
		void SetDescription(const std::string& desc) { m_Description = desc; }

		void AddObjective(const QuestObjective& objective);
		void RemoveObjective(const std::string& objectiveId);
		QuestObjective* GetObjective(const std::string& objectiveId);
		const QuestObjective* GetObjective(const std::string& objectiveId) const;
		std::vector<QuestObjective>& GetObjectives() { return m_Objectives; }
		const std::vector<QuestObjective>& GetObjectives() const { return m_Objectives; }

		void AddObjectiveProgress(const std::string& objectiveId, int32_t delta = 1);
		bool AreAllObjectivesComplete() const;
		float GetOverallProgress() const;

		const std::string& GetRewardId() const { return m_RewardId; }
		void SetRewardId(const std::string& id) { m_RewardId = id; }
		int32_t GetRewardMoney() const { return m_RewardMoney; }
		void SetRewardMoney(int32_t money) { m_RewardMoney = money; }

		bool CanAccept() const;
		bool Accept();
		bool CanComplete() const;
		bool Complete();
		bool Fail();
		bool CanTurnIn() const;

		void AddNextQuest(const std::string& questId) { m_NextQuests.push_back(questId); }
		const std::vector<std::string>& GetNextQuests() const { return m_NextQuests; }

		bool HasStartCondition() const { return !m_StartConditions.empty(); }
		void AddStartCondition(const EventCondition& condition) { m_StartConditions.push_back(condition); }
		const std::vector<EventCondition>& GetStartConditions() const { return m_StartConditions; }

		std::string GetEventId() const { return m_EventId; }
		void SetEventId(const std::string& id) { m_EventId = id; }

	private:
		std::string m_Id;
		std::string m_Title;
		std::string m_Description;
		std::string m_EventId;
		EQuestType m_Type = EQuestType::None;
		EQuestState m_State = EQuestState::Hidden;
		std::vector<QuestObjective> m_Objectives;
		std::vector<EventCondition> m_StartConditions;
		std::vector<std::string> m_NextQuests;
		std::string m_RewardId;
		int32_t m_RewardMoney = 0;
	};

	class QuestState
	{
	public:
		QuestState() = default;
		static QuestState& Get();

		void RegisterQuest(const Quest& quest);
		void UnregisterQuest(const std::string& questId);
		Quest* GetQuest(const std::string& questId);
		const Quest* GetQuest(const std::string& questId) const;
		const std::unordered_map<std::string, Quest>& GetAllQuests() const { return m_Quests; }

		bool AcceptQuest(const std::string& questId);
		bool CompleteQuest(const std::string& questId);
		bool FailQuest(const std::string& questId);
		bool AbandonQuest(const std::string& questId);

		void UpdateObjective(const std::string& questId, const std::string& objectiveId, int32_t delta);

		bool HasQuest(const std::string& questId) const;
		bool IsQuestActive(const std::string& questId) const;
		bool IsQuestComplete(const std::string& questId) const;
		bool IsQuestFailed(const std::string& questId) const;

		std::vector<const Quest*> GetActiveQuests() const;
		std::vector<const Quest*> GetCompletedQuests() const;
		std::vector<const Quest*> GetAvailableQuests() const;

		void AdvanceDay();
		void Reset();

		std::function<void(const std::string& questId)> OnQuestAccepted;
		std::function<void(const std::string& questId)> OnQuestCompleted;
		std::function<void(const std::string& questId)> OnQuestFailed;

	private:
		std::unordered_map<std::string, Quest> m_Quests;
		std::unordered_set<std::string> m_CompletedQuests;
		std::unordered_set<std::string> m_FailedQuests;
	};

	class EventGraphNode;

	struct CutsceneStep
	{
		std::string Id;
		std::string Text;
		std::string Speaker;
		std::vector<std::string> Responses;
		std::string NextStepId;
		std::string Condition;
		std::vector<EventParameter> Parameters;
		float Duration = 0.0f;
		std::string Animation;
		std::string Sound;
		bool Skippable = true;

		std::function<bool()> OnExecute;
	};

	class EventGraph
	{
	public:
		EventGraph() = default;
		explicit EventGraph(const std::string& id);
		~EventGraph() = default;

		const std::string& GetId() const { return m_Id; }

		void AddNode(const std::string& nodeId, Ref<EventGraphNode> node);
		void RemoveNode(const std::string& nodeId);
		Ref<EventGraphNode> GetNode(const std::string& nodeId);
		const Ref<EventGraphNode> GetNode(const std::string& nodeId) const;
		const std::unordered_map<std::string, Ref<EventGraphNode>>& GetAllNodes() const { return m_Nodes; }

		void SetStartNode(const std::string& nodeId) { m_StartNodeId = nodeId; }
		const std::string& GetStartNodeId() const { return m_StartNodeId; }

		bool HasNode(const std::string& nodeId) const;
		void Clear();

		void SetVariable(const std::string& key, const std::string& value);
		std::string GetVariable(const std::string& key) const;
		bool HasVariable(const std::string& key) const;
		const std::unordered_map<std::string, std::string>& GetVariables() const { return m_Variables; }

		bool EvaluateCondition(const std::string& condition) const;

	private:
		std::string m_Id;
		std::string m_StartNodeId;
		std::unordered_map<std::string, Ref<EventGraphNode>> m_Nodes;
		mutable std::unordered_map<std::string, std::string> m_Variables;
	};

	class EventGraphNode
	{
	public:
		EventGraphNode() = default;
		explicit EventGraphNode(const std::string& id);
		virtual ~EventGraphNode() = default;

		const std::string& GetId() const { return m_Id; }
		EventGraph* GetGraph() const { return m_Graph; }
		void SetGraph(EventGraph* graph) { m_Graph = graph; }

		virtual bool Execute() = 0;
		virtual const char* GetType() const = 0;

		void SetNextNode(const std::string& nodeId) { m_NextNodeId = nodeId; }
		const std::string& GetNextNodeId() const { return m_NextNodeId; }

	protected:
		std::string m_Id;
		EventGraph* m_Graph = nullptr;
		std::string m_NextNodeId;
	};

	class DialogueNode : public EventGraphNode
	{
	public:
		DialogueNode() = default;
		explicit DialogueNode(const std::string& id);
		DialogueNode(const std::string& id, const std::string& text, const std::string& speaker);

		const std::string& GetText() const { return m_Text; }
		void SetText(const std::string& text) { m_Text = text; }
		const std::string& GetSpeaker() const { return m_Speaker; }
		void SetSpeaker(const std::string& speaker) { m_Speaker = speaker; }
		const std::vector<std::string>& GetResponses() const { return m_Responses; }
		void SetResponses(const std::vector<std::string>& responses) { m_Responses = responses; }

		bool Execute() override;
		const char* GetType() const override { return "Dialogue"; }

	private:
		std::string m_Text;
		std::string m_Speaker;
		std::vector<std::string> m_Responses;
	};

	class ConditionNode : public EventGraphNode
	{
	public:
		ConditionNode() = default;
		explicit ConditionNode(const std::string& id);
		ConditionNode(const std::string& id, const std::string& condition, const std::string& trueNode, const std::string& falseNode);

		void SetCondition(const std::string& condition) { m_Condition = condition; }
		const std::string& GetCondition() const { return m_Condition; }
		void SetTrueNode(const std::string& nodeId) { m_TrueNodeId = nodeId; }
		void SetFalseNode(const std::string& nodeId) { m_FalseNodeId = nodeId; }

		bool Execute() override;
		const char* GetType() const override { return "Condition"; }

	private:
		std::string m_Condition;
		std::string m_TrueNodeId;
		std::string m_FalseNodeId;
	};

	class SetVariableNode : public EventGraphNode
	{
	public:
		SetVariableNode() = default;
		explicit SetVariableNode(const std::string& id);
		SetVariableNode(const std::string& id, const std::string& key, const std::string& value);

		void SetKey(const std::string& key) { m_Key = key; }
		void SetValue(const std::string& value) { m_Value = value; }
		const std::string& GetKey() const { return m_Key; }
		const std::string& GetValue() const { return m_Value; }

		bool Execute() override;
		const char* GetType() const override { return "SetVariable"; }

	private:
		std::string m_Key;
		std::string m_Value;
	};

	class QuestEventModule
	{
	public:
		QuestEventModule() = default;
		static QuestEventModule& Get();

		QuestState& GetQuestState() { return QuestState::Get(); }
		const QuestState& GetQuestState() const { return QuestState::Get(); }

		EventGraph* CreateEventGraph(const std::string& id);
		void RemoveEventGraph(const std::string& id);
		EventGraph* GetEventGraph(const std::string& id);
		const EventGraph* GetEventGraph(const std::string& id) const;
		bool StartEventGraph(const std::string& graphId);
		void StopEventGraph(const std::string& graphId);
		bool IsEventGraphRunning(const std::string& graphId) const;

		void Update(float deltaTime);
		void AdvanceDay();

		void RegisterDialogue(const std::string& npcId, const std::string& dialogueId, const std::string& text);
		const std::string* GetDialogue(const std::string& npcId, const std::string& dialogueId) const;
		const std::unordered_map<std::string, std::string>& GetAllDialogues(const std::string& npcId) const;

		void TriggerMail(const std::string& mailId, const std::string& from, const std::string& subject, const std::string& body);
		bool HasUnreadMail() const;
		const std::vector<std::tuple<std::string, std::string, std::string, std::string>>& GetMailList() const { return m_MailList; }
		void MarkMailRead(size_t index);
		void DeleteMail(size_t index);

		std::function<void(const std::string& graphId)> OnEventGraphStarted;
		std::function<void(const std::string& graphId)> OnEventGraphEnded;

	private:
		std::unordered_map<std::string, Ref<EventGraph>> m_EventGraphs;
		std::unordered_set<std::string> m_RunningGraphs;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_Dialogues;
		std::vector<std::tuple<std::string, std::string, std::string, std::string>> m_MailList;
	};

	inline bool EventCondition::Evaluate(const std::unordered_map<std::string, std::string>& variables) const
	{
		if (!SubConditions.empty())
		{
			bool result = true;
			for (const auto& sub : SubConditions)
			{
				bool subResult = sub.Evaluate(variables);
				if (CombineWith == "AND")
					result = result && subResult;
				else if (CombineWith == "OR")
					result = result || subResult;
			}
			return result;
		}

		auto it = variables.find(Key);
		if (it == variables.end()) return false;

		const std::string& varValue = it->second;
		if (Operator == "==") return varValue == Value;
		if (Operator == "!=") return varValue != Value;
		if (Operator == ">")
		{
			try { return std::stof(varValue) > std::stof(Value); }
			catch (...) { return false; }
		}
		if (Operator == "<")
		{
			try { return std::stof(varValue) < std::stof(Value); }
			catch (...) { return false; }
		}
		return false;
	}

	inline Quest::Quest(const std::string& id, const std::string& title, EQuestType type)
		: m_Id(id), m_Title(title), m_Type(type) {}

	inline void Quest::AddObjective(const QuestObjective& objective)
	{
		m_Objectives.push_back(objective);
	}

	inline void Quest::RemoveObjective(const std::string& objectiveId)
	{
		m_Objectives.erase(
			std::remove_if(m_Objectives.begin(), m_Objectives.end(),
				[&objectiveId](const QuestObjective& o) { return o.Id == objectiveId; }),
			m_Objectives.end()
		);
	}

	inline QuestObjective* Quest::GetObjective(const std::string& objectiveId)
	{
		for (auto& obj : m_Objectives)
			if (obj.Id == objectiveId) return &obj;
		return nullptr;
	}

	inline const QuestObjective* Quest::GetObjective(const std::string& objectiveId) const
	{
		for (const auto& obj : m_Objectives)
			if (obj.Id == objectiveId) return &obj;
		return nullptr;
	}

	inline void Quest::AddObjectiveProgress(const std::string& objectiveId, int32_t delta)
	{
		auto* obj = GetObjective(objectiveId);
		if (!obj || obj->IsComplete) return;
		obj->CurrentCount += delta;
		if (obj->CurrentCount >= obj->TargetCount)
		{
			obj->CurrentCount = obj->TargetCount;
			obj->IsComplete = true;
		}
	}

	inline bool Quest::AreAllObjectivesComplete() const
	{
		for (const auto& obj : m_Objectives)
			if (!obj.IsComplete) return false;
		return true;
	}

	inline float Quest::GetOverallProgress() const
	{
		if (m_Objectives.empty()) return 0.0f;
		float total = 0.0f;
		for (const auto& obj : m_Objectives)
			total += obj.GetProgress();
		return total / m_Objectives.size();
	}

	inline bool Quest::CanAccept() const
	{
		return m_State == EQuestState::Hidden || m_State == EQuestState::Active;
	}

	inline bool Quest::Accept()
	{
		if (!CanAccept()) return false;
		m_State = EQuestState::Accepted;
		return true;
	}

	inline bool Quest::CanComplete() const
	{
		return m_State == EQuestState::Accepted && AreAllObjectivesComplete();
	}

	inline bool Quest::Complete()
	{
		if (!CanComplete()) return false;
		m_State = EQuestState::Complete;
		return true;
	}

	inline bool Quest::Fail()
	{
		if (m_State == EQuestState::Complete || m_State == EQuestState::Failed) return false;
		m_State = EQuestState::Failed;
		return true;
	}

	inline bool Quest::CanTurnIn() const
	{
		return CanComplete();
	}

	inline QuestState& QuestState::Get()
	{
		static QuestState instance;
		return instance;
	}

	inline void QuestState::RegisterQuest(const Quest& quest)
	{
		m_Quests[quest.GetId()] = quest;
	}

	inline void QuestState::UnregisterQuest(const std::string& questId)
	{
		m_Quests.erase(questId);
		m_CompletedQuests.erase(questId);
		m_FailedQuests.erase(questId);
	}

	inline Quest* QuestState::GetQuest(const std::string& questId)
	{
		return const_cast<Quest*>(static_cast<const QuestState*>(this)->GetQuest(questId));
	}

	inline const Quest* QuestState::GetQuest(const std::string& questId) const
	{
		auto it = m_Quests.find(questId);
		return it != m_Quests.end() ? &it->second : nullptr;
	}

	inline bool QuestState::AcceptQuest(const std::string& questId)
	{
		auto* quest = GetQuest(questId);
		if (!quest || !quest->Accept()) return false;
		if (OnQuestAccepted)
			OnQuestAccepted(questId);
		return true;
	}

	inline bool QuestState::CompleteQuest(const std::string& questId)
	{
		auto* quest = GetQuest(questId);
		if (!quest || !quest->Complete()) return false;
		m_CompletedQuests.insert(questId);
		if (OnQuestCompleted)
			OnQuestCompleted(questId);
		return true;
	}

	inline bool QuestState::FailQuest(const std::string& questId)
	{
		auto* quest = GetQuest(questId);
		if (!quest || !quest->Fail()) return false;
		m_FailedQuests.insert(questId);
		if (OnQuestFailed)
			OnQuestFailed(questId);
		return true;
	}

	inline bool QuestState::AbandonQuest(const std::string& questId)
	{
		auto* quest = GetQuest(questId);
		if (!quest) return false;
		quest->SetState(EQuestState::Hidden);
		return true;
	}

	inline void QuestState::UpdateObjective(const std::string& questId, const std::string& objectiveId, int32_t delta)
	{
		auto* quest = GetQuest(questId);
		if (!quest) return;
		quest->AddObjectiveProgress(objectiveId, delta);
		if (quest->CanComplete())
			CompleteQuest(questId);
	}

	inline bool QuestState::HasQuest(const std::string& questId) const
	{
		return m_Quests.find(questId) != m_Quests.end();
	}

	inline bool QuestState::IsQuestActive(const std::string& questId) const
	{
		auto* quest = GetQuest(questId);
		return quest && quest->GetState() == EQuestState::Accepted;
	}

	inline bool QuestState::IsQuestComplete(const std::string& questId) const
	{
		return m_CompletedQuests.find(questId) != m_CompletedQuests.end();
	}

	inline bool QuestState::IsQuestFailed(const std::string& questId) const
	{
		return m_FailedQuests.find(questId) != m_FailedQuests.end();
	}

	inline std::vector<const Quest*> QuestState::GetActiveQuests() const
	{
		std::vector<const Quest*> result;
		for (const auto& [id, quest] : m_Quests)
			if (quest.GetState() == EQuestState::Accepted) result.push_back(&quest);
		return result;
	}

	inline std::vector<const Quest*> QuestState::GetCompletedQuests() const
	{
		std::vector<const Quest*> result;
		for (const auto& id : m_CompletedQuests)
		{
			auto* q = GetQuest(id);
			if (q) result.push_back(q);
		}
		return result;
	}

	inline std::vector<const Quest*> QuestState::GetAvailableQuests() const
	{
		std::vector<const Quest*> result;
		for (const auto& [id, quest] : m_Quests)
			if (quest.GetState() == EQuestState::Hidden) result.push_back(&quest);
		return result;
	}

	inline void QuestState::AdvanceDay()
	{
	}

	inline void QuestState::Reset()
	{
		m_Quests.clear();
		m_CompletedQuests.clear();
		m_FailedQuests.clear();
	}

}
