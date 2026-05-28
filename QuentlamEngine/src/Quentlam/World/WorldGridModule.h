#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Entity.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <set>

namespace std
{
	template<>
	struct hash<glm::ivec2>
	{
		size_t operator()(const glm::ivec2& v) const noexcept
		{
			size_t h1 = std::hash<int>()(v.x);
			size_t h2 = std::hash<int>()(v.y);
			return h1 ^ (h2 << 16);
		}
	};

	template<>
	struct equal_to<glm::ivec2>
	{
		bool operator()(const glm::ivec2& a, const glm::ivec2& b) const noexcept
		{
			return a.x == b.x && a.y == b.y;
		}
	};
}

namespace Quentlam
{
	enum class ETileType : uint8_t
	{
		Empty = 0,
		Solid = 1,
		Grass = 2,
		Water = 3,
		Sand = 4,
		Stone = 5,
		Wood = 6,
		Custom = 100
	};

	enum class ETileCollision : uint8_t
	{
		None = 0,
		Block = 1,
		OneWay = 2,
		Platform = 3
	};

	enum class ETileLayer : uint8_t
	{
		Ground = 0,
		Objects = 1,
		Water = 2,
		Weather = 3,
		Count = 4
	};

	enum class ERegionType : uint8_t
	{
		Outdoor = 0,
		Indoor = 1,
		Cave = 2,
		Building = 3
	};

	struct TileLayerData
	{
		ETileType Type = ETileType::Empty;
		int32_t Variant = 0;
		uint8_t Variation = 0;

		bool IsActive() const { return Type != ETileType::Empty; }
		void Clear() { Type = ETileType::Empty; Variant = 0; Variation = 0; }
	};

	struct TileDef
	{
		uint32_t ID = 0;
		std::string Name;
		std::string TexturePath;
		ETileType Type = ETileType::Empty;
		ETileCollision Collision = ETileCollision::None;
		glm::vec2 UVOffset = { 0.0f, 0.0f };
		glm::vec2 UVSize = { 1.0f, 1.0f };
		glm::ivec2 AtlasOffset = { 0, 0 };
		glm::ivec2 TileSize = { 16, 16 };
		int32_t TextureID = -1;
		bool Walkable = true;
		bool UVLock = false;
		float WalkSpeedMultiplier = 1.0f;
		bool Interactive = false;
		std::string CustomData;
	};

	struct Tile
	{
		TileLayerData Layers[static_cast<size_t>(ETileLayer::Count)];

		TileLayerData& operator[](ETileLayer layer) { return Layers[static_cast<size_t>(layer)]; }
		const TileLayerData& operator[](ETileLayer layer) const { return Layers[static_cast<size_t>(layer)]; }

		TileLayerData& Ground() { return Layers[static_cast<size_t>(ETileLayer::Ground)]; }
		TileLayerData& Objects() { return Layers[static_cast<size_t>(ETileLayer::Objects)]; }
		TileLayerData& Water() { return Layers[static_cast<size_t>(ETileLayer::Water)]; }
		TileLayerData& Weather() { return Layers[static_cast<size_t>(ETileLayer::Weather)]; }
		const TileLayerData& Ground() const { return Layers[static_cast<size_t>(ETileLayer::Ground)]; }
		const TileLayerData& Objects() const { return Layers[static_cast<size_t>(ETileLayer::Objects)]; }
		const TileLayerData& Water() const { return Layers[static_cast<size_t>(ETileLayer::Water)]; }
		const TileLayerData& Weather() const { return Layers[static_cast<size_t>(ETileLayer::Weather)]; }

		bool IsHighlighted = false;
		bool IsLocked = false;

		ETileType GetPrimaryType() const;
		ETileLayer GetPrimaryLayer() const;
		bool IsLayerEmpty(ETileLayer layer) const { return !Layers[static_cast<size_t>(layer)].IsActive(); }
		bool IsAllEmpty() const;
		void Clear();
		void ClearLayer(ETileLayer layer);
	};

	class Chunk
	{
	public:
		static constexpr int32_t Size = 16;

		Chunk() = default;
		Chunk(int32_t x, int32_t y);

		void SetTile(int32_t localX, int32_t localY, const Tile& tile);
		Tile* GetTile(int32_t localX, int32_t localY);
		const Tile* GetTile(int32_t localX, int32_t localY) const;

		glm::ivec2 GetChunkCoord() const { return m_ChunkCoord; }
		bool IsDirty() const { return m_IsDirty; }
		void MarkClean() { m_IsDirty = false; }
		void MarkDirty() { m_IsDirty = true; }

	private:
		glm::ivec2 m_ChunkCoord = { 0, 0 };
		Tile m_Tiles[Size][Size];
		bool m_IsDirty = true;
	};

