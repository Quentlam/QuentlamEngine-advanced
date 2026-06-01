#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/World/TileMap.h"
#include "Quentlam/World/WorldGridModule.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include <string>

namespace Quentlam
{

enum class ENavResult : uint8_t
{
	Success = 0,
	Blocked = 1,
	Timeout = 2,
	NoPath = 3
};

struct NavPathRequest
{
	glm::ivec2 Start;
	glm::ivec2 End;
	bool AllowDiagonal = true;
	int32_t MaxIterations = 1000;
	std::function<bool(const glm::ivec2&)> CanTraverse;
	std::function<float(const glm::ivec2&, const glm::ivec2&)> Heuristic;

	ENavResult Result = ENavResult::NoPath;
	std::vector<glm::ivec2> Path;
};

struct NavMeshTile
{
	bool Walkable = true;
	float Cost = 1.0f;
};

struct NavMeshAgentSettings
{
	std::string Name = "DefaultAgent";
	float Radius = 0.25f;
	float Height = 1.0f;
	float StepHeight = 0.1f;
	float MaxSlope = 45.0f;
	int32_t MaxIterations = 1000;
};

struct NavMeshLayer
{
	std::string Name;
	uint8_t LayerId = 0;
	bool DefaultWalkable = true;
	std::unordered_map<glm::ivec2, NavMeshTile, std::hash<glm::ivec2>> Tiles;
};

class NavigationMesh
{
public:
	NavigationMesh() = default;
	NavigationMesh(int32_t width, int32_t height);

	void SetSize(int32_t width, int32_t height);
	void SetTile(const glm::ivec2& pos, bool walkable, float cost = 1.0f);
	void SetWalkable(const glm::ivec2& pos, bool walkable);
	void SetCost(const glm::ivec2& pos, float cost);
	void Clear();

	bool IsWalkable(const glm::ivec2& pos) const;
	float GetCost(const glm::ivec2& pos) const;
	const glm::ivec2& GetSize() const { return m_Size; }
	const glm::ivec2& GetOrigin() const { return m_Origin; }
	void SetOrigin(const glm::ivec2& origin) { m_Origin = origin; }

	std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& pos, bool allowDiagonal) const;

	static NavigationMesh FromTileMap(const TileMap* tileMap, float tileSize = 1.0f, ETileCollision minCollision = ETileCollision::Block);

	bool SerializeToFile(const std::string& filepath) const;
	bool DeserializeFromFile(const std::string& filepath);
	std::string SerializeToString() const;
	bool DeserializeFromString(const std::string& json);

	NavMeshAgentSettings AgentSettings;
	std::vector<NavMeshLayer> Layers;
	float TileSize = 1.0f;

private:
	glm::ivec2 WorldToMesh(const glm::ivec2& worldPos) const;
	glm::ivec2 MeshToWorld(const glm::ivec2& meshPos) const;

	glm::ivec2 m_Size = { 0, 0 };
	glm::ivec2 m_Origin = { 0, 0 };
	std::unordered_map<glm::ivec2, NavMeshTile, std::hash<glm::ivec2>> m_Tiles;
};

class NavigationModule
{
public:
	NavigationModule() = default;
	static NavigationModule& Get();

	void SetNavMesh(Ref<NavigationMesh> mesh) { m_NavMesh = mesh; }
	Ref<NavigationMesh> GetNavMesh() const { return m_NavMesh; }
	TileMap* GetTileMap() const { return m_TileMap; }
	void SetTileMap(TileMap* tileMap);

	bool FindPath(const glm::ivec2& start, const glm::ivec2& end, std::vector<glm::ivec2>& outPath, bool allowDiagonal = true);
	ENavResult RequestPath(NavPathRequest& request);

	void Update(Scene* scene, float dt);
	void MoveTo(entt::registry& reg, entt::entity entity, const glm::ivec2& target);
	void Stop(entt::registry& reg, entt::entity entity);
	bool HasPath(entt::registry& reg, entt::entity entity);
	bool IsMoving(entt::registry& reg, entt::entity entity);

	void OnTileMapChanged();
	bool LoadNavMesh(const std::string& filepath);
	bool SaveNavMesh(const std::string& filepath);

private:
	float Heuristic(const glm::ivec2& a, const glm::ivec2& b) const;

	Ref<NavigationMesh> m_NavMesh;
	TileMap* m_TileMap = nullptr;
};

inline NavigationMesh::NavigationMesh(int32_t width, int32_t height)
	: m_Size(width, height) {}

inline void NavigationMesh::SetSize(int32_t width, int32_t height)
{
	m_Size = { width, height };
	m_Tiles.clear();
}

inline void NavigationMesh::SetTile(const glm::ivec2& pos, bool walkable, float cost)
{
	m_Tiles[pos] = { walkable, cost };
}

inline void NavigationMesh::SetWalkable(const glm::ivec2& pos, bool walkable)
{
	auto it = m_Tiles.find(pos);
	if (it != m_Tiles.end())
		it->second.Walkable = walkable;
}

inline void NavigationMesh::SetCost(const glm::ivec2& pos, float cost)
{
	auto it = m_Tiles.find(pos);
	if (it != m_Tiles.end())
		it->second.Cost = cost;
}

inline void NavigationMesh::Clear()
{
	m_Tiles.clear();
}

inline bool NavigationMesh::IsWalkable(const glm::ivec2& pos) const
{
	auto it = m_Tiles.find(pos);
	if (it == m_Tiles.end())
		return true;
	return it->second.Walkable;
}

inline float NavigationMesh::GetCost(const glm::ivec2& pos) const
{
	auto it = m_Tiles.find(pos);
	if (it == m_Tiles.end())
		return 1.0f;
	return it->second.Cost;
}

inline glm::ivec2 NavigationMesh::WorldToMesh(const glm::ivec2& worldPos) const
{
	return worldPos - m_Origin;
}

inline glm::ivec2 NavigationMesh::MeshToWorld(const glm::ivec2& meshPos) const
{
	return meshPos + m_Origin;
}

inline std::vector<glm::ivec2> NavigationMesh::GetNeighbors(const glm::ivec2& pos, bool allowDiagonal) const
{
	std::vector<glm::ivec2> result;
	if (allowDiagonal)
	{
		result = {
			{ pos.x, pos.y + 1 },
			{ pos.x + 1, pos.y + 1 },
			{ pos.x + 1, pos.y },
			{ pos.x + 1, pos.y - 1 },
			{ pos.x, pos.y - 1 },
			{ pos.x - 1, pos.y - 1 },
			{ pos.x - 1, pos.y },
			{ pos.x - 1, pos.y + 1 }
		};
	}
	else
	{
		result = {
			{ pos.x, pos.y + 1 },
			{ pos.x + 1, pos.y },
			{ pos.x, pos.y - 1 },
			{ pos.x - 1, pos.y }
		};
	}
	return result;
}

inline float NavigationModule::Heuristic(const glm::ivec2& a, const glm::ivec2& b) const
{
	return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

}
