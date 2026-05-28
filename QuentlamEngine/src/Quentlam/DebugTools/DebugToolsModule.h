#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/World/WorldGridModule.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <optional>
#include <cstdarg>

namespace Quentlam
{
	class TileMap;

	class MapInspector
	{
	public:
		MapInspector() = default;

		void InspectTileMap(TileMap* tileMap);
		void SetHighlightTile(const glm::ivec2& pos, bool highlight);
		void ClearHighlights();

		const std::string& GetInspectedMapName() const { return m_InspectedMapName; }
		const std::vector<glm::ivec2>& GetHighlightedTiles() const { return m_HighlightedTiles; }

		void AddMarker(const glm::ivec2& pos, const std::string& label);
		void RemoveMarker(const glm::ivec2& pos);
		const std::vector<std::pair<glm::ivec2, std::string>>& GetMarkers() const { return m_Markers; }

		bool IsTileWalkable(const glm::ivec2& pos) const;
		ETileType GetTileType(const glm::ivec2& pos) const;
		std::string GetTileInfo(const glm::ivec2& pos) const;

	private:
		std::string m_InspectedMapName;
		std::vector<glm::ivec2> m_HighlightedTiles;
		std::vector<std::pair<glm::ivec2, std::string>> m_Markers;
		TileMap* m_TileMap = nullptr;
	};

	class ColliderOverlay
	{
	public:
		ColliderOverlay() = default;

		void Enable() { m_Enabled = true; }
		void Disable() { m_Enabled = false; }
		bool IsEnabled() const { return m_Enabled; }
		void Toggle() { m_Enabled = !m_Enabled; }

		void SetOverlayColor(const glm::vec4& color) { m_OverlayColor = color; }
		const glm::vec4& GetOverlayColor() const { return m_OverlayColor; }

		void SetLineWidth(float width) { m_LineWidth = width; }
		float GetLineWidth() const { return m_LineWidth; }

		void ShowTileColliders(bool show) { m_ShowTileColliders = show; }
		void ShowEntityColliders(bool show) { m_ShowEntityColliders = show; }
		void ShowTriggerRegions(bool show) { m_ShowTriggerRegions = show; }

		bool ShowsTileColliders() const { return m_ShowTileColliders; }
		bool ShowsEntityColliders() const { return m_ShowEntityColliders; }
		bool ShowsTriggerRegions() const { return m_ShowTriggerRegions; }

		void AddIgnoredEntity(entt::entity entity) { m_IgnoredEntities.insert(entity); }
		void RemoveIgnoredEntity(entt::entity entity) { m_IgnoredEntities.erase(entity); }
		void ClearIgnoredEntities() { m_IgnoredEntities.clear(); }
		bool IsEntityIgnored(entt::entity entity) const { return m_IgnoredEntities.find(entity) != m_IgnoredEntities.end(); }

	private:
		bool m_Enabled = false;
		glm::vec4 m_OverlayColor = { 0.0f, 1.0f, 0.0f, 1.0f };
		float m_LineWidth = 1.0f;
		bool m_ShowTileColliders = true;
		bool m_ShowEntityColliders = true;
		bool m_ShowTriggerRegions = true;
		std::unordered_set<entt::entity> m_IgnoredEntities;
	};

	class EntityPicker
	{
	public:
		EntityPicker() = default;

		void SetMaxResults(int32_t max) { m_MaxResults = max; }
		int32_t GetMaxResults() const { return m_MaxResults; }

		void SetPickRadius(float radius) { m_PickRadius = radius; }
		float GetPickRadius() const { return m_PickRadius; }

		void SetIncludeInvisible(bool include) { m_IncludeInvisible = include; }
		bool IncludesInvisible() const { return m_IncludeInvisible; }

		void SetIncludeLocked(bool include) { m_IncludeLocked = include; }
		bool IncludesLocked() const { return m_IncludeLocked; }

		void AddFilterTag(const std::string& tag) { m_FilterTags.push_back(tag); }
		void ClearFilterTags() { m_FilterTags.clear(); }
		const std::vector<std::string>& GetFilterTags() const { return m_FilterTags; }

