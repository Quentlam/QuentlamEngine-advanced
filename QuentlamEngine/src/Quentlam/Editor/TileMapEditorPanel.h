#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/World/TileMap.h"
#include "Quentlam/Renderer/TileMapRenderer.h"
#include <glm/glm.hpp>
#include <string>

namespace Quentlam
{
	class TileMapEditorPanel
	{
	public:
		TileMapEditorPanel();
		~TileMapEditorPanel() = default;

		void SetTileMap(TileMap* tileMap);
		void OnImGuiRender();

		void SetBrushType(ETileType type) { m_BrushType = type; }
		ETileType GetBrushType() const { return m_BrushType; }

		void SetBrushSize(int size) { m_BrushSize = size; }
		int GetBrushSize() const { return m_BrushSize; }

		void SetEditorTileSize(float size) { m_EditorTileSize = size; }
		float GetEditorTileSize() const { return m_EditorTileSize; }

		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }

		void HandleBrushStroke(const glm::ivec2& gridPos);
		void HandleBrushStrokeContinuous(const glm::ivec2& gridPos);
		void HandleBrushStrokeContinuousReset();

	private:
		void RenderTilePalette();
		void RenderBrushSettings();
		void RenderMapSettings();

		TileMap* m_TileMap = nullptr;
		TileMapRenderer m_Renderer;
		Ref<Texture2D> m_AtlasTexture;

		bool m_Visible = false;
		ETileType m_BrushType = ETileType::Grass;
		int m_BrushSize = 1;
		float m_EditorTileSize = 1.0f;
		bool m_LeftMouseDown = false;
		bool m_RightMouseDown = false;

		int m_MapWidth = 100;
		int m_MapHeight = 100;
		std::string m_MapName = "NewMap";

		glm::ivec2 m_LastPaintedTile = { INT_MAX, INT_MAX };

		static constexpr int PALETTE_COLS = 8;
	};
}
