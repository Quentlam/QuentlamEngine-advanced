#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/World/TileMap.h"
#include "Quentlam/Renderer/Camera.h"
#include "Quentlam/Renderer/OrthographicCamera.h"
#include "Quentlam/Renderer/PerspectiveCamera.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Renderer/Renderer2D.h"
#include <glm/glm.hpp>

namespace Quentlam
{
	class QUENTLAM_API TileMapRenderer
	{
	public:
		TileMapRenderer() = default;
		~TileMapRenderer() = default;

		void Render(TileMap* tileMap, Camera& camera);
		void RenderRegion(TileMap* tileMap, const glm::ivec2& min, const glm::ivec2& max, Camera& camera);
		void RenderLayer(TileMap* tileMap, ETileLayer layer, Camera& camera, bool useBlending = false);

		void SetAtlasTexture(Ref<Texture2D> texture, const glm::ivec2& atlasTileCount = { 16, 16 });
		void SetDefaultTileTexture(Ref<Texture2D> texture) { m_DefaultTileTexture = texture; }
		Ref<Texture2D> GetDefaultTileTexture() const { return m_DefaultTileTexture; }
		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		void SetTileSize(float size) { m_TileRenderSize = size; }
		float GetTileSize() const { return m_TileRenderSize; }

		void SetLayerEnabled(ETileLayer layer, bool enabled);
		bool IsLayerEnabled(ETileLayer layer) const;
		void SetAllLayersEnabled(bool enabled);

		void SetRenderLayers(bool ground, bool objects, bool water, bool weather = true)
		{
			SetLayerEnabled(ETileLayer::Ground, ground);
			SetLayerEnabled(ETileLayer::Objects, objects);
			SetLayerEnabled(ETileLayer::Water, water);
			SetLayerEnabled(ETileLayer::Weather, weather);
		}

		void SetVisibleArea(const glm::vec2& worldMin, const glm::vec2& worldMax)
		{
			m_VisibleMin = worldMin;
			m_VisibleMax = worldMax;
			m_UseVisibleArea = true;
		}
		void ClearVisibleArea() { m_UseVisibleArea = false; }

		void SetHighlightColor(const glm::vec4& color) { m_HighlightColor = color; }
		const glm::vec4& GetHighlightColor() const { return m_HighlightColor; }

		void SetHighlightTiles(const std::vector<glm::ivec2>& tiles) { m_HighlightTiles = tiles; }
		void ClearHighlights() { m_HighlightTiles.clear(); }

		void SetCullingEnabled(bool enabled) { m_CullingEnabled = enabled; }
		bool IsCullingEnabled() const { return m_CullingEnabled; }

		bool HasAtlasTexture() const { return m_AtlasTexture != nullptr; }

	private:
		void RenderChunk(TileMap* tileMap, Chunk* chunk, ETileLayer layer, Camera& camera, bool useBlending);
		void RenderRegionLayer(TileMap* tileMap, ETileLayer layer, const glm::ivec2& min, const glm::ivec2& max, Camera& camera, bool useBlending);
		bool IsChunkVisible(Chunk* chunk, Camera& camera) const;
		glm::vec4 GetColorForTile(const TileLayerData& layerData, ETileLayer layer) const;
		bool IsTileHighlighted(const glm::ivec2& pos) const;

		glm::vec2 GetUVForTileType(ETileType type, ETileLayer layer) const;
		glm::vec2 GetUVFromAtlas(int atlasX, int atlasY) const;

		void DrawTile(const glm::vec2& worldPos, const TileLayerData& layerData, ETileLayer layer, Camera& camera, bool useBlending);

		Ref<Texture2D> m_AtlasTexture;
		Ref<Texture2D> m_DefaultTileTexture;
		float m_TileRenderSize = 1.0f;
		glm::ivec2 m_AtlasTileCount = { 16, 16 };
		glm::vec2 m_UVStep = { 1.0f / 16.0f, 1.0f / 16.0f };

		bool m_LayerEnabled[4] = { true, true, true, true };

		bool m_CullingEnabled = true;

		glm::vec2 m_VisibleMin = { -50.0f, -50.0f };
		glm::vec2 m_VisibleMax = { 50.0f, 50.0f };
		bool m_UseVisibleArea = false;

