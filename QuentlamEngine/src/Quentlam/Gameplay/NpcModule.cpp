#include "qlpch.h"
#include "Quentlam/Gameplay/NpcModule.h"
#include "Quentlam/Gameplay/Simulation/SimulationModule.h"
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <float.h>

namespace Quentlam
{
	static constexpr float NPC_MOVE_SPEED = 2.0f;

	static float Heuristic(const glm::ivec2& a, const glm::ivec2& b)
	{
		return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
	}

	bool PathFinder::FindPath(PathGoal& goal)
	{
		goal.Path.clear();
		goal.StepsTaken = 0;
		goal.TimeElapsed = 0.0f;
		goal.Result = EPathResult::Success;

		glm::ivec2 start = { 0, 0 };
		glm::ivec2 end = goal.Target;

		if (start == end)
		{
			return true;
		}

		if (!IsWalkable(start, goal) || !IsWalkable(end, goal))
		{
			goal.Result = EPathResult::Blocked;
			return false;
		}

		struct Node
		{
			glm::ivec2 pos = glm::ivec2(0);
			float g = 0.0f;
			float h = 0.0f;
			float f = 0.0f;
			glm::ivec2 parent = glm::ivec2(0);
			Node() = default;
			Node(glm::ivec2 p, float gScore, float hScore, glm::ivec2 par)
				: pos(p), g(gScore), h(hScore), f(gScore + hScore), parent(par) {}
		};

		struct Ivec2Hash
		{
			size_t operator()(const glm::ivec2& v) const
			{
				size_t h1 = std::hash<int>()(v.x);
				size_t h2 = std::hash<int>()(v.y);
				return h1 ^ (h2 << 16);
			}
		};

		std::unordered_map<glm::ivec2, Node, Ivec2Hash> openSet;
		std::unordered_map<glm::ivec2, glm::ivec2, Ivec2Hash> cameFrom;
		std::unordered_set<glm::ivec2, Ivec2Hash> closedSet;

		openSet[start] = Node(start, 0.0f, Heuristic(start, end), start);

		while (!openSet.empty())
		{
			Node* current = nullptr;
			float bestF = FLT_MAX;
			glm::ivec2 currentPos{ 0, 0 };

			for (auto& [pos, node] : openSet)
			{
				if (node.f < bestF)
				{
					bestF = node.f;
					current = &node;
					currentPos = pos;
				}
			}

			if (current == nullptr) break;

			if (currentPos == end)
			{
				glm::ivec2 node = end;
				while (!(node == start))
				{
					goal.Path.push_back(node);
					auto it = cameFrom.find(node);
					if (it == cameFrom.end()) break;
					node = it->second;
				}
				goal.Path.push_back(start);
				std::reverse(goal.Path.begin(), goal.Path.end());
				goal.Result = EPathResult::Success;
				return true;
			}

			openSet.erase(currentPos);
			closedSet.insert(currentPos);

			if (goal.MaxSteps > 0 && static_cast<int32_t>(closedSet.size()) > goal.MaxSteps)
			{
				goal.Result = EPathResult::Timeout;
				return false;
			}

			for (const glm::ivec2& neighbor : GetNeighbors(currentPos, goal.AllowDiagonal))
			{
				if (closedSet.count(neighbor))
					continue;

				if (!IsWalkable(neighbor, goal))
					continue;

				float moveCost = 1.0f;
				if (goal.CostFunction)
					moveCost = goal.CostFunction(neighbor);

				float tentativeG = current->g + moveCost;

				auto it = openSet.find(neighbor);
				if (it == openSet.end() || tentativeG < it->second.g)
				{
					cameFrom[neighbor] = currentPos;
					float h = Heuristic(neighbor, end);
					openSet[neighbor] = Node(neighbor, tentativeG, h, currentPos);
				}
			}
		}

		goal.Result = EPathResult::NoPath;
		return false;
	}

