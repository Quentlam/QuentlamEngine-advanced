#include "qlpch.h"
#include "Quentlam/Gameplay/QuestEventModule.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{
	EventGraph::EventGraph(const std::string& id)
		: m_Id(id) {}

	void EventGraph::AddNode(const std::string& nodeId, Ref<EventGraphNode> node)
	{
		node->SetGraph(this);
		m_Nodes[nodeId] = node;
	}

	void EventGraph::RemoveNode(const std::string& nodeId)
	{
		m_Nodes.erase(nodeId);
	}

	Ref<EventGraphNode> EventGraph::GetNode(const std::string& nodeId)
	{
		auto it = m_Nodes.find(nodeId);
		return it != m_Nodes.end() ? it->second : nullptr;
	}

	const Ref<EventGraphNode> EventGraph::GetNode(const std::string& nodeId) const
	{
		auto it = m_Nodes.find(nodeId);
		return it != m_Nodes.end() ? it->second : nullptr;
	}

	bool EventGraph::HasNode(const std::string& nodeId) const
	{
		return m_Nodes.find(nodeId) != m_Nodes.end();
	}

	void EventGraph::Clear()
	{
		m_Nodes.clear();
		m_StartNodeId.clear();
		m_Variables.clear();
	}

	void EventGraph::SetVariable(const std::string& key, const std::string& value)
	{
		m_Variables[key] = value;
	}

	std::string EventGraph::GetVariable(const std::string& key) const
	{
		auto it = m_Variables.find(key);
		return it != m_Variables.end() ? it->second : std::string();
	}

	bool EventGraph::HasVariable(const std::string& key) const
	{
		return m_Variables.find(key) != m_Variables.end();
	}

	bool EventGraph::EvaluateCondition(const std::string& condition) const
	{
		if (condition.empty())
			return true;

		size_t eqPos = condition.find("==");
		size_t nePos = condition.find("!=");
		size_t gtPos = condition.find(">");
		size_t ltPos = condition.find("<");

		size_t opPos = std::string::npos;
		std::string op;

		if (eqPos != std::string::npos) { opPos = eqPos; op = "=="; }
		else if (nePos != std::string::npos) { opPos = nePos; op = "!="; }
		else if (gtPos != std::string::npos) { opPos = gtPos; op = ">"; }
		else if (ltPos != std::string::npos) { opPos = ltPos; op = "<"; }

		if (opPos == std::string::npos)
		{
			auto it = m_Variables.find(condition);
			return it != m_Variables.end() && it->second == "true";
		}

		std::string key = condition.substr(0, opPos);
		std::string condVal = condition.substr(opPos + op.length());

		auto keyIt = m_Variables.find(key);
		std::string varVal = keyIt != m_Variables.end() ? keyIt->second : std::string();

		auto Trim = [](std::string& s) {
			size_t start = s.find_first_not_of(" \t");
			if (start == std::string::npos) { s.clear(); return; }
			size_t end = s.find_last_not_of(" \t");
			s = s.substr(start, end - start + 1);
		};
		Trim(varVal);
		Trim(condVal);

		if (op == "==") return varVal == condVal;
		if (op == "!=") return varVal != condVal;

		try {
			double varNum = std::stod(varVal);
			double condNum = std::stod(condVal);
			if (op == ">") return varNum > condNum;
			if (op == "<") return varNum < condNum;
		}
		catch (...) {}

		return varVal == condVal;
	}

	EventGraphNode::EventGraphNode(const std::string& id)
		: m_Id(id) {}

	DialogueNode::DialogueNode(const std::string& id)
		: EventGraphNode(id) {}

	DialogueNode::DialogueNode(const std::string& id, const std::string& text, const std::string& speaker)
		: EventGraphNode(id), m_Text(text), m_Speaker(speaker) {}

	bool DialogueNode::Execute()
	{
		QL_CORE_TRACE("[Dialogue] Speaker: {0}, Text: {1}", m_Speaker, m_Text);
		return !GetNextNodeId().empty();
	}

	ConditionNode::ConditionNode(const std::string& id)
		: EventGraphNode(id) {}

	ConditionNode::ConditionNode(const std::string& id, const std::string& condition, const std::string& trueNode, const std::string& falseNode)
		: EventGraphNode(id), m_Condition(condition), m_TrueNodeId(trueNode), m_FalseNodeId(falseNode) {}

	bool ConditionNode::Execute()
	{
		if (!m_Graph)
		{
			QL_CORE_WARN("[ConditionNode] No graph attached");
			return false;
		}

		bool result = m_Graph->EvaluateCondition(m_Condition);
		QL_CORE_TRACE("[ConditionNode] '{0}' evaluated to {1}", m_Condition, result ? "true" : "false");
		return result;
	}

	SetVariableNode::SetVariableNode(const std::string& id)
		: EventGraphNode(id) {}

	SetVariableNode::SetVariableNode(const std::string& id, const std::string& key, const std::string& value)
		: EventGraphNode(id), m_Key(key), m_Value(value) {}

	bool SetVariableNode::Execute()
	{
		if (!m_Graph)
		{
			QL_CORE_WARN("[SetVariableNode] No graph attached");
			return false;
		}

		m_Graph->SetVariable(m_Key, m_Value);
		QL_CORE_TRACE("[SetVariable] Set '{0}' = '{1}'", m_Key, m_Value);
		return !GetNextNodeId().empty();
	}

	QuestEventModule& QuestEventModule::Get()
	{
		static QuestEventModule instance;
		return instance;
	}

	EventGraph* QuestEventModule::CreateEventGraph(const std::string& id)
	{
		if (m_EventGraphs.find(id) != m_EventGraphs.end()) return nullptr;
		m_EventGraphs[id] = CreateRef<EventGraph>(id);
		return m_EventGraphs[id].get();
	}

	void QuestEventModule::RemoveEventGraph(const std::string& id)
	{
		m_EventGraphs.erase(id);
		m_RunningGraphs.erase(id);
	}

	EventGraph* QuestEventModule::GetEventGraph(const std::string& id)
	{
		return const_cast<EventGraph*>(static_cast<const QuestEventModule*>(this)->GetEventGraph(id));
	}

	const EventGraph* QuestEventModule::GetEventGraph(const std::string& id) const
	{
		auto it = m_EventGraphs.find(id);
		return it != m_EventGraphs.end() ? it->second.get() : nullptr;
	}

	bool QuestEventModule::StartEventGraph(const std::string& graphId)
	{
		auto* graph = GetEventGraph(graphId);
		if (!graph) return false;
		m_RunningGraphs.insert(graphId);
		if (OnEventGraphStarted)
			OnEventGraphStarted(graphId);
		return true;
	}

	void QuestEventModule::StopEventGraph(const std::string& graphId)
	{
		m_RunningGraphs.erase(graphId);
		if (OnEventGraphEnded)
			OnEventGraphEnded(graphId);
	}

	bool QuestEventModule::IsEventGraphRunning(const std::string& graphId) const
	{
		return m_RunningGraphs.find(graphId) != m_RunningGraphs.end();
	}

	void QuestEventModule::Update(float deltaTime)
	{
		for (const auto& graphId : m_RunningGraphs)
		{
			auto* graph = GetEventGraph(graphId);
			if (!graph) continue;

			const auto& nodes = graph->GetAllNodes();
			auto it = nodes.find(graph->GetStartNodeId());
			if (it != nodes.end())
			{
				bool hasNext = it->second->Execute();
				if (hasNext)
				{
					const std::string& nextId = it->second->GetNextNodeId();
					if (!nextId.empty())
						graph->SetStartNode(nextId);
				}
			}
		}
		(void)deltaTime;
	}

	void QuestEventModule::AdvanceDay()
	{
		QuestState::Get().AdvanceDay();
	}

	void QuestEventModule::RegisterDialogue(const std::string& npcId, const std::string& dialogueId, const std::string& text)
	{
		m_Dialogues[npcId][dialogueId] = text;
	}

	const std::string* QuestEventModule::GetDialogue(const std::string& npcId, const std::string& dialogueId) const
	{
		auto npcIt = m_Dialogues.find(npcId);
		if (npcIt == m_Dialogues.end()) return nullptr;
		auto diaIt = npcIt->second.find(dialogueId);
		return diaIt != npcIt->second.end() ? &diaIt->second : nullptr;
	}

	const std::unordered_map<std::string, std::string>& QuestEventModule::GetAllDialogues(const std::string& npcId) const
	{
		static std::unordered_map<std::string, std::string> empty;
		auto it = m_Dialogues.find(npcId);
		return it != m_Dialogues.end() ? it->second : empty;
	}

	void QuestEventModule::TriggerMail(const std::string& mailId, const std::string& from, const std::string& subject, const std::string& body)
	{
		m_MailList.emplace_back(mailId, from, subject, body);
	}

	bool QuestEventModule::HasUnreadMail() const
	{
		return !m_MailList.empty();
	}

	void QuestEventModule::MarkMailRead(size_t index)
	{
		if (index < m_MailList.size())
		{
			auto& mail = m_MailList[index];
			mail = std::make_tuple(std::get<0>(mail), std::get<1>(mail), std::get<2>(mail), std::get<3>(mail));
		}
	}

	void QuestEventModule::DeleteMail(size_t index)
	{
		if (index < m_MailList.size())
			m_MailList.erase(m_MailList.begin() + index);
	}
}
