#pragma once
#include "WorldGridModule.h"
#include <string>
#include <unordered_map>
#include <set>

namespace Quentlam
{
	class TileMap
	{
	public:
		TileMap() = default;
		TileMap(const std::string& name, const glm::ivec2& mapSize, float tileSize = 1.0f);
		~TileMap() = default;

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
		const glm::ivec2& GetMapSize() const { return m_MapSize; }
		float GetTileSize() const { return m_TileSize; }
		void SetTileSize(float size) { m_TileSize = size; }

		void SetTile(const glm::ivec2& gridPos, const Tile& tile);
		void SetTileType(const glm::ivec2& gridPos, ETileType type);
		void SetTileLayer(const glm::ivec2& gridPos, ETileLayer layer, ETileType type, int variant = 0);
		Tile* GetTile(const glm::ivec2& gridPos);
		const Tile* GetTile(const glm::ivec2& gridPos) const;
		TileLayerData* GetTileLayer(const glm::ivec2& gridPos, ETileLayer layer);
		void ClearTileLayer(const glm::ivec2& gridPos, ETileLayer layer);

		bool IsValidPosition(const glm::ivec2& gridPos) const;
		bool IsWalkable(const glm::ivec2& gridPos) const;
		bool IsInteractive(const glm::ivec2& gridPos) const;

		void AddRegion(const Region& region);
		void RemoveRegion(const std::string& regionName);
		const Region* GetRegionAt(const glm::ivec2& gridPos) const;
		const Region* GetRegion(const std::string& regionName) const;
		const std::vector<Region>& GetRegions() const { return m_Regions; }

		void AddPortal(const glm::ivec2& gridPos, const Portal& portal);
		const Portal* GetPortalAt(const glm::ivec2& gridPos) const;
		void RemovePortal(const glm::ivec2& gridPos);
		const std::unordered_map<glm::ivec2, Portal>& GetPortals() const { return m_Portals; }

		void AddEntityAt(const glm::ivec2& gridPos, entt::entity entity);
		void RemoveEntityFrom(const glm::ivec2& gridPos, entt::entity entity);
		const std::set<entt::entity>& GetEntitiesAt(const glm::ivec2& gridPos) const;
		std::set<entt::entity> GetEntitiesInRect(const glm::ivec2& min, const glm::ivec2& max) const;

		void SetTileDefinition(ETileType type, const TileDef& def);
		const TileDef* GetTileDefinition(ETileType type) const;

		void Clear();
		void IncrementVersion() { m_Version++; }
		void Resize(const glm::ivec2& newSize);

		Chunk* GetChunk(const glm::ivec2& chunkCoord);
		const Chunk* GetChunk(const glm::ivec2& chunkCoord) const;
		void EnsureChunkExists(const glm::ivec2& chunkCoord);

		const std::unordered_map<glm::ivec2, Ref<Chunk>>& GetAllChunks() const { return m_Chunks; }

		const std::string& GetMapId() const { return m_MapId; }
		void SetMapId(const std::string& id) { m_MapId = id; }
		int32_t GetVersion() const { return m_Version; }
		void SetVersion(int32_t v) { m_Version = v; }

	private:
		std::string m_Name;
		glm::ivec2 m_MapSize = { 100, 100 };
		float m_TileSize = 1.0f;

		std::unordered_map<glm::ivec2, Ref<Chunk>> m_Chunks;
		std::unordered_map<glm::ivec2, std::set<entt::entity>> m_EntityGrid;

		std::vector<Region> m_Regions;
		std::unordered_map<glm::ivec2, Portal> m_Portals;

		std::unordered_map<ETileType, TileDef> m_TileDefinitions;

		std::string m_MapId;
		int32_t m_Version = 1;
	};