		entt::entity Pick(const glm::vec2& screenPos, class Scene* scene);
		std::vector<entt::entity> PickAll(const glm::vec2& screenPos, class Scene* scene);

		entt::entity PickByGridCoord(const glm::ivec2& gridPos, class Scene* scene);
		std::vector<entt::entity> PickByTag(const std::string& tag, class Scene* scene);

		void SetOnEntityPicked(std::function<void(entt::entity)> callback) { m_OnEntityPicked = std::move(callback); }

	private:
		int32_t m_MaxResults = 10;
		float m_PickRadius = 1.0f;
		bool m_IncludeInvisible = false;
		bool m_IncludeLocked = false;
		std::vector<std::string> m_FilterTags;
		std::function<void(entt::entity)> m_OnEntityPicked;
	};

	class DebugConsole
	{
	public:
		DebugConsole() = default;
		static DebugConsole& Get();

		void RegisterCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> callback, const std::string& help = "");
		void UnregisterCommand(const std::string& name);

		bool Execute(const std::string& commandLine);
		bool Executef(const char* format, ...);

		const std::vector<std::string>& GetCommandNames() const { return m_CommandNames; }
		bool HasCommand(const std::string& name) const;

		void AddOutput(const std::string& message);
		void ClearOutput();
		const std::vector<std::string>& GetOutputHistory() const { return m_OutputHistory; }
		const std::vector<std::string>& GetCommandHistory() const { return m_CommandHistory; }

		void SetMaxHistorySize(int32_t max) { m_MaxHistorySize = max; }
		int32_t GetMaxHistorySize() const { return m_MaxHistorySize; }

		struct CommandInfo
		{
			std::string Name;
			std::string Help;
			std::function<void(const std::vector<std::string>&)> Callback;
		};
		const CommandInfo* GetCommandInfo(const std::string& name) const;

