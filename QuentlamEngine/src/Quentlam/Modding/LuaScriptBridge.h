#pragma once
#include "ModdingModule.h"
#include <sol/sol.hpp>
#include <fstream>
#include <sstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Gameplay/AnimationLoader.h"
#include "Quentlam/Audio/AudioModule.h"
#include "Quentlam/Scene/PrefabSystem.h"
#include "Quentlam/Scene/SceneManager.h"

namespace Quentlam
{
	class Scene;

	class LuaScriptBridge : public ScriptBridge
	{
	public:
		LuaScriptBridge();
		~LuaScriptBridge() override;

		bool LoadScript(const std::string& scriptPath) override;
		bool CallFunction(const std::string& funcName, const std::vector<std::string>& args, std::string* result = nullptr) override;
		bool HasFunction(const std::string& funcName) const override;
		void UnloadScript() override;
		bool IsValid() const override;

		bool CallLoad();
		bool CallUpdate(float dt);
		bool CallOnSave(const std::string& context);
		bool CallOnLoad(const std::string& context);

		void SetScene(Scene* scene) { m_Scene = scene; }
		Scene* GetScene() const { return m_Scene; }

		sol::state& GetLuaState() { return m_Lua; }
		const sol::state& GetLuaState() const { return m_Lua; }

	private:
		void SetupBindings();
		void SetupInputAPI();
		void SetupEntityAPI();
		void SetupGameAPI();
		void SetupMathAPI();
		void SetupAnimationAPI();
		void SetupInteractionAPI();
		void SetupParticleAPI();
		void SetupNavigationAPI();
		void SetupSceneAPI();
		void SetupAtlasAPI();
		void SetupCameraAPI();
		void SetupAudioAPI();
		void SetupPrefabAPI();

		Scene* m_Scene = nullptr;

		sol::state m_Lua;
		std::string m_CurrentScriptPath;
		std::unordered_set<std::string> m_RegisteredFunctions;

		std::unordered_map<std::string, glm::vec3> m_EntityPositions;
		std::unordered_map<std::string, glm::vec4> m_EntityColors;
		std::unordered_map<std::string, bool> m_EntityVisibility;
		std::unordered_map<std::string, std::string> m_EntityTags;

		std::string m_GameState;
		int m_Score = 0;
		float m_GameTime = 0.0f;
		float m_DeltaTime = 0.0f;
		std::string m_LastMessage;
		bool m_GameOver = false;
	};
}
