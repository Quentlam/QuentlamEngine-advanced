#pragma once

#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include <sol/sol.hpp>
#include <string>
#include <functional>
#include <unordered_map>

namespace Quentlam
{
	class Scene;

	// Each instance owns an isolated sol::environment on a shared sol::state.
	// This lets multiple entities run the same script with zero interference,
	// while still sharing the global API bindings (Key, Input_*, Math_*).
	class LuaScriptInstance
	{
	public:
		LuaScriptInstance(Scene* scene, uint32_t entityID, const std::string& scriptPath);
		~LuaScriptInstance();

		// Load + run the script file into this instance's environment.
		// Returns false on error and logs to QL_CORE_ERROR.
		bool Instantiate();

		// Lifecycle hooks called from Scene during runtime.
		void OnLoad();
		void OnUpdate(float dt);
		void OnDestroy();

		// Call an arbitrary function by name.
		// Returns false if the function is missing or threw an error.
		bool CallFunction(const std::string& name);

		// Expose entity data from C++ side (position, color, etc.) to this instance.
		void SetEntityPosition(const glm::vec3& pos);
		void SetEntityColor(const glm::vec4& color);
		void SetEntityVisible(bool visible);

		const std::string& GetScriptPath() const { return m_ScriptPath; }
		uint32_t GetEntityID() const { return m_EntityID; }
		bool IsValid() const { return m_Valid; }

		// Read entity state from Lua environment (called by Scene each frame).
		// Returns a map of entityID -> position (world coordinates).
		std::unordered_map<uint32_t, glm::vec3> GetAllEntityPositions() const;
		std::unordered_map<uint32_t, glm::vec4> GetAllEntityColors() const;
		std::unordered_map<uint32_t, bool> GetAllEntityVisibility() const;
		std::string GetGameState() const;
		int GetGameScore() const;

	private:
		void SetupGlobals();
		void SetupInstanceAPI();
		void CreateEnvironment();

		Scene* m_Scene = nullptr;
		uint32_t m_EntityID = 0;
		std::string m_ScriptPath;
		sol::environment m_Env;
		bool m_Valid = false;

		glm::vec3 m_EntityPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec4 m_EntityColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool m_EntityVisible = true;

		// Shared state: one per process (all instances share it).
		// Initialized lazily via double-checked locking.
		static sol::state s_SharedLua;
		static bool s_SharedInitialized;
	};
}
