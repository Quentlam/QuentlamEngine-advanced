#pragma once

#include <entt/entt.hpp>
#include <string>
#include <glm/glm.hpp>
#include "Quentlam/Core/Timestep.h"

namespace Quentlam
{
	class Entity;

	class Scene
	{
	public:
		Scene(const std::string& name = std::string());
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithData(const std::string& name, const std::string& entityJson);
		std::string SerializeEntityToString(entt::entity entity);
		void DestroyEntity(entt::entity entity);
		void CreateDefaultScene(Entity* outSceneCamera, Entity* outGameCamera);
		void FindDefaultCameraEntities(Entity* outSceneCamera, Entity* outGameCamera);
		void SyncSceneCameraFromController(const glm::vec3& position, bool is3D, float aspect);
		Entity GetOrCreateSceneCamera();
		Entity GetOrCreateGameCamera();
		Entity GetActiveGameCamera();
		void SetActiveGameCamera(Entity entity);

		Entity GetEntity(entt::entity entity);

		bool OnRuntimeStart();
		void OnRuntimeStop();
		bool ValidateRuntimeState(std::string* failureReason = nullptr);

		void OnUpdate(Timestep ts);
		void OnUpdateRuntime(Timestep ts);
		void OnStepFrame();
		void OnRender2DOnly(Timestep ts);

		// Lua script lifecycle
		void InstantiateLuaScripts();
		void UpdateLuaScripts(Timestep ts);
		void DestroyLuaScripts();
		void RenderLuaEntities();

		// UI component lifecycle
		void InstantiateUIComponents();
		void UpdateUIComponents(Timestep ts);
		void RenderUIComponents();
		void DestroyUIComponents();

		void OnViewportResize(uint32_t width, uint32_t height);

		entt::registry& GetRegistry() { return m_Registry; }
		const std::string& GetName() const { return m_Name; }

	public:
		std::string m_Name;
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		entt::entity m_ActiveGameCamera = entt::null;

		friend class Entity;
	};
}
