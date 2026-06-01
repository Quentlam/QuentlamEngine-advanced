#include "qlpch.h"
#include "NavMeshEditorPanel.h"
#include "NavigationModule.h"
#include <imgui.h>
#include <Commdlg.h>

namespace Quentlam
{

NavMeshEditorPanel& NavMeshEditorPanel::Get()
{
	static NavMeshEditorPanel instance;
	return instance;
}

Ref<NavigationMesh> NavMeshEditorPanel::GetNavMesh() const
{
	return NavigationModule::Get().GetNavMesh();
}

void NavMeshEditorPanel::OnImGuiRender()
{
	if (!m_IsOpen)
		return;

	ImGui::Begin("Navigation", &m_IsOpen);

	if (ImGui::CollapsingHeader("Agent Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("AgentSettings");
		ImGui::DragFloat("Agent Radius", &m_AgentRadius, 0.05f, 0.01f, 10.0f, "%.2f");
		ImGui::DragFloat("Agent Height", &m_AgentHeight, 0.05f, 0.1f, 50.0f, "%.2f");
		ImGui::DragFloat("Step Height", &m_AgentStepHeight, 0.01f, 0.0f, 5.0f, "%.2f");
		ImGui::DragFloat("Max Slope", &m_AgentMaxSlope, 1.0f, 0.0f, 90.0f, "%.1f deg");
		ImGui::Separator();
		ImGui::DragFloat("Tile Size", &m_TileSize, 0.1f, 0.1f, 10.0f, "%.1f");
		ImGui::PopID();

		if (ImGui::IsItemDeactivatedAfterEdit())
			MarkDirty();
	}

	if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Show NavMesh Overlay", &m_ShowOverlay);
	}

	if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Bake", ImVec2(120, 0)))
		{
			Bake();
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear", ImVec2(120, 0)))
		{
			auto nav = NavigationModule::Get().GetNavMesh();
			if (nav)
			{
				nav->Clear();
				MarkDirty();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Save NavMesh...", ImVec2(120, 0)))
		{
			char filepath[512] = { 0 };
			OPENFILENAMEA ofn = {};
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = "NavMesh Files (*.navmesh)\0*.navmesh\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = filepath;
			ofn.nMaxFile = 511;
			ofn.lpstrTitle = "Save NavMesh";
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
			if (GetSaveFileNameA(&ofn))
			{
				auto nav = NavigationModule::Get().GetNavMesh();
				if (nav && nav->SerializeToFile(std::string(filepath)))
				{
					m_CurrentFilePath = filepath;
					MarkSaved();
					QL_CORE_INFO("NavMesh saved to {0}", filepath);
				}
				else
				{
					QL_CORE_ERROR("Failed to save NavMesh to {0}", filepath);
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Load NavMesh...", ImVec2(120, 0)))
		{
			char filepath[512] = { 0 };
			OPENFILENAMEA ofn = {};
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = "NavMesh Files (*.navmesh)\0*.navmesh\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = filepath;
			ofn.nMaxFile = 511;
			ofn.lpstrTitle = "Load NavMesh";
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileNameA(&ofn))
			{
				if (NavigationModule::Get().LoadNavMesh(std::string(filepath)))
				{
					m_CurrentFilePath = filepath;
					auto nav = NavigationModule::Get().GetNavMesh();
					if (nav)
					{
						m_AgentRadius = nav->AgentSettings.Radius;
						m_AgentHeight = nav->AgentSettings.Height;
						m_AgentStepHeight = nav->AgentSettings.StepHeight;
						m_AgentMaxSlope = nav->AgentSettings.MaxSlope;
						m_TileSize = nav->TileSize;
					}
					MarkSaved();
					QL_CORE_INFO("NavMesh loaded from {0}", filepath);
				}
				else
				{
					QL_CORE_ERROR("Failed to load NavMesh from {0}", filepath);
				}
			}
		}

		if (!m_CurrentFilePath.empty())
		{
			ImGui::Spacing();
			ImGui::Text("Current: %s", m_CurrentFilePath.c_str());
		}

		if (m_UnsavedChanges)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Unsaved changes!");
		}
	}

	auto nav = GetNavMesh();
	if (nav)
	{
		if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto size = nav->GetSize();
			ImGui::Text("Mesh Size: %d x %d", size.x, size.y);
			ImGui::Text("Tiles: %zu", nav->GetSize().x * nav->GetSize().y);
		}
	}

	ImGui::End();
}

void NavMeshEditorPanel::Bake()
{
	auto nav = NavigationModule::Get().GetNavMesh();
	if (!nav)
	{
		QL_CORE_WARN("NavMeshEditor: No NavMesh available for baking. Set a TileMap first.");
		return;
	}

	nav->AgentSettings.Radius = m_AgentRadius;
	nav->AgentSettings.Height = m_AgentHeight;
	nav->AgentSettings.StepHeight = m_AgentStepHeight;
	nav->AgentSettings.MaxSlope = m_AgentMaxSlope;
	nav->TileSize = m_TileSize;

	auto tileMap = NavigationModule::Get().GetTileMap();
	if (tileMap)
	{
		auto baked = NavigationMesh::FromTileMap(tileMap, m_TileSize);
		auto size = baked.GetSize();
		nav->SetSize(size.x, size.y);
		nav->SetOrigin(baked.GetOrigin());

		for (int32_t y = 0; y < size.y; ++y)
		{
			for (int32_t x = 0; x < size.x; ++x)
			{
				glm::ivec2 pos(x, y);
				nav->SetWalkable(pos, baked.IsWalkable(pos));
				nav->SetCost(pos, baked.GetCost(pos));
			}
		}
	}

	MarkDirty();
	QL_CORE_INFO("NavMeshEditor: Bake complete. Size: %d x %d", nav->GetSize().x, nav->GetSize().y);
}

}
