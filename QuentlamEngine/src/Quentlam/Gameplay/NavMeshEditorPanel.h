#pragma once

#include "Quentlam/Core/Base.h"
#include <string>

namespace Quentlam
{

class NavigationMesh;

class NavMeshEditorPanel
{
public:
	static NavMeshEditorPanel& Get();

	void OnImGuiRender();

	bool IsOpen() const { return m_IsOpen; }
	void Open() { m_IsOpen = true; }
	void Close() { m_IsOpen = false; }
	void Toggle() { m_IsOpen = !m_IsOpen; }

	bool HasUnsavedChanges() const { return m_UnsavedChanges; }
	void MarkSaved() { m_UnsavedChanges = false; }
	void MarkDirty() { m_UnsavedChanges = true; }

	Ref<NavigationMesh> GetNavMesh() const;
	float GetAgentRadius() const { return m_AgentRadius; }
	float GetAgentHeight() const { return m_AgentHeight; }
	float GetAgentStepHeight() const { return m_AgentStepHeight; }
	float GetAgentMaxSlope() const { return m_AgentMaxSlope; }
	bool IsOverlayEnabled() const { return m_ShowOverlay; }
	float GetTileSize() const { return m_TileSize; }

	void Bake();

private:
	NavMeshEditorPanel() = default;
	~NavMeshEditorPanel() = default;

	bool m_IsOpen = false;
	bool m_UnsavedChanges = false;
	bool m_ShowOverlay = true;

	float m_AgentRadius = 0.25f;
	float m_AgentHeight = 1.0f;
	float m_AgentStepHeight = 0.1f;
	float m_AgentMaxSlope = 45.0f;
	float m_TileSize = 1.0f;

	std::string m_CurrentFilePath;
};

}