	private:
		std::unordered_map<std::string, CommandInfo> m_Commands;
		std::vector<std::string> m_CommandNames;
		std::vector<std::string> m_OutputHistory;
		std::vector<std::string> m_CommandHistory;
		int32_t m_MaxHistorySize = 100;
	};

	class QUENTLAM_API DebugToolsModule
	{
	public:
		DebugToolsModule() = default;
		static DebugToolsModule& Get();

		void Initialize();
		void Shutdown();
		void Update(float deltaTime);

		MapInspector& GetMapInspector() { return m_MapInspector; }
		const MapInspector& GetMapInspector() const { return m_MapInspector; }

		ColliderOverlay& GetColliderOverlay() { return m_ColliderOverlay; }
		const ColliderOverlay& GetColliderOverlay() const { return m_ColliderOverlay; }

		EntityPicker& GetEntityPicker() { return m_EntityPicker; }
		const EntityPicker& GetEntityPicker() const { return m_EntityPicker; }

		DebugConsole& GetConsole() { return DebugConsole::Get(); }
		const DebugConsole& GetConsole() const { return DebugConsole::Get(); }

		void ToggleMapInspector() { m_MapInspectorEnabled = !m_MapInspectorEnabled; }
		void ToggleColliderOverlay() { m_ColliderOverlay.Toggle(); }
		void ToggleEntityPicker() { m_EntityPickerEnabled = !m_EntityPickerEnabled; }
		void ToggleConsole() { m_ConsoleEnabled = !m_ConsoleEnabled; }

		bool IsMapInspectorEnabled() const { return m_MapInspectorEnabled; }
		bool IsColliderOverlayEnabled() const { return m_ColliderOverlay.IsEnabled(); }
		bool IsEntityPickerEnabled() const { return m_EntityPickerEnabled; }
		bool IsConsoleEnabled() const { return m_ConsoleEnabled; }

		void SetAllEnabled(bool enabled);

		void AddCheckpoint(const std::string& name, const std::string& description = "");
		void RemoveCheckpoint(const std::string& name);
		void ListCheckpoints() const;
		bool GotoCheckpoint(const std::string& name);

		std::string CaptureDebugSnapshot() const;
		void PrintDebugSummary() const;

		std::function<void(const std::string& message)> OnDebugMessage;
		std::function<void(const std::string& error)> OnDebugError;

	private:
		MapInspector m_MapInspector;
		ColliderOverlay m_ColliderOverlay;
		EntityPicker m_EntityPicker;
		bool m_MapInspectorEnabled = false;
		bool m_EntityPickerEnabled = false;
		bool m_ConsoleEnabled = false;
		std::vector<std::pair<std::string, std::string>> m_Checkpoints;
	};

	inline DebugConsole& DebugConsole::Get()
	{
		static DebugConsole instance;
		return instance;
	}

	inline void DebugConsole::RegisterCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> callback, const std::string& help)
	{
		if (m_Commands.find(name) == m_Commands.end())
			m_CommandNames.push_back(name);
		m_Commands[name] = { name, help, std::move(callback) };
	}

	inline void DebugConsole::UnregisterCommand(const std::string& name)
	{
		m_Commands.erase(name);
		m_CommandNames.erase(std::remove(m_CommandNames.begin(), m_CommandNames.end(), name), m_CommandNames.end());
	}

	inline bool DebugConsole::HasCommand(const std::string& name) const
	{
		return m_Commands.find(name) != m_Commands.end();
	}

	inline bool DebugConsole::Execute(const std::string& commandLine)
	{
		if (commandLine.empty()) return false;

		m_CommandHistory.push_back(commandLine);
		if ((int32_t)m_CommandHistory.size() > m_MaxHistorySize)
			m_CommandHistory.erase(m_CommandHistory.begin());

		size_t spacePos = commandLine.find(' ');
		std::string cmd = commandLine.substr(0, spacePos);
		std::vector<std::string> args;

		if (spacePos != std::string::npos)
		{
			std::string argsStr = commandLine.substr(spacePos + 1);
			size_t start = 0;
			while (start < argsStr.size())
			{
				if (argsStr[start] == ' ') { start++; continue; }
				if (argsStr[start] == '"')
				{
					auto end = argsStr.find('"', start + 1);
					if (end != std::string::npos)
					{
						args.push_back(argsStr.substr(start + 1, end - start - 1));
						start = end + 1;
					}
					else
					{
						args.push_back(argsStr.substr(start));
						break;
					}
				}
				else
				{
					auto end = argsStr.find(' ', start);
					if (end == std::string::npos)
					{
						args.push_back(argsStr.substr(start));
						break;
					}
					else
					{
						args.push_back(argsStr.substr(start, end - start));
						start = end + 1;
					}
				}
			}
		}

		auto it = m_Commands.find(cmd);
		if (it != m_Commands.end())
		{
			try { it->second.Callback(args); return true; }
			catch (const std::exception& e)
			{
				AddOutput("[ERROR] Exception in command '" + cmd + "': " + e.what());
				return false;
			}
		}
		else
		{
			AddOutput("[ERROR] Unknown command: " + cmd);
			return false;
		}
	}

	inline bool DebugConsole::Executef(const char* format, ...)
	{
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		return Execute(buffer);
	}

	inline const DebugConsole::CommandInfo* DebugConsole::GetCommandInfo(const std::string& name) const
	{
		auto it = m_Commands.find(name);
		return it != m_Commands.end() ? &it->second : nullptr;
	}

	inline void DebugConsole::AddOutput(const std::string& message)
	{
		m_OutputHistory.push_back(message);
		if ((int32_t)m_OutputHistory.size() > m_MaxHistorySize)
			m_OutputHistory.erase(m_OutputHistory.begin());
	}

	inline void DebugConsole::ClearOutput()
	{
		m_OutputHistory.clear();
	}

	inline DebugToolsModule& DebugToolsModule::Get()
	{
		static DebugToolsModule instance;
		return instance;
	}

	inline void DebugToolsModule::Initialize()
	{
		auto& console = DebugConsole::Get();
		auto registerHelp = [](const std::vector<std::string>& a) {
			auto& c = DebugConsole::Get();
			if (a.empty()) {
				c.AddOutput("Available commands:");
				for (const auto& n : c.GetCommandNames()) c.AddOutput("  " + n);
			}
			else {
				auto* info = c.GetCommandInfo(a[0]);
				if (info) c.AddOutput(a[0] + ": " + info->Help);
				else c.AddOutput("Unknown command: " + a[0]);
			}
		};
		console.RegisterCommand("help", registerHelp, "help [command]");
		console.RegisterCommand("clear", [](const std::vector<std::string>&) { DebugConsole::Get().ClearOutput(); }, "clear");
		console.RegisterCommand("checkpoint", [](const std::vector<std::string>& a) {
			auto& dt = DebugToolsModule::Get();
			if (a.empty() || a[0] == "list") { dt.ListCheckpoints(); }
			else if (a[0] == "goto" && a.size() > 1) { dt.GotoCheckpoint(a[1]); }
			else { std::string d = a.size() > 1 ? a[1] : ""; dt.AddCheckpoint(a[0], d); }
		}, "checkpoint [name] [desc]");
		console.RegisterCommand("toggle_overlay", [](const std::vector<std::string>&) { DebugToolsModule::Get().ToggleColliderOverlay(); }, "toggle_overlay");
		console.RegisterCommand("toggle_inspector", [](const std::vector<std::string>&) { DebugToolsModule::Get().ToggleMapInspector(); }, "toggle_inspector");
		console.RegisterCommand("snapshot", [](const std::vector<std::string>&) {
			auto& dt = DebugToolsModule::Get();
			DebugConsole::Get().AddOutput("Snapshot: " + dt.CaptureDebugSnapshot());
		}, "snapshot");
		console.RegisterCommand("summary", [](const std::vector<std::string>&) { DebugToolsModule::Get().PrintDebugSummary(); }, "summary");
	}

	inline void DebugToolsModule::Shutdown() { m_Checkpoints.clear(); }
	inline void DebugToolsModule::Update(float) {}

	inline void DebugToolsModule::SetAllEnabled(bool enabled)
	{
		m_MapInspectorEnabled = enabled;
		m_EntityPickerEnabled = enabled;
		m_ConsoleEnabled = enabled;
		if (enabled) m_ColliderOverlay.Enable();
		else m_ColliderOverlay.Disable();
	}

	inline void DebugToolsModule::AddCheckpoint(const std::string& name, const std::string& description)
	{
		m_Checkpoints.push_back({ name, description });
	}

	inline void DebugToolsModule::RemoveCheckpoint(const std::string& name)
	{
		m_Checkpoints.erase(std::remove_if(m_Checkpoints.begin(), m_Checkpoints.end(),
			[&name](const auto& cp) { return cp.first == name; }), m_Checkpoints.end());
	}

	inline void DebugToolsModule::ListCheckpoints() const
	{
		DebugConsole::Get().AddOutput("Checkpoints:");
		for (const auto& cp : m_Checkpoints)
			DebugConsole::Get().AddOutput("  [" + cp.first + "] " + cp.second);
	}

	inline bool DebugToolsModule::GotoCheckpoint(const std::string& name)
	{
		for (const auto& cp : m_Checkpoints)
			if (cp.first == name) { return true; }
		return false;
	}

	inline std::string DebugToolsModule::CaptureDebugSnapshot() const
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "Inspector=%d Overlay=%d Picker=%d Console=%d CP=%zu",
			m_MapInspectorEnabled, m_ColliderOverlay.IsEnabled(),
			m_EntityPickerEnabled, m_ConsoleEnabled, m_Checkpoints.size());
		return buf;
	}

	inline void DebugToolsModule::PrintDebugSummary() const
	{
		auto& c = DebugConsole::Get();
		c.AddOutput("=== Debug Summary ===");
		c.AddOutput("Inspector: " + std::string(m_MapInspectorEnabled ? "ON" : "OFF"));
		c.AddOutput("Overlay: " + std::string(m_ColliderOverlay.IsEnabled() ? "ON" : "OFF"));
		c.AddOutput("Picker: " + std::string(m_EntityPickerEnabled ? "ON" : "OFF"));
		c.AddOutput("Console: " + std::string(m_ConsoleEnabled ? "ON" : "OFF"));
		c.AddOutput("Checkpoints: " + std::to_string(m_Checkpoints.size()));
	}
}