	std::vector<glm::ivec2> PathFinder::GetNeighbors(const glm::ivec2& pos, bool allowDiagonal)
	{
		static const glm::ivec2 cardinal[] = {
			{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
		};
		static const glm::ivec2 diagonal[] = {
			{ 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
		};

		std::vector<glm::ivec2> result;
		for (const auto& d : cardinal)
			result.push_back({ pos.x + d.x, pos.y + d.y });

		if (allowDiagonal)
		{
			for (const auto& d : diagonal)
			{
				glm::ivec2 diag = { pos.x + d.x, pos.y + d.y };
				glm::ivec2 card1 = { pos.x + d.x, pos.y };
				glm::ivec2 card2 = { pos.x, pos.y + d.y };
				if (IsWalkable(card1, {}) && IsWalkable(card2, {}))
					result.push_back(diag);
			}
		}
		return result;
	}

	bool PathFinder::IsWalkable(const glm::ivec2& pos, const PathGoal& goal)
	{
		if (goal.CanTraverse && !goal.CanTraverse(pos))
			return false;
		return true;
	}

	void NpcModule::Update(float deltaTime)
	{
		if (SimulationModule::Get().GetClock().IsPaused())
			return;

		float tileSize = m_TileMap ? m_TileMap->GetTileSize() : 1.0f;

		for (auto& [npcId, schedule] : m_Schedules)
		{
			const ScheduleEntry* entry = schedule.GetEntryForTime(
				SimulationModule::Get().GetClock().GetHour(),
				SimulationModule::Get().GetClock().GetMinute()
			);
			if (!entry) continue;

			auto it = m_RuntimeStates.find(npcId);
			if (it == m_RuntimeStates.end())
			{
				m_RuntimeStates[npcId] = NpcRuntimeState{};
				it = m_RuntimeStates.find(npcId);
			}
			NpcRuntimeState& runtime = it->second;

			if (runtime.CurrentEntry != entry)
			{
				runtime.CurrentEntry = entry;
				runtime.PathIndex = 0;
				runtime.MoveProgress = 0.0f;
				runtime.bPathComplete = true;
				runtime.CurrentPath.clear();

				if (entry->TargetPosition.x != 0 || entry->TargetPosition.y != 0)
				{
					glm::ivec2 currentGridPos = GetNpcPosition(npcId);
					if (currentGridPos != entry->TargetPosition)
					{
						PathGoal goal;
						goal.Target = entry->TargetPosition;
						goal.MaxSteps = 500;
						goal.AllowDiagonal = true;

						if (m_TileMap)
						{
							goal.CanTraverse = [this](const glm::ivec2& pos) -> bool {
								return m_TileMap->IsWalkable(pos);
							};
						}

						if (PathFinder::FindPath(goal))
						{
							runtime.CurrentPath = goal.Path;
							runtime.PathIndex = 0;
							runtime.bPathComplete = false;
							runtime.CurrentWorldPos = glm::vec2(
								static_cast<float>(currentGridPos.x) * tileSize,
								static_cast<float>(currentGridPos.y) * tileSize
							);
							SetNpcState(npcId, ENpcState::Walking);
						}
					}
				}
				else
				{
					ENpcState currentState = GetNpcState(npcId);
					if (currentState == ENpcState::Walking)
						NpcArrived(npcId);
				}
			}

		if (!runtime.bPathComplete && !runtime.CurrentPath.empty())
		{
			if (runtime.PathIndex >= 0 && runtime.PathIndex < static_cast<int32_t>(runtime.CurrentPath.size()) - 1)
			{
					runtime.MoveProgress += deltaTime * NPC_MOVE_SPEED;

					glm::vec2 fromWorld = {
						static_cast<float>(runtime.CurrentPath[runtime.PathIndex].x) * tileSize,
						static_cast<float>(runtime.CurrentPath[runtime.PathIndex].y) * tileSize
					};
					glm::vec2 toWorld = {
						static_cast<float>(runtime.CurrentPath[runtime.PathIndex + 1].x) * tileSize,
						static_cast<float>(runtime.CurrentPath[runtime.PathIndex + 1].y) * tileSize
					};

					float t = std::min(1.0f, runtime.MoveProgress);
					runtime.CurrentWorldPos = glm::mix(fromWorld, toWorld, t);

					glm::ivec2 interpGrid = {
						static_cast<int>(std::round(runtime.CurrentWorldPos.x / tileSize)),
						static_cast<int>(std::round(runtime.CurrentWorldPos.y / tileSize))
					};
					SetNpcPosition(npcId, interpGrid);

					if (runtime.MoveProgress >= 1.0f)
					{
						runtime.MoveProgress = 0.0f;
						runtime.PathIndex++;
						if (runtime.PathIndex < static_cast<int32_t>(runtime.CurrentPath.size()))
						{
							SetNpcPosition(npcId, runtime.CurrentPath[runtime.PathIndex]);
						}
						if (runtime.PathIndex >= static_cast<int32_t>(runtime.CurrentPath.size()) - 1)
						{
							runtime.bPathComplete = true;
							NpcArrived(npcId);
						}
					}
				}
			}
		}
	}
}
