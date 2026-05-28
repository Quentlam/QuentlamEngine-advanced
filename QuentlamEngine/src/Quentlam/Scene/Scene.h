#pragma once

#include <entt/entt.hpp>
#include <string>
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
		void DestroyEntity(entt::entity entity);

		Entity GetEntity(entt::entity entity);

		bool OnRuntimeStart();
		void OnRuntimeStop();
		bool ValidateRuntimeState(std::string* failureReason = nullptr);

		void OnUpdate(Timestep ts);
		void OnUpdateRuntime(Timestep ts);

		void OnViewportResize(uint32_t width, uint32_t height);

		entt::registry& GetRegistry() { return m_Registry; }
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		friend class Entity;
	};
}