	inline TileMap::TileMap(const std::string& name, const glm::ivec2& mapSize, float tileSize)
		: m_Name(name), m_MapSize(mapSize), m_TileSize(tileSize)
	{
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Empty; def.Type = ETileType::Empty; def.Collision = ETileCollision::None;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Empty] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Solid; def.Type = ETileType::Solid; def.Collision = ETileCollision::Block;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Solid] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Grass; def.Type = ETileType::Grass; def.Collision = ETileCollision::None;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Grass] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Water; def.Type = ETileType::Water; def.Collision = ETileCollision::None;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true; def.UVLock = false;
			m_TileDefinitions[ETileType::Water] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Sand; def.Type = ETileType::Sand; def.Collision = ETileCollision::None;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Sand] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Stone; def.Type = ETileType::Stone; def.Collision = ETileCollision::Block;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Stone] = def;
		}
		{
			TileDef def;
			def.ID = (uint32_t)ETileType::Wood; def.Type = ETileType::Wood; def.Collision = ETileCollision::Block;
			def.UVOffset = { 0.0f, 0.0f }; def.UVSize = { 1.0f, 1.0f };
			def.AtlasOffset = { 0, 0 }; def.TileSize = { 16, 16 }; def.TextureID = -1; def.Walkable = true;
			m_TileDefinitions[ETileType::Wood] = def;
		}
	}

	inline void TileMap::SetTile(const glm::ivec2& gridPos, const Tile& tile)
	{
		if (!IsValidPosition(gridPos)) return;
		EnsureChunkExists(GridQuery::GetChunkCoord(gridPos));
		Chunk* chunk = GetChunk(GridQuery::GetChunkCoord(gridPos));
		if (chunk)
		{
			glm::ivec2 local = GridQuery::GetLocalTileCoord(gridPos);
			chunk->SetTile(local.x, local.y, tile);
		}
	}

	inline void TileMap::SetTileType(const glm::ivec2& gridPos, ETileType type)
	{
		SetTileLayer(gridPos, ETileLayer::Ground, type, 0);
	}

	inline void TileMap::SetTileLayer(const glm::ivec2& gridPos, ETileLayer layer, ETileType type, int variant)
	{
		if (!IsValidPosition(gridPos)) return;
		EnsureChunkExists(GridQuery::GetChunkCoord(gridPos));
		Chunk* chunk = GetChunk(GridQuery::GetChunkCoord(gridPos));
		if (!chunk) return;
		glm::ivec2 local = GridQuery::GetLocalTileCoord(gridPos);
		Tile* tile = chunk->GetTile(local.x, local.y);
		if (!tile) return;
		(*tile)[layer].Type = type;
		(*tile)[layer].Variant = variant;
		chunk->MarkDirty();
	}

	inline TileLayerData* TileMap::GetTileLayer(const glm::ivec2& gridPos, ETileLayer layer)
	{
		Tile* tile = GetTile(gridPos);
		if (!tile) return nullptr;
		return &(*tile)[layer];
	}

	inline void TileMap::ClearTileLayer(const glm::ivec2& gridPos, ETileLayer layer)
	{
		Tile* tile = GetTile(gridPos);
		if (!tile) return;
		tile->ClearLayer(layer);
		Chunk* chunk = GetChunk(GridQuery::GetChunkCoord(gridPos));
		if (chunk) chunk->MarkDirty();
	}

	inline Tile* TileMap::GetTile(const glm::ivec2& gridPos)
	{
		if (!IsValidPosition(gridPos)) return nullptr;
		Chunk* chunk = GetChunk(GridQuery::GetChunkCoord(gridPos));
		if (!chunk) return nullptr;
		glm::ivec2 local = GridQuery::GetLocalTileCoord(gridPos);
		return chunk->GetTile(local.x, local.y);
	}

	inline const Tile* TileMap::GetTile(const glm::ivec2& gridPos) const
	{
		if (!IsValidPosition(gridPos)) return nullptr;
		const Chunk* chunk = GetChunk(GridQuery::GetChunkCoord(gridPos));
		if (!chunk) return nullptr;
		glm::ivec2 local = GridQuery::GetLocalTileCoord(gridPos);
		return chunk->GetTile(local.x, local.y);
	}

	inline bool TileMap::IsValidPosition(const glm::ivec2& gridPos) const
	{
		return gridPos.x >= 0 && gridPos.x < m_MapSize.x && gridPos.y >= 0 && gridPos.y < m_MapSize.y;
	}

	inline bool TileMap::IsWalkable(const glm::ivec2& gridPos) const
	{
		const Tile* tile = GetTile(gridPos);
		if (!tile) return false;
		ETileType primaryType = tile->GetPrimaryType();
		if (primaryType == ETileType::Empty) return false;
		if (primaryType == ETileType::Water) return false;
		if (primaryType == ETileType::Solid) return false;
		return true;
	}

	inline bool TileMap::IsInteractive(const glm::ivec2& gridPos) const
	{
		const Tile* tile = GetTile(gridPos);
		if (!tile) return false;
		ETileType primaryType = tile->GetPrimaryType();
		return primaryType != ETileType::Empty && primaryType != ETileType::Solid;
	}

	inline void TileMap::AddRegion(const Region& region)
	{
		m_Regions.push_back(region);
	}

	inline void TileMap::RemoveRegion(const std::string& regionName)
	{
		m_Regions.erase(
			std::remove_if(m_Regions.begin(), m_Regions.end(),
				[&regionName](const Region& r) { return r.GetName() == regionName; }),
			m_Regions.end()
		);
	}

	inline const Region* TileMap::GetRegionAt(const glm::ivec2& gridPos) const
	{
		for (const auto& region : m_Regions)
		{
			if (region.ContainsPoint(gridPos))
				return &region;
		}
		return nullptr;
	}

	inline const Region* TileMap::GetRegion(const std::string& regionName) const
	{
		for (const auto& region : m_Regions)
		{
			if (region.GetName() == regionName)
				return &region;
		}
		return nullptr;
	}

	inline void TileMap::AddPortal(const glm::ivec2& gridPos, const Portal& portal)
	{
		m_Portals[gridPos] = portal;
	}

	inline const Portal* TileMap::GetPortalAt(const glm::ivec2& gridPos) const
	{
		auto it = m_Portals.find(gridPos);
		return it != m_Portals.end() ? &it->second : nullptr;
	}

	inline void TileMap::RemovePortal(const glm::ivec2& gridPos)
	{
		m_Portals.erase(gridPos);
	}

	inline void TileMap::AddEntityAt(const glm::ivec2& gridPos, entt::entity entity)
	{
		if (IsValidPosition(gridPos))
			m_EntityGrid[gridPos].insert(entity);
	}

	inline void TileMap::RemoveEntityFrom(const glm::ivec2& gridPos, entt::entity entity)
	{
		auto it = m_EntityGrid.find(gridPos);
		if (it != m_EntityGrid.end())
		{
			it->second.erase(entity);
			if (it->second.empty())
				m_EntityGrid.erase(it);
		}
	}

	inline const std::set<entt::entity>& TileMap::GetEntitiesAt(const glm::ivec2& gridPos) const
	{
		static std::set<entt::entity> empty;
		auto it = m_EntityGrid.find(gridPos);
		return it != m_EntityGrid.end() ? it->second : empty;
	}

	inline std::set<entt::entity> TileMap::GetEntitiesInRect(const glm::ivec2& min, const glm::ivec2& max) const
	{
		std::set<entt::entity> result;
		auto tiles = GridQuery::GetTilesInRect(min, max);
		for (const auto& tile : tiles)
		{
			auto it = m_EntityGrid.find(tile);
			if (it != m_EntityGrid.end())
				result.insert(it->second.begin(), it->second.end());
		}
		return result;
	}

	inline void TileMap::SetTileDefinition(ETileType type, const TileDef& def)
	{
		m_TileDefinitions[type] = def;
	}

	inline const TileDef* TileMap::GetTileDefinition(ETileType type) const
	{
		auto it = m_TileDefinitions.find(type);
		return it != m_TileDefinitions.end() ? &it->second : nullptr;
	}

	inline void TileMap::Clear()
	{
		m_Chunks.clear();
		m_EntityGrid.clear();
		m_Regions.clear();
		m_Portals.clear();
		IncrementVersion();
	}

	inline void TileMap::Resize(const glm::ivec2& newSize)
	{
		m_MapSize = newSize;
		Clear();
	}

	inline Chunk* TileMap::GetChunk(const glm::ivec2& chunkCoord)
	{
		auto it = m_Chunks.find(chunkCoord);
		return it != m_Chunks.end() ? it->second.get() : nullptr;
	}

	inline const Chunk* TileMap::GetChunk(const glm::ivec2& chunkCoord) const
	{
		auto it = m_Chunks.find(chunkCoord);
		return it != m_Chunks.end() ? it->second.get() : nullptr;
	}

	inline void TileMap::EnsureChunkExists(const glm::ivec2& chunkCoord)
	{
		if (m_Chunks.find(chunkCoord) == m_Chunks.end())
		{
			m_Chunks[chunkCoord] = CreateRef<Chunk>(chunkCoord.x, chunkCoord.y);
		}
	}
}
