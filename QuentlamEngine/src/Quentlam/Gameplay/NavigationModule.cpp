#include "qlpch.h"
#include "NavigationModule.h"
#include "Quentlam/Scene/Scene.h"
#include "Quentlam/World/TileMap.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Quentlam
{

NavigationModule& NavigationModule::Get()
{
	static NavigationModule instance;
	return instance;
}

void NavigationModule::SetTileMap(TileMap* tileMap)
{
	m_TileMap = tileMap;
	if (tileMap)
	{
		auto navMesh = NavigationMesh::FromTileMap(tileMap, tileMap->GetTileSize());
		m_NavMesh = CreateRef<NavigationMesh>(navMesh);
	}
}

bool NavigationModule::FindPath(const glm::ivec2& start, const glm::ivec2& end, std::vector<glm::ivec2>& outPath, bool allowDiagonal)
{
	outPath.clear();
	if (!m_NavMesh)
		return false;

	NavPathRequest request;
	request.Start = start;
	request.End = end;
	request.AllowDiagonal = allowDiagonal;
	auto result = RequestPath(request);
	if (result == ENavResult::Success)
	{
		outPath = request.Path;
		return true;
	}
	return false;
}

ENavResult NavigationModule::RequestPath(NavPathRequest& request)
{
	request.Path.clear();
	request.Result = ENavResult::NoPath;

	if (!m_NavMesh)
		return ENavResult::NoPath;

	const glm::ivec2& start = request.Start;
	const glm::ivec2& end = request.End;

	if (!m_NavMesh->IsWalkable(start) || !m_NavMesh->IsWalkable(end))
		return ENavResult::Blocked;

	struct Node
	{
		glm::ivec2 Pos;
		float G = 0.0f;
		float H = 0.0f;
		float F() const { return G + H; }
		glm::ivec2 Parent;
	};

	auto HeuristicFunc = request.Heuristic ? request.Heuristic : [this](const glm::ivec2& a, const glm::ivec2& b) { return Heuristic(a, b); };

	std::unordered_map<glm::ivec2, Node, std::hash<glm::ivec2>> openSet;
	std::unordered_map<glm::ivec2, Node, std::hash<glm::ivec2>> closedSet;

	Node startNode;
	startNode.Pos = start;
	startNode.G = 0.0f;
	startNode.H = HeuristicFunc(start, end);
	startNode.Parent = start;
	openSet[start] = startNode;

	int32_t iterations = 0;
	while (!openSet.empty() && iterations < request.MaxIterations)
	{
		iterations++;

		glm::ivec2 currentPos;
		float lowestF = std::numeric_limits<float>::max();
		for (const auto& [pos, node] : openSet)
		{
			if (node.F() < lowestF)
			{
				lowestF = node.F();
				currentPos = pos;
			}
		}

		Node current = openSet[currentPos];
		openSet.erase(currentPos);

		if (currentPos == end)
		{
			glm::ivec2 pathPos = end;
			while (!(pathPos == start))
			{
				request.Path.push_back(pathPos);
				auto it = closedSet.find(pathPos);
				if (it == closedSet.end())
					break;
				pathPos = it->second.Parent;
			}
			request.Path.push_back(start);
			std::reverse(request.Path.begin(), request.Path.end());
			request.Result = ENavResult::Success;
			return ENavResult::Success;
		}

		closedSet[currentPos] = current;

		for (const auto& neighbor : m_NavMesh->GetNeighbors(currentPos, request.AllowDiagonal))
		{
			if (closedSet.find(neighbor) != closedSet.end())
				continue;
			if (!m_NavMesh->IsWalkable(neighbor))
				continue;
			if (request.CanTraverse && !request.CanTraverse(neighbor))
				continue;

			float moveCost = m_NavMesh->GetCost(neighbor);
			if (request.AllowDiagonal)
			{
				if (neighbor.x != currentPos.x && neighbor.y != currentPos.y)
					moveCost *= 1.414f;
			}

			float tentativeG = current.G + moveCost;

			auto it = openSet.find(neighbor);
			if (it == openSet.end() || tentativeG < it->second.G)
			{
				Node neighborNode;
				neighborNode.Pos = neighbor;
				neighborNode.G = tentativeG;
				neighborNode.H = HeuristicFunc(neighbor, end);
				neighborNode.Parent = currentPos;
				openSet[neighbor] = neighborNode;
			}
		}
	}

	request.Result = ENavResult::NoPath;
	return ENavResult::NoPath;
}

void NavigationModule::Update(Scene* scene, float dt)
{
	if (!scene)
		return;
	auto& reg = scene->GetRegistry();

	auto view = reg.view<NavAgentComponent, TransformComponent>();
	for (auto entity : view)
	{
		auto& agent = view.get<NavAgentComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		if (!agent.HasTarget || agent.CurrentPath.empty())
		{
			agent.State = NavAgentComponent::EAgentState::Idle;
			continue;
		}

		if (agent.State == NavAgentComponent::EAgentState::Paused)
			continue;

		agent.State = NavAgentComponent::EAgentState::Moving;

		if (agent.PathIndex >= static_cast<int32_t>(agent.CurrentPath.size()))
		{
			agent.CurrentPath.clear();
			agent.HasTarget = false;
			agent.State = NavAgentComponent::EAgentState::Idle;
			continue;
		}

		glm::ivec2 targetGrid = agent.CurrentPath[agent.PathIndex];
		glm::mat4 worldTransform = transform.GetWorldTransform(reg);
		glm::vec3 currentPos3 = glm::vec3(worldTransform[3]);

		glm::vec3 targetWorld(
			static_cast<float>(targetGrid.x) + 0.5f,
			static_cast<float>(targetGrid.y) + 0.5f,
			currentPos3.z
		);

		glm::vec2 direction2D(targetWorld.x - currentPos3.x, targetWorld.y - currentPos3.y);
		float distToTarget = glm::length(direction2D);

		if (distToTarget < agent.ArrivalThreshold)
		{
			agent.PathIndex++;
			if (agent.PathIndex >= static_cast<int32_t>(agent.CurrentPath.size()))
			{
				agent.CurrentPath.clear();
				agent.HasTarget = false;
				agent.State = NavAgentComponent::EAgentState::Idle;
			}
			continue;
		}

		if (distToTarget > 0.001f)
		{
			direction2D /= distToTarget;
		}
		else
		{
			direction2D = glm::vec2(0.0f);
		}

		float zVal = currentPos3.z;
		glm::mat4 moveMat = glm::translate(glm::mat4(1.0f),
			glm::vec3(direction2D.x * agent.Speed * dt, direction2D.y * agent.Speed * dt, 0.0f));
		transform.Transform = moveMat * transform.Transform;
	}
}

void NavigationModule::MoveTo(entt::registry& reg, entt::entity entity, const glm::ivec2& target)
{
	if (!reg.all_of<NavAgentComponent>(entity))
		return;

	auto& agent = reg.get<NavAgentComponent>(entity);
	auto& transform = reg.get<TransformComponent>(entity);

	glm::mat4 worldTransform = transform.GetWorldTransform(reg);
	glm::vec3 currentPos = glm::vec3(worldTransform[3]);

	glm::ivec2 startGrid;
	startGrid.x = static_cast<int32_t>(std::floor(currentPos.x));
	startGrid.y = static_cast<int32_t>(std::floor(currentPos.y));

	agent.TargetGridPos = target;
	agent.HasTarget = true;
	agent.PathIndex = 0;
	agent.State = NavAgentComponent::EAgentState::Moving;

	if (FindPath(startGrid, target, agent.CurrentPath, agent.AllowDiagonal))
	{
		if (!agent.CurrentPath.empty())
			agent.PathIndex = 0;
	}
	else
	{
		agent.CurrentPath.clear();
		agent.HasTarget = false;
		agent.State = NavAgentComponent::EAgentState::Idle;
	}
}

void NavigationModule::Stop(entt::registry& reg, entt::entity entity)
{
	if (!reg.all_of<NavAgentComponent>(entity))
		return;
	auto& agent = reg.get<NavAgentComponent>(entity);
	agent.CurrentPath.clear();
	agent.HasTarget = false;
	agent.State = NavAgentComponent::EAgentState::Idle;
}

bool NavigationModule::HasPath(entt::registry& reg, entt::entity entity)
{
	if (!reg.all_of<NavAgentComponent>(entity))
		return false;
	const auto& agent = reg.get<NavAgentComponent>(entity);
	return agent.HasTarget && !agent.CurrentPath.empty();
}

bool NavigationModule::IsMoving(entt::registry& reg, entt::entity entity)
{
	if (!reg.all_of<NavAgentComponent>(entity))
		return false;
	const auto& agent = reg.get<NavAgentComponent>(entity);
	return agent.State == NavAgentComponent::EAgentState::Moving;
}

void NavigationModule::OnTileMapChanged()
{
	if (m_TileMap)
	{
		auto navMesh = NavigationMesh::FromTileMap(m_TileMap, m_TileMap->GetTileSize());
		m_NavMesh = CreateRef<NavigationMesh>(navMesh);
	}
}

bool NavigationModule::SaveNavMesh(const std::string& filepath)
{
	if (!m_NavMesh)
	{
		QL_CORE_ERROR("NavigationModule: Cannot save null NavMesh to {0}", filepath);
		return false;
	}
	return m_NavMesh->SerializeToFile(filepath);
}

bool NavigationModule::LoadNavMesh(const std::string& filepath)
{
	m_NavMesh = CreateRef<NavigationMesh>();
	if (!m_NavMesh->DeserializeFromFile(filepath))
	{
		QL_CORE_ERROR("NavigationModule: Failed to load NavMesh from {0}", filepath);
		m_NavMesh.reset();
		return false;
	}
	QL_CORE_INFO("NavigationModule: Loaded NavMesh from {0}", filepath);
	return true;
}

NavigationMesh NavigationMesh::FromTileMap(const TileMap* tileMap, float tileSize, ETileCollision)
{
	NavigationMesh navMesh;
	if (!tileMap)
		return navMesh;

	auto mapSize = tileMap->GetMapSize();
	navMesh.SetSize(mapSize.x, mapSize.y);

	for (int32_t y = 0; y < mapSize.y; ++y)
	{
		for (int32_t x = 0; x < mapSize.x; ++x)
		{
			glm::ivec2 gridPos(x, y);
			auto* tile = tileMap->GetTile(gridPos);
			if (!tile)
			{
				navMesh.SetWalkable(gridPos, false);
				continue;
			}

			bool walkable = true;
			for (size_t i = 0; i < static_cast<size_t>(ETileLayer::Count); ++i)
			{
				const auto& layerData = tile->Layers[i];
				if (!layerData.IsActive())
					continue;

				auto* tileDef = tileMap->GetTileDefinition(layerData.Type);
				if (!tileDef)
					continue;

				if (tileDef->Collision != ETileCollision::None)
					walkable = false;
			}

			navMesh.SetWalkable(gridPos, walkable);
		}
	}

	return navMesh;
}

bool NavigationMesh::SerializeToFile(const std::string& filepath) const
{
	std::string json = SerializeToString();
	FILE* f = fopen(filepath.c_str(), "w");
	if (!f)
	{
		QL_CORE_ERROR("NavigationMesh: Cannot open file for writing: {0}", filepath);
		return false;
	}
	fwrite(json.c_str(), 1, json.size(), f);
	fclose(f);
	return true;
}

bool NavigationMesh::DeserializeFromFile(const std::string& filepath)
{
	FILE* f = fopen(filepath.c_str(), "rb");
	if (!f)
	{
		QL_CORE_ERROR("NavigationMesh: Cannot open file for reading: {0}", filepath);
		return false;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::string json(len + 1, '\0');
	fread(json.data(), 1, len, f);
	fclose(f);
	json.resize(len);
	return DeserializeFromString(json);
}

std::string NavigationMesh::SerializeToString() const
{
	std::string json;
	json.reserve(4096);
	json += "{\n";
	json += "  \"agent_radius\": " + std::to_string(AgentSettings.Radius) + ",\n";
	json += "  \"agent_height\": " + std::to_string(AgentSettings.Height) + ",\n";
	json += "  \"agent_step_height\": " + std::to_string(AgentSettings.StepHeight) + ",\n";
	json += "  \"agent_max_slope\": " + std::to_string(AgentSettings.MaxSlope) + ",\n";
	json += "  \"agent_max_iterations\": " + std::to_string(AgentSettings.MaxIterations) + ",\n";
	json += "  \"tile_size\": " + std::to_string(TileSize) + ",\n";
	json += "  \"size_x\": " + std::to_string(m_Size.x) + ",\n";
	json += "  \"size_y\": " + std::to_string(m_Size.y) + ",\n";
	json += "  \"origin_x\": " + std::to_string(m_Origin.x) + ",\n";
	json += "  \"origin_y\": " + std::to_string(m_Origin.y) + ",\n";

	json += "  \"tiles\": [\n";
	bool first = true;
	for (const auto& [pos, tile] : m_Tiles)
	{
		if (!first) json += ",\n";
		first = false;
		json += "    {\"x\": " + std::to_string(pos.x) +
			", \"y\": " + std::to_string(pos.y) +
			", \"walkable\": " + (tile.Walkable ? "true" : "false") +
			", \"cost\": " + std::to_string(tile.Cost) + "}";
	}
	json += "\n  ]\n";
	json += "}\n";
	return json;
}

bool NavigationMesh::DeserializeFromString(const std::string& str)
{
	m_Tiles.clear();

	size_t p = 0;

	auto SkipWhite = [&str, &p]() {
		while (p < str.size() && std::isspace((unsigned char)str[p])) p++;
	};

	auto Expect = [&str, &p, &SkipWhite](char c) -> bool {
		SkipWhite();
		if (p < str.size() && str[p] == c) { p++; return true; }
		return false;
	};

	auto ReadNumber = [&str, &p, &SkipWhite]() -> double {
		SkipWhite();
		size_t start = p;
		if (p < str.size() && (str[p] == '-' || str[p] == '+')) p++;
		while (p < str.size() && (std::isdigit((unsigned char)str[p]) || str[p] == '.' || str[p] == 'e' || str[p] == 'E' || str[p] == '-' || str[p] == '+')) p++;
		return std::stod(str.substr(start, p - start));
	};

	auto ReadInt = [&ReadNumber]() -> int64_t {
		return static_cast<int64_t>(ReadNumber());
	};

	auto ReadString = [&str, &p, &SkipWhite]() -> std::string {
		SkipWhite();
		if (p < str.size() && (str[p] == '"' || str[p] == '\'')) {
			char quote = str[p++];
			size_t start = p;
			while (p < str.size() && str[p] != quote) {
				if (str[p] == '\\') p++;
				p++;
			}
			std::string result = str.substr(start, p - start);
			if (p < str.size()) p++;
			return result;
		}
		return "";
	};

	auto ReadBool = [&str, &p, &SkipWhite]() -> bool {
		SkipWhite();
		size_t start = p;
		while (p < str.size() && std::isalpha((unsigned char)str[p])) p++;
		std::string val = str.substr(start, p - start);
		return val == "true" || val == "True" || val == "TRUE";
	};

	auto ReadKey = [&ReadString]() -> std::string {
		return ReadString();
	};

	SkipWhite();
	if (p < str.size() && str[p] == '{') p++;
	else return false;

	while (true)
	{
		SkipWhite();
		if (p >= str.size()) break;
		if (str[p] == '}') { p++; break; }
		if (str[p] == ',') { p++; continue; }

		std::string key = ReadKey();
		Expect(':');

		if (key == "agent_radius") AgentSettings.Radius = static_cast<float>(ReadNumber());
		else if (key == "agent_height") AgentSettings.Height = static_cast<float>(ReadNumber());
		else if (key == "agent_step_height") AgentSettings.StepHeight = static_cast<float>(ReadNumber());
		else if (key == "agent_max_slope") AgentSettings.MaxSlope = static_cast<float>(ReadNumber());
		else if (key == "agent_max_iterations") AgentSettings.MaxIterations = static_cast<int32_t>(ReadInt());
		else if (key == "tile_size") TileSize = static_cast<float>(ReadNumber());
		else if (key == "size_x") m_Size.x = static_cast<int32_t>(ReadInt());
		else if (key == "size_y") m_Size.y = static_cast<int32_t>(ReadInt());
		else if (key == "origin_x") m_Origin.x = static_cast<int32_t>(ReadInt());
		else if (key == "origin_y") m_Origin.y = static_cast<int32_t>(ReadInt());
		else if (key == "tiles")
		{
			Expect('[');
			while (true)
			{
				SkipWhite();
				if (p < str.size() && str[p] == ']') { p++; break; }
				if (str[p] == ',') { p++; continue; }
				Expect('{');
				int tx = 0, ty = 0;
				bool twalk = true;
				float tcost = 1.0f;
				for (int i = 0; i < 4; i++)
				{
					SkipWhite();
					std::string tk = ReadKey();
					Expect(':');
					if (tk == "x") tx = static_cast<int32_t>(ReadInt());
					else if (tk == "y") ty = static_cast<int32_t>(ReadInt());
					else if (tk == "walkable") twalk = ReadBool();
					else if (tk == "cost") tcost = static_cast<float>(ReadNumber());
					SkipWhite();
					if (p < str.size() && str[p] == ',') p++;
				}
				Expect('}');
				m_Tiles[glm::ivec2(tx, ty)] = { twalk, tcost };
			}
		}
		else
		{
			SkipWhite();
			while (p < str.size() && str[p] != ',' && str[p] != '}') p++;
		}
	}

	return true;
}

}