	class Portal
	{
	public:
		Portal() = default;
		Portal(const std::string& targetMap, const glm::ivec2& targetPos, const std::string& targetPortalId = "");

		const std::string& GetTargetMap() const { return m_TargetMap; }
		const glm::ivec2& GetTargetPos() const { return m_TargetPos; }
		const std::string& GetTargetPortalId() const { return m_TargetPortalId; }
		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }

	private:
		std::string m_Id;
		std::string m_TargetMap;
		glm::ivec2 m_TargetPos = { 0, 0 };
		std::string m_TargetPortalId;
	};

	class Region
	{
	public:
		Region() = default;
		Region(const std::string& name, const glm::ivec2& boundsMin, const glm::ivec2& boundsMax);

		const std::string& GetName() const { return m_Name; }
		const glm::ivec2& GetBoundsMin() const { return m_BoundsMin; }
		const glm::ivec2& GetBoundsMax() const { return m_BoundsMax; }
		ERegionType GetRegionType() const { return m_Type; }
		void SetRegionType(ERegionType type) { m_Type = type; }

		bool ContainsPoint(const glm::ivec2& point) const;
		bool IsOverlapping(const Region& other) const;

		void AddPortal(const Portal& portal);
		const Portal* GetPortal(const std::string& portalId) const;

	private:
		std::string m_Name;
		glm::ivec2 m_BoundsMin = { 0, 0 };
		glm::ivec2 m_BoundsMax = { 0, 0 };
		ERegionType m_Type = ERegionType::Outdoor;
		std::unordered_map<std::string, Portal> m_Portals;
	};

	class GridQuery
	{
	public:
		GridQuery() = default;

		static glm::ivec2 WorldToGrid(const glm::vec2& worldPos, float tileSize);
		static glm::vec2 GridToWorld(const glm::ivec2& gridPos, float tileSize);

		static glm::ivec2 GetChunkCoord(const glm::ivec2& gridPos);
		static glm::ivec2 GetLocalTileCoord(const glm::ivec2& gridPos);

		static bool IsValidGridPos(const glm::ivec2& gridPos);

		static std::vector<glm::ivec2> GetNeighbors4(const glm::ivec2& gridPos);
		static std::vector<glm::ivec2> GetNeighbors8(const glm::ivec2& gridPos);

		static std::vector<glm::ivec2> GetTilesInRect(const glm::ivec2& min, const glm::ivec2& max);
		static std::vector<glm::ivec2> GetTilesInRadius(const glm::ivec2& center, int32_t radius);
	};

	inline Chunk::Chunk(int32_t x, int32_t y)
		: m_ChunkCoord(x, y), m_IsDirty(true)
	{
		for (int y = 0; y < Size; ++y)
			for (int x = 0; x < Size; ++x)
				m_Tiles[y][x] = Tile{};
	}

	inline void Chunk::SetTile(int32_t localX, int32_t localY, const Tile& tile)
	{
		if (localX >= 0 && localX < Size && localY >= 0 && localY < Size)
		{
			m_Tiles[localY][localX] = tile;
			m_IsDirty = true;
		}
	}

	inline Tile* Chunk::GetTile(int32_t localX, int32_t localY)
	{
		if (localX >= 0 && localX < Size && localY >= 0 && localY < Size)
			return &m_Tiles[localY][localX];
		return nullptr;
	}

	inline const Tile* Chunk::GetTile(int32_t localX, int32_t localY) const
	{
		if (localX >= 0 && localX < Size && localY >= 0 && localY < Size)
			return &m_Tiles[localY][localX];
		return nullptr;
	}

	inline Portal::Portal(const std::string& targetMap, const glm::ivec2& targetPos, const std::string& targetPortalId)
		: m_TargetMap(targetMap), m_TargetPos(targetPos), m_TargetPortalId(targetPortalId) {}

	inline Region::Region(const std::string& name, const glm::ivec2& boundsMin, const glm::ivec2& boundsMax)
		: m_Name(name), m_BoundsMin(boundsMin), m_BoundsMax(boundsMax) {}

	inline bool Region::ContainsPoint(const glm::ivec2& point) const
	{
		return point.x >= m_BoundsMin.x && point.x <= m_BoundsMax.x &&
			   point.y >= m_BoundsMin.y && point.y <= m_BoundsMax.y;
	}

	inline bool Region::IsOverlapping(const Region& other) const
	{
		return !(m_BoundsMax.x < other.m_BoundsMin.x || m_BoundsMin.x > other.m_BoundsMax.x ||
				 m_BoundsMax.y < other.m_BoundsMin.y || m_BoundsMin.y > other.m_BoundsMax.y);
	}

	inline void Region::AddPortal(const Portal& portal)
	{
		m_Portals[portal.GetId()] = portal;
	}

	inline const Portal* Region::GetPortal(const std::string& portalId) const
	{
		auto it = m_Portals.find(portalId);
		return it != m_Portals.end() ? &it->second : nullptr;
	}

	inline glm::ivec2 GridQuery::WorldToGrid(const glm::vec2& worldPos, float tileSize)
	{
		return { static_cast<int32_t>(std::floor(worldPos.x / tileSize)),
				 static_cast<int32_t>(std::floor(worldPos.y / tileSize)) };
	}

	inline glm::vec2 GridQuery::GridToWorld(const glm::ivec2& gridPos, float tileSize)
	{
		return { static_cast<float>(gridPos.x) * tileSize,
				 static_cast<float>(gridPos.y) * tileSize };
	}

	inline glm::ivec2 GridQuery::GetChunkCoord(const glm::ivec2& gridPos)
	{
		return { gridPos.x < 0 ? (gridPos.x + 1) / Chunk::Size - 1 : gridPos.x / Chunk::Size,
				 gridPos.y < 0 ? (gridPos.y + 1) / Chunk::Size - 1 : gridPos.y / Chunk::Size };
	}

	inline glm::ivec2 GridQuery::GetLocalTileCoord(const glm::ivec2& gridPos)
	{
		glm::ivec2 chunkCoord = GetChunkCoord(gridPos);
		int32_t baseX = chunkCoord.x * Chunk::Size;
		int32_t baseY = chunkCoord.y * Chunk::Size;
		int32_t localX = gridPos.x - baseX;
		int32_t localY = gridPos.y - baseY;
		if (localX < 0) localX += Chunk::Size;
		if (localY < 0) localY += Chunk::Size;
		return { localX, localY };
	}

	inline bool GridQuery::IsValidGridPos(const glm::ivec2& gridPos)
	{
		return gridPos.x != INT32_MIN && gridPos.y != INT32_MIN;
	}

	inline std::vector<glm::ivec2> GridQuery::GetNeighbors4(const glm::ivec2& gridPos)
	{
		return {
			{ gridPos.x, gridPos.y + 1 },
			{ gridPos.x + 1, gridPos.y },
			{ gridPos.x, gridPos.y - 1 },
			{ gridPos.x - 1, gridPos.y }
		};
	}

	inline std::vector<glm::ivec2> GridQuery::GetNeighbors8(const glm::ivec2& gridPos)
	{
		return {
			{ gridPos.x, gridPos.y + 1 },
			{ gridPos.x + 1, gridPos.y + 1 },
			{ gridPos.x + 1, gridPos.y },
			{ gridPos.x + 1, gridPos.y - 1 },
			{ gridPos.x, gridPos.y - 1 },
			{ gridPos.x - 1, gridPos.y - 1 },
			{ gridPos.x - 1, gridPos.y },
			{ gridPos.x - 1, gridPos.y + 1 }
		};
	}

	inline std::vector<glm::ivec2> GridQuery::GetTilesInRect(const glm::ivec2& min, const glm::ivec2& max)
	{
		std::vector<glm::ivec2> tiles;
		for (int32_t y = min.y; y <= max.y; ++y)
			for (int32_t x = min.x; x <= max.x; ++x)
				tiles.push_back({ x, y });
		return tiles;
	}

	inline std::vector<glm::ivec2> GridQuery::GetTilesInRadius(const glm::ivec2& center, int32_t radius)
	{
		std::vector<glm::ivec2> tiles;
		for (int32_t y = center.y - radius; y <= center.y + radius; ++y)
			for (int32_t x = center.x - radius; x <= center.x + radius; ++x)
				if ((x - center.x) * (x - center.x) + (y - center.y) * (y - center.y) <= radius * radius)
					tiles.push_back({ x, y });
		return tiles;
	}

	inline ETileType Tile::GetPrimaryType() const
	{
		for (size_t i = 0; i < static_cast<size_t>(ETileLayer::Count); ++i)
		{
			if (Layers[i].IsActive())
				return Layers[i].Type;
		}
		return ETileType::Empty;
	}

	inline ETileLayer Tile::GetPrimaryLayer() const
	{
		for (size_t i = 0; i < static_cast<size_t>(ETileLayer::Count); ++i)
		{
			if (Layers[i].IsActive())
				return static_cast<ETileLayer>(i);
		}
		return ETileLayer::Ground;
	}

	inline bool Tile::IsAllEmpty() const
	{
		for (size_t i = 0; i < static_cast<size_t>(ETileLayer::Count); ++i)
		{
			if (Layers[i].IsActive())
				return false;
		}
		return true;
	}

	inline void Tile::Clear()
	{
		for (size_t i = 0; i < static_cast<size_t>(ETileLayer::Count); ++i)
			Layers[i].Clear();
		IsHighlighted = false;
		IsLocked = false;
	}

	inline void Tile::ClearLayer(ETileLayer layer)
	{
		Layers[static_cast<size_t>(layer)].Clear();
	}
}
