#include "qlpch.h"
#include "TileMapEditorPanel.h"
#include "Quentlam/Core/Application.h"
#include <imgui.h>

namespace Quentlam
{
	TileMapEditorPanel::TileMapEditorPanel()
	{
	}

	void TileMapEditorPanel::SetTileMap(TileMap* tileMap)
	{
		m_TileMap = tileMap;
		if (tileMap)
		{
			m_EditorTileSize = tileMap->GetTileSize();
			m_MapWidth = tileMap->GetMapSize().x;
			m_MapHeight = tileMap->GetMapSize().y;
			m_MapName = tileMap->GetName();
		}
	}

	void TileMapEditorPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::SetNextWindowSize(ImVec2(350.0f, 500.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("瓦片地图编辑器##TileMapEditor", &m_Visible))
		{
			ImGui::End();
			return;
		}

		ImGui::Text("工具栏");
		ImGui::Separator();
		RenderToolBar();

		ImGui::Spacing();
		ImGui::Text("瓦片调色板");
		ImGui::Separator();
		RenderTilePalette();

		ImGui::Spacing();
		ImGui::Text("画笔设置");
		ImGui::Separator();
		RenderBrushSettings();

		ImGui::Spacing();
		ImGui::Text("地图设置");
		ImGui::Separator();
		RenderMapSettings();

		ImGui::End();
	}