		glm::vec4 m_HighlightColor = { 1.0f, 1.0f, 0.0f, 0.3f };
		std::vector<glm::ivec2> m_HighlightTiles;
	};

	inline void TileMapRenderer::SetAtlasTexture(Ref<Texture2D> texture, const glm::ivec2& atlasTileCount)
	{
		m_AtlasTexture = texture;
		m_AtlasTileCount = atlasTileCount;
		m_UVStep = {
			1.0f / static_cast<float>(atlasTileCount.x),
			1.0f / static_cast<float>(atlasTileCount.y)
		};
	}

	inline void TileMapRenderer::SetLayerEnabled(ETileLayer layer, bool enabled)
	{
		m_LayerEnabled[static_cast<size_t>(layer)] = enabled;
	}

	inline bool TileMapRenderer::IsLayerEnabled(ETileLayer layer) const
	{
		return m_LayerEnabled[static_cast<size_t>(layer)];
	}

	inline void TileMapRenderer::SetAllLayersEnabled(bool enabled)
	{
		for (size_t i = 0; i < 4; ++i)
			m_LayerEnabled[i] = enabled;
	}

	inline void TileMapRenderer::Render(TileMap* tileMap, Camera& camera)
	{
		if (!tileMap) return;
		m_TileRenderSize = tileMap->GetTileSize();

		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Ground)])
			RenderLayer(tileMap, ETileLayer::Ground, camera, false);

		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Objects)])
			RenderLayer(tileMap, ETileLayer::Objects, camera, false);

		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Water)])
			RenderLayer(tileMap, ETileLayer::Water, camera, true);

		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Weather)])
			RenderLayer(tileMap, ETileLayer::Weather, camera, true);
	}

	inline void TileMapRenderer::RenderLayer(TileMap* tileMap, ETileLayer layer, Camera& camera, bool useBlending)
	{
		if (!tileMap) return;
		m_TileRenderSize = tileMap->GetTileSize();

		const auto& chunks = tileMap->GetAllChunks();
		for (auto& [coord, chunk] : chunks)
		{
			if (!chunk) continue;
			if (m_CullingEnabled && !IsChunkVisible(chunk.get(), camera))
				continue;
			RenderChunk(tileMap, chunk.get(), layer, camera, useBlending);
			chunk->MarkClean();
		}
	}

	inline void TileMapRenderer::RenderRegion(TileMap* tileMap, const glm::ivec2& min, const glm::ivec2& max, Camera& camera)
	{
		if (!tileMap) return;
		m_TileRenderSize = tileMap->GetTileSize();

		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Ground)])
			RenderRegionLayer(tileMap, ETileLayer::Ground, min, max, camera, false);
		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Objects)])
			RenderRegionLayer(tileMap, ETileLayer::Objects, min, max, camera, false);
		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Water)])
			RenderRegionLayer(tileMap, ETileLayer::Water, min, max, camera, true);
		if (m_LayerEnabled[static_cast<size_t>(ETileLayer::Weather)])
			RenderRegionLayer(tileMap, ETileLayer::Weather, min, max, camera, true);
	}

	inline void TileMapRenderer::RenderRegionLayer(TileMap* tileMap, ETileLayer layer, const glm::ivec2& min, const glm::ivec2& max, Camera& camera, bool useBlending)
	{
		if (!tileMap) return;

		glm::ivec2 clampedMin = { std::max(0, min.x), std::max(0, min.y) };
		glm::ivec2 clampedMax = {
			std::min(tileMap->GetMapSize().x - 1, max.x),
			std::min(tileMap->GetMapSize().y - 1, max.y)
		};

		for (int32_t y = clampedMin.y; y <= clampedMax.y; ++y)
		{
			for (int32_t x = clampedMin.x; x <= clampedMax.x; ++x)
			{
				glm::ivec2 pos(x, y);
				Tile* tile = tileMap->GetTile(pos);
				if (!tile) continue;

				const TileLayerData& layerData = (*tile)[layer];
				if (!layerData.IsActive()) continue;

				glm::vec2 worldPos = GridQuery::GridToWorld(pos, m_TileRenderSize);

				if (m_CullingEnabled)
				{
					glm::vec2 camPos = camera.GetPosition();
					if (worldPos.x + m_TileRenderSize < camPos.x - 20 || worldPos.x > camPos.x + 20 ||
						worldPos.y + m_TileRenderSize < camPos.y - 20 || worldPos.y > camPos.y + 20)
						continue;
				}

				DrawTile(worldPos, layerData, layer, camera, useBlending);
			}
		}
	}

	inline void TileMapRenderer::RenderChunk(TileMap* tileMap, Chunk* chunk, ETileLayer layer, Camera& camera, bool useBlending)
	{
		if (!tileMap || !chunk) return;

		glm::ivec2 chunkCoord = chunk->GetChunkCoord();
		glm::ivec2 baseTile = { chunkCoord.x * Chunk::Size, chunkCoord.y * Chunk::Size };

		glm::vec2 camPos = camera.GetPosition();
		float camRange = 20.0f;

		for (int ly = 0; ly < Chunk::Size; ++ly)
		{
			for (int lx = 0; lx < Chunk::Size; ++lx)
			{
				Tile* tile = chunk->GetTile(lx, ly);
				if (!tile) continue;

				glm::ivec2 worldTile = { baseTile.x + lx, baseTile.y + ly };
				if (!tileMap->IsValidPosition(worldTile)) continue;

				const TileLayerData& layerData = (*tile)[layer];
				if (!layerData.IsActive()) continue;

				glm::vec2 worldPos = GridQuery::GridToWorld(worldTile, m_TileRenderSize);

				if (m_CullingEnabled)
				{
					if (worldPos.x + m_TileRenderSize < camPos.x - camRange ||
						worldPos.x > camPos.x + camRange ||
						worldPos.y + m_TileRenderSize < camPos.y - camRange ||
						worldPos.y > camPos.y + camRange)
						continue;
				}

				DrawTile(worldPos, layerData, layer, camera, useBlending);
			}
		}
	}

	inline void TileMapRenderer::DrawTile(const glm::vec2& worldPos, const TileLayerData& layerData, ETileLayer layer, Camera& camera, bool useBlending)
	{
		bool isHighlighted = layerData.IsActive() && IsTileHighlighted(glm::ivec2(
			static_cast<int>(std::round(worldPos.x / m_TileRenderSize)),
			static_cast<int>(std::round(worldPos.y / m_TileRenderSize))));

		glm::vec4 color = GetColorForTile(layerData, layer);

		if (isHighlighted)
			color = m_HighlightColor;

		float zOffset = 0.0f;
		if (layer == ETileLayer::Water)
			zOffset = -0.01f;
		else if (layer == ETileLayer::Weather)
			zOffset = 0.01f;
		else if (layer == ETileLayer::Objects)
			zOffset = 0.005f;

		glm::vec3 drawPos = { worldPos.x + m_TileRenderSize * 0.5f, worldPos.y + m_TileRenderSize * 0.5f, zOffset };

		if (m_AtlasTexture)
		{
			glm::vec2 uv = GetUVForTileType(layerData.Type, layer);
			Renderer2D::DrawQuad(drawPos, { m_TileRenderSize, m_TileRenderSize }, m_AtlasTexture, 1.0f, color);
		}
		else
		{
			Renderer2D::DrawQuad(drawPos, { m_TileRenderSize, m_TileRenderSize }, color);
		}
	}

	inline bool TileMapRenderer::IsTileHighlighted(const glm::ivec2& pos) const
	{
		for (const auto& h : m_HighlightTiles)
			if (h.x == pos.x && h.y == pos.y)
				return true;
		return false;
	}

	inline glm::vec2 TileMapRenderer::GetUVForTileType(ETileType type, ETileLayer layer) const
	{
		if (layer == ETileLayer::Ground)
		{
			switch (type)
			{
			case ETileType::Grass:  return GetUVFromAtlas(0, 0);
			case ETileType::Sand:   return GetUVFromAtlas(2, 0);
			case ETileType::Stone:  return GetUVFromAtlas(3, 0);
			case ETileType::Wood:   return GetUVFromAtlas(4, 0);
			case ETileType::Solid:  return GetUVFromAtlas(5, 0);
			default:                 return GetUVFromAtlas(15, 15);
			}
		}
		else if (layer == ETileLayer::Objects)
		{
			switch (type)
			{
			case ETileType::Solid:  return GetUVFromAtlas(6, 0);
			case ETileType::Grass:  return GetUVFromAtlas(0, 1);
			case ETileType::Wood:   return GetUVFromAtlas(4, 1);
			default:                 return GetUVFromAtlas(15, 1);
			}
		}
		else if (layer == ETileLayer::Water)
		{
			switch (type)
			{
			case ETileType::Water:  return GetUVFromAtlas(1, 0);
			default:                 return GetUVFromAtlas(1, 0);
			}
		}
		else if (layer == ETileLayer::Weather)
		{
			switch (type)
			{
			case ETileType::Water:  return GetUVFromAtlas(1, 1);
			case ETileType::Grass:  return GetUVFromAtlas(0, 2);
			default:                 return GetUVFromAtlas(15, 15);
			}
		}
		return GetUVFromAtlas(15, 15);
	}

	inline glm::vec2 TileMapRenderer::GetUVFromAtlas(int atlasX, int atlasY) const
	{
		atlasX = std::max(0, std::min(atlasX, m_AtlasTileCount.x - 1));
		atlasY = std::max(0, std::min(atlasY, m_AtlasTileCount.y - 1));
		return {
			static_cast<float>(atlasX) * m_UVStep.x,
			static_cast<float>(atlasY) * m_UVStep.y
		};
	}

	inline bool TileMapRenderer::IsChunkVisible(Chunk* chunk, Camera& camera) const
	{
		if (!chunk) return false;
		glm::ivec2 cc = chunk->GetChunkCoord();
		glm::vec2 minWorld = GridQuery::GridToWorld({ cc.x * Chunk::Size, cc.y * Chunk::Size }, m_TileRenderSize);
		glm::vec2 maxWorld = GridQuery::GridToWorld({ (cc.x + 1) * Chunk::Size, (cc.y + 1) * Chunk::Size }, m_TileRenderSize);
		glm::vec2 camPos = camera.GetPosition();
		float range = Chunk::Size * m_TileRenderSize + 5.0f;
		return !(maxWorld.x < camPos.x - range || minWorld.x > camPos.x + range ||
				 maxWorld.y < camPos.y - range || minWorld.y > camPos.y + range);
	}

	inline glm::vec4 TileMapRenderer::GetColorForTile(const TileLayerData& layerData, ETileLayer layer) const
	{
		switch (layer)
		{
		case ETileLayer::Ground:
		{
			switch (layerData.Type)
			{
			case ETileType::Grass: return { 0.3f, 0.7f, 0.2f, 1.0f };
			case ETileType::Sand:  return { 0.9f, 0.8f, 0.5f, 1.0f };
			case ETileType::Stone: return { 0.5f, 0.5f, 0.5f, 1.0f };
			case ETileType::Wood:  return { 0.5f, 0.3f, 0.1f, 1.0f };
			case ETileType::Solid: return { 0.4f, 0.4f, 0.4f, 1.0f };
			default:                return { 0.8f, 0.8f, 0.8f, 1.0f };
			}
		}
		case ETileLayer::Objects:
		{
			switch (layerData.Type)
			{
			case ETileType::Solid: return { 0.4f, 0.4f, 0.4f, 1.0f };
			case ETileType::Grass: return { 0.2f, 0.5f, 0.15f, 1.0f };
			case ETileType::Wood:  return { 0.4f, 0.25f, 0.1f, 1.0f };
			default:                return { 0.0f, 0.0f, 0.0f, 0.0f };
			}
		}
		case ETileLayer::Water:
		{
			switch (layerData.Type)
			{
			case ETileType::Water: return { 0.2f, 0.4f, 0.9f, 0.7f };
			default:                return { 0.0f, 0.0f, 0.0f, 0.0f };
			}
		}
		case ETileLayer::Weather:
		{
			switch (layerData.Type)
			{
			case ETileType::Water: return { 0.3f, 0.3f, 0.5f, 0.5f };
			default:                return { 0.0f, 0.0f, 0.0f, 0.0f };
			}
		}
		default:
			return { 0.8f, 0.8f, 0.8f, 1.0f };
		}
	}
}
