#pragma once
#include "qlpch.h"
#include "Quentlam/Core/Base.h"
#include "Quentlam/World/WorldGridModule.h"
#include "Quentlam/Scene/Components.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace Quentlam
{
	enum class EInteractionType : uint8_t
	{
		None = 0,
		Click = 1,
		Proximity = 2,
		Hold = 3,
		Area = 4
	};

	enum class EInteractionPriority : uint8_t
	{
		Low = 0,
		Normal = 1,
		High = 2,
		Critical = 3
	};

	class Interactable;

	struct InteractionResult
	{
		bool Success = false;
		std::string Message;
		bool ConsumeInput = true;
		bool ShowPrompt = true;
		float Cooldown = 0.0f;
		std::unordered_map<std::string, std::string> CustomData;
	};

	struct InteractionPrompt
	{
		std::string Title;
		std::string Description;
		std::string Keybind;
		EInteractionPriority Priority = EInteractionPriority::Normal;
		bool ShowKeybind = true;
	};

	using InteractionCallback = std::function<InteractionResult(class Entity)>;

	class Interactable
	{
	public:
		Interactable() = default;
		virtual ~Interactable() = default;

		virtual bool CanInteract(class Entity actor) const;
		virtual InteractionResult Interact(class Entity actor);

		virtual InteractionPrompt GetPrompt(class Entity actor) const;
		virtual std::vector<InteractionPrompt> GetAllPrompts(class Entity actor) const;

		EInteractionType GetInteractionType() const { return m_InteractionType; }
		void SetInteractionType(EInteractionType type) { m_InteractionType = type; }

		EInteractionPriority GetPriority() const { return m_Priority; }
		void SetPriority(EInteractionPriority priority) { m_Priority = priority; }

		float GetRange() const { return m_Range; }
		void SetRange(float range) { m_Range = range; }

		float GetCooldown() const { return m_Cooldown; }
		void SetCooldown(float cooldown) { m_Cooldown = cooldown; }

		float GetLastInteractionTime() const { return m_LastInteractionTime; }

		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }

		const std::string& GetDisplayName() const { return m_DisplayName; }
		void SetDisplayName(const std::string& name) { m_DisplayName = name; }

		bool IsEnabled() const { return m_Enabled; }
		void SetEnabled(bool enabled) { m_Enabled = enabled; }

		void SetInteractionCallback(InteractionCallback callback);
		InteractionCallback GetInteractionCallback() const { return m_InteractionCallback; }

		bool IsInRange(const glm::vec2& actorPos) const;
		float GetDistanceTo(const glm::vec2& actorPos) const;

	protected:
		std::string m_Id;
		std::string m_DisplayName = "Interactable";
		EInteractionType m_InteractionType = EInteractionType::Click;
		EInteractionPriority m_Priority = EInteractionPriority::Normal;
		float m_Range = 1.5f;
		float m_Cooldown = 0.0f;
		float m_LastInteractionTime = -999.0f;
		bool m_Enabled = true;
		InteractionCallback m_InteractionCallback;
	};

	class InteractionQuery
	{
	public:
		InteractionQuery() = default;
		InteractionQuery(class Entity actor);

		class Entity GetActor() const { return m_Actor; }
		void SetActor(class Entity actor) { m_Actor = actor; }

		const glm::vec2& GetWorldPosition() const { return m_WorldPosition; }
		void SetWorldPosition(const glm::vec2& pos) { m_WorldPosition = pos; }

		const glm::ivec2& GetGridPosition() const { return m_GridPosition; }
		void SetGridPosition(const glm::ivec2& pos) { m_GridPosition = pos; }

		bool IsActorValid() const;
		bool IsInInteractionRange(const Interactable* target) const;

		void AddFilter(const std::string& key, const std::string& value);
		bool HasFilter(const std::string& key) const;
		const std::string& GetFilter(const std::string& key) const;

	private:
		class Entity m_Actor;
		glm::vec2 m_WorldPosition = { 0.0f, 0.0f };
		glm::ivec2 m_GridPosition = { 0, 0 };
		std::unordered_map<std::string, std::string> m_Filters;
	};

	class ActionDispatcher
	{
	public:
		ActionDispatcher() = default;

		void RegisterAction(const std::string& actionName, std::function<void(class Entity, const InteractionQuery&)> callback);
		void UnregisterAction(const std::string& actionName);

		void DispatchAction(const std::string& actionName, class Entity target, const InteractionQuery& query);
		bool HasAction(const std::string& actionName) const;

		void RegisterGlobalAction(const std::string& actionName, std::function<void()> callback);
		void DispatchGlobalAction(const std::string& actionName);

		void Clear();

	private:
		std::unordered_map<std::string, std::function<void(class Entity, const InteractionQuery&)>> m_Actions;
		std::unordered_map<std::string, std::function<void()>> m_GlobalActions;
	};

	class InteractionModule
	{
	public:
		static InteractionModule& Get()
		{
			static InteractionModule instance;
			return instance;
		}

		void OnUpdate(class Scene* scene, float dt);

		bool TryInteract(class Scene* scene, class Entity targetEntity);

		InteractableComponent* GetHoveredInteractable() const
		{
			return m_HoveredInteractable;
		}

		entt::entity GetHoveredEntity() const
		{
			return m_HoveredEntity;
		}

		void SetPromptKeybind(const std::string& key) { m_PromptKeybind = key; }
		const char* GetPromptKeybind() const { return m_PromptKeybind.c_str(); }

		InteractableComponent* m_HoveredInteractable = nullptr;
		entt::entity m_HoveredEntity = entt::null;

	private:
		InteractionModule() = default;
		float m_Time = 0.0f;
		std::string m_PromptKeybind = "E";
	};

	inline bool Interactable::CanInteract(class Entity actor) const
	{
		if (!m_Enabled) return false;
		float currentTime = 0.0f;
		if (currentTime - m_LastInteractionTime < m_Cooldown)
			return false;
		return true;
	}

	inline InteractionResult Interactable::Interact(class Entity actor)
	{
		if (m_InteractionCallback)
			return m_InteractionCallback(actor);

		InteractionResult result;
		result.Success = true;
		result.Message = "Interaction performed.";
		return result;
	}

	inline InteractionPrompt Interactable::GetPrompt(class Entity actor) const
	{
		InteractionPrompt prompt;
		prompt.Title = m_DisplayName;
		prompt.Description = "Interact with " + m_DisplayName;
		return prompt;
	}

	inline std::vector<InteractionPrompt> Interactable::GetAllPrompts(class Entity actor) const
	{
		return { GetPrompt(actor) };
	}

	inline void Interactable::SetInteractionCallback(InteractionCallback callback)
	{
		m_InteractionCallback = std::move(callback);
	}

	inline bool Interactable::IsInRange(const glm::vec2& actorPos) const
	{
		return GetDistanceTo(actorPos) <= m_Range;
	}

	inline float Interactable::GetDistanceTo(const glm::vec2& actorPos) const
	{
		return glm::distance(actorPos, { 0.0f, 0.0f });
	}

	inline InteractionQuery::InteractionQuery(class Entity actor)
		: m_Actor(actor) {}

	inline bool InteractionQuery::IsActorValid() const
	{
		return m_Actor && (bool)m_Actor;
	}

	inline bool InteractionQuery::IsInInteractionRange(const Interactable* target) const
	{
		if (!target || !IsActorValid()) return false;
		return target->IsInRange(m_WorldPosition);
	}

	inline void InteractionQuery::AddFilter(const std::string& key, const std::string& value)
	{
		m_Filters[key] = value;
	}

	inline bool InteractionQuery::HasFilter(const std::string& key) const
	{
		return m_Filters.find(key) != m_Filters.end();
	}

	inline const std::string& InteractionQuery::GetFilter(const std::string& key) const
	{
		static std::string empty;
		auto it = m_Filters.find(key);
		return it != m_Filters.end() ? it->second : empty;
	}

	inline void ActionDispatcher::RegisterAction(const std::string& actionName, std::function<void(class Entity, const InteractionQuery&)> callback)
	{
		m_Actions[actionName] = std::move(callback);
	}

	inline void ActionDispatcher::UnregisterAction(const std::string& actionName)
	{
		m_Actions.erase(actionName);
	}

	inline void ActionDispatcher::DispatchAction(const std::string& actionName, class Entity target, const InteractionQuery& query)
	{
		auto it = m_Actions.find(actionName);
		if (it != m_Actions.end() && it->second)
			it->second(target, query);
	}

	inline bool ActionDispatcher::HasAction(const std::string& actionName) const
	{
		return m_Actions.find(actionName) != m_Actions.end();
	}

	inline void ActionDispatcher::RegisterGlobalAction(const std::string& actionName, std::function<void()> callback)
	{
		m_GlobalActions[actionName] = std::move(callback);
	}

	inline void ActionDispatcher::DispatchGlobalAction(const std::string& actionName)
	{
		auto it = m_GlobalActions.find(actionName);
		if (it != m_GlobalActions.end() && it->second)
			it->second();
	}

	inline void ActionDispatcher::Clear()
	{
		m_Actions.clear();
		m_GlobalActions.clear();
	}

	inline void InteractionModule::OnUpdate(class Scene* scene, float dt)
	{
		m_Time += dt;

		if (!scene)
			return;

		auto& registry = scene->GetRegistry();
		glm::vec3 playerPos3 = glm::vec3(0.0f);
		auto playerView = registry.view<TransformComponent>();
		for (auto entity : playerView)
		{
			auto& transform = registry.get<TransformComponent>(entity);
			playerPos3 = glm::vec3(transform.Transform[3]);
			break;
		}
		glm::vec2 playerPos(playerPos3.x, playerPos3.y);

		m_HoveredInteractable = nullptr;
		m_HoveredEntity = entt::null;
		InteractableComponent* nearest = nullptr;
		float nearestDist = FLT_MAX;

		auto view = registry.view<TransformComponent, InteractableComponent>();
		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& interactable = view.get<InteractableComponent>(entity);

			if (!interactable.IsEnabled)
				continue;

			glm::vec3 wp3 = glm::vec3(transform.Transform[3]);
			float dist = glm::distance(playerPos, glm::vec2(wp3.x, wp3.y));

			if (dist <= interactable.InteractionRadius && dist < nearestDist)
			{
				nearestDist = dist;
				nearest = &interactable;
				m_HoveredEntity = entity;
			}
		}

		m_HoveredInteractable = nearest;
	}

	inline bool InteractionModule::TryInteract(class Scene* scene, class Entity targetEntity)
	{
		if (!scene)
			return false;

		auto& registry = scene->GetRegistry();
		if (!registry.valid(targetEntity))
			return false;

		if (!registry.all_of<InteractableComponent>(targetEntity))
			return false;

		auto& comp = registry.get<InteractableComponent>(targetEntity);
		if (!comp.IsEnabled)
			return false;
		if (m_Time - comp.LastInteractionTime < comp.InteractionCooldown)
			return false;

		comp.LastInteractionTime = m_Time;
		return true;
	}
}