	void TileMapEditorPanel::RenderTilePalette()
	{
		const char* tileNames[] = {
			"Empty", "Solid", "Grass", "Water", "Sand", "Stone", "Wood"
		};
		int tileCount = 7;

		ImGui::Text("选择贴图:");
		ImGui::Spacing();

		if (m_AtlasTexture)
		{
			ImGui::Text("图集已加载 (%dx%d)", m_AtlasTexture->GetWidth(), m_AtlasTexture->GetHeight());
			ImGui::Spacing();
		}
		else
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "未加载图集 - 使用纯色渲染");
			if (ImGui::Button("加载贴图图集"))
			{
				QL_CORE_INFO("请通过 Asset Browser 拖拽贴图到 TileMapRenderer");
			}
			ImGui::Spacing();
		}

		ImGui::Text("选择地块类型:");
		for (int i = 0; i < tileCount; ++i)
		{
			ETileType type = static_cast<ETileType>(i);
			bool selected = (m_BrushType == type);

			ImVec4 tileColor(0.5f, 0.5f, 0.5f, 1.0f);
			switch (type)
			{
			case ETileType::Empty:   tileColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
			case ETileType::Solid:   tileColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); break;
			case ETileType::Grass:   tileColor = ImVec4(0.3f, 0.7f, 0.2f, 1.0f); break;
			case ETileType::Water:   tileColor = ImVec4(0.2f, 0.4f, 0.9f, 0.8f); break;
			case ETileType::Sand:    tileColor = ImVec4(0.9f, 0.8f, 0.5f, 1.0f); break;
			case ETileType::Stone:   tileColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
			case ETileType::Wood:    tileColor = ImVec4(0.5f, 0.3f, 0.1f, 1.0f); break;
			}

			ImGui::PushID(i);
			ImVec4 textColor = selected ? ImVec4(1.0f, 0.9f, 0.3f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.3f, 0.3f, 0.2f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.3f, 1.0f));

			char label[32];
			snprintf(label, sizeof(label), "%s###tile_btn_%d", tileNames[i], i);

			if (ImGui::Button(label, ImVec2(80.0f, 28.0f)))
			{
				m_BrushType = type;
			}

			ImGui::SameLine();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 boxMin = ImVec2(cursor.x - 32.0f, cursor.y - 28.0f);
			ImVec2 boxMax = ImVec2(boxMin.x + 20.0f, boxMin.y + 20.0f);
			ImU32 col = ImGui::ColorConvertFloat4ToU32(tileColor);
			drawList->AddRectFilled(boxMin, boxMax, col);
			drawList->AddRect(boxMin, boxMax, IM_COL32(100, 100, 100, 255));

			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}
	}

	void TileMapEditorPanel::RenderBrushSettings()
	{
		const char* brushSizes[] = { "1x1", "2x2", "3x3", "5x5" };
		int brushIndex = 0;
		switch (m_BrushSize)
		{
		case 1: brushIndex = 0; break;
		case 2: brushIndex = 1; break;
		case 3: brushIndex = 2; break;
		case 5: brushIndex = 3; break;
		}

		ImGui::Text("画笔大小:");
		ImGui::SameLine();
		if (ImGui::Combo("##BrushSize", &brushIndex, brushSizes, 4))
		{
			switch (brushIndex)
			{
			case 0: m_BrushSize = 1; break;
			case 1: m_BrushSize = 2; break;
			case 2: m_BrushSize = 3; break;
			case 3: m_BrushSize = 5; break;
			}
		}

		ImGui::Text("地块大小: %.2f", m_EditorTileSize);
	}

	void TileMapEditorPanel::RenderMapSettings()
	{
		char mapNameBuf[128] = {};
		strncpy_s(mapNameBuf, m_MapName.c_str(), sizeof(mapNameBuf) - 1);
		if (ImGui::InputText("地图名称", mapNameBuf, sizeof(mapNameBuf)))
		{
			m_MapName = mapNameBuf;
		}
		ImGui::InputInt("地图宽度", &m_MapWidth, 1, 10);
		ImGui::InputInt("地图高度", &m_MapHeight, 1, 10);

		m_MapWidth = std::max(10, std::min(1000, m_MapWidth));
		m_MapHeight = std::max(10, std::min(1000, m_MapHeight));

		if (ImGui::Button("应用设置"))
		{
			if (m_TileMap)
			{
				m_TileMap->SetName(m_MapName);
				m_TileMap->Resize(glm::ivec2(m_MapWidth, m_MapHeight));
				m_TileMap->SetTileSize(m_EditorTileSize);
				QL_CORE_INFO("TileMap resized to {0}x{1}", m_MapWidth, m_MapHeight);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("清空地图"))
		{
			if (m_TileMap)
			{
				m_TileMap->Clear();
				QL_CORE_INFO("TileMap cleared");
			}
		}
	}

	void TileMapEditorPanel::RenderToolBar()
	{
		const char* toolNames[] = { "Brush", "Eraser", "Fill", "Picker" };
		for (int i = 0; i < 4; ++i)
		{
			bool selected = (m_CurrentTool == static_cast<ETileMapTool>(i));
			ImGui::PushID(i);
			if (ImGui::Button(toolNames[i], ImVec2(70, 24)))
				m_CurrentTool = static_cast<ETileMapTool>(i);
			if (selected)
				ImGui::SameLine();
			ImGui::PopID();
		}
	}

	void TileMapEditorPanel::FloodFill(const glm::ivec2& pos, ETileType newType)
	{
		if (!m_TileMap || !m_TileMap->IsValidPosition(pos))
			return;

		TileLayerData* tileData = m_TileMap->GetTileLayer(pos, ETileLayer::Ground);
		if (!tileData)
			return;

		ETileType targetType = tileData->Type;

		if (targetType == newType)
			return;

		std::stack<glm::ivec2> stack;
		stack.push(pos);

		while (!stack.empty())
		{
			glm::ivec2 current = stack.top();
			stack.pop();

			if (!m_TileMap->IsValidPosition(current))
				continue;

			TileLayerData* td = m_TileMap->GetTileLayer(current, ETileLayer::Ground);
			if (!td || td->Type != targetType)
				continue;

			m_TileMap->SetTileLayer(current, ETileLayer::Ground, newType);

			stack.emplace(current.x + 1, current.y);
			stack.emplace(current.x - 1, current.y);
			stack.emplace(current.x, current.y + 1);
			stack.emplace(current.x, current.y - 1);
		}
	}

	void TileMapEditorPanel::HandleBrushStroke(const glm::ivec2& gridPos)
	{
		if (!m_TileMap) return;

		switch (m_CurrentTool)
		{
		case ETileMapTool::Eraser:
		{
			int half = m_BrushSize / 2;
			for (int dy = -half; dy <= half; ++dy)
			{
				for (int dx = -half; dx <= half; ++dx)
				{
					glm::ivec2 pos(gridPos.x + dx, gridPos.y + dy);
					if (m_TileMap->IsValidPosition(pos))
						m_TileMap->SetTileType(pos, ETileType::Empty);
				}
			}
			break;
		}
		case ETileMapTool::Fill:
		{
			FloodFill(gridPos, m_BrushType);
			break;
		}
		case ETileMapTool::Picker:
		{
			if (m_TileMap->IsValidPosition(gridPos))
			{
				auto* td = m_TileMap->GetTileLayer(gridPos, ETileLayer::Ground);
				if (td)
				{
					m_BrushType = td->Type;
					m_CurrentTool = ETileMapTool::Brush;
					QL_CORE_INFO("Picked tile type: {0}", static_cast<int>(td->Type));
				}
			}
			break;
		}
		case ETileMapTool::Brush:
		default:
		{
			int half = m_BrushSize / 2;
			for (int dy = -half; dy <= half; ++dy)
			{
				for (int dx = -half; dx <= half; ++dx)
				{
					glm::ivec2 pos(gridPos.x + dx, gridPos.y + dy);
					if (m_TileMap->IsValidPosition(pos))
						m_TileMap->SetTileType(pos, m_BrushType);
				}
			}
			break;
		}
		}
	}

	void TileMapEditorPanel::HandleBrushStrokeContinuous(const glm::ivec2& gridPos)
	{
		if (gridPos == m_LastPaintedTile)
			return;
		m_LastPaintedTile = gridPos;
		HandleBrushStroke(gridPos);
	}

	void TileMapEditorPanel::HandleBrushStrokeContinuousReset()
	{
		m_LastPaintedTile = { INT_MAX, INT_MAX };
	}
}
