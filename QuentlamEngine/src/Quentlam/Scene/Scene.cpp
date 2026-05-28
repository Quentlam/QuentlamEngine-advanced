#include "qlpch.h"

#include "Scene.h"
#include "Quentlam/Renderer/Renderer2D.h"
#include "Quentlam/Renderer/Material.h"
#include "SpriteRendererComponent.h"
#include "SpriteAnimationComponent.h"
#include "Entity.h"
#include "Components.h"
#include "Quentlam/Physics/Physics2D.h"
#include "Quentlam/Physics/Physics3D.h"
#include "Quentlam/Physics/Physics3DValidation.h"
#include "Quentlam/Gameplay/NpcModule.h"
#include "Quentlam/Gameplay/QuestEventModule.h"

#include <glm/glm.hpp>
#include <sstream>


namespace Quentlam
{
	namespace
	{
		bool IsFiniteFloat(float value)
		{
			return std::isfinite(value);
		}

		bool IsFiniteVec2(const glm::vec2& value)
		{
			return IsFiniteFloat(value.x) && IsFiniteFloat(value.y);
		}

		bool IsFiniteVec3(const glm::vec3& value)
		{
			return IsFiniteFloat(value.x) && IsFiniteFloat(value.y) && IsFiniteFloat(value.z);
		}

		bool IsFiniteMat4(const glm::mat4& transform)
		{
			for (int column = 0; column < 4; ++column)
			{
				for (int row = 0; row < 4; ++row)
				{
					if (!IsFiniteFloat(transform[column][row]))
						return false;
				}
			}

			return true;
		}

		glm::vec3 ExtractWorldScale(const glm::mat4& transform)
		{
			return {
				glm::length(glm::vec3(transform[0])),
				glm::length(glm::vec3(transform[1])),
				glm::length(glm::vec3(transform[2]))
			};
		}

		std::string GetEntityLabel(entt::registry& registry, entt::entity entity)
		{
			if (registry.all_of<TagComponent>(entity))
				return registry.get<TagComponent>(entity).Tag;

			return "Entity";
		}

		bool AppendFailure(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
				*failureReason = message;
			return false;
		}
	}

	static void DoMath(const glm::mat4& transform)
	{


	};

	static void OnTransformConstruct(entt::registry& registry, entt::entity entity)
	{


	};



	Scene::Scene(const std::string& name)
	{



	};


	Scene::~Scene()
	{



	};

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create() , this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		return entity;
	};

	void Scene::DestroyEntity(entt::entity entity)
	{
		m_Registry.destroy(entity);
	}

	Entity Scene::GetEntity(entt::entity entity)
	{
		return Entity{ entity, this };
	}

	bool Scene::OnRuntimeStart()
	{
		if (!Physics2D::OnRuntimeStart(this))
			return false;

		if (!Physics3D::OnRuntimeStart(this))
		{
			Physics2D::OnRuntimeStop(this);
			return false;
		}

		NpcModule::Get().OnDayStart();
		QuestEventModule::Get().AdvanceDay();

		return true;
	}

	bool Scene::ValidateRuntimeState(std::string* failureReason)
	{
		auto invalidTransformView = m_Registry.view<TransformComponent>();
		for (auto entityHandle : invalidTransformView)
		{
			const auto& transform = invalidTransformView.get<TransformComponent>(entityHandle).Transform;
			if (!IsFiniteMat4(transform))
			{
				return AppendFailure(failureReason,
					"Entity '" + GetEntityLabel(m_Registry, entityHandle) + "' has a transform containing NaN/Inf values.");
			}
		}

		auto rigidbody2DView = m_Registry.view<Rigidbody2DComponent>();
		for (auto entityHandle : rigidbody2DView)
		{
			if (!m_Registry.all_of<TransformComponent>(entityHandle))
			{
				return AppendFailure(failureReason,
					"2D rigid body '" + GetEntityLabel(m_Registry, entityHandle) + "' is missing TransformComponent.");
			}

			const auto& transform = m_Registry.get<TransformComponent>(entityHandle).Transform;
			const glm::vec2 worldScale = { transform[0][0], transform[1][1] };
			if (!IsFiniteVec2(worldScale))
			{
				return AppendFailure(failureReason,
					"2D rigid body '" + GetEntityLabel(m_Registry, entityHandle) + "' has a non-finite scale.");
			}

			if (m_Registry.all_of<BoxCollider2DComponent>(entityHandle))
			{
				const auto& collider = m_Registry.get<BoxCollider2DComponent>(entityHandle);
				const glm::vec2 scaledSize = glm::abs(collider.Size * worldScale);
				if (!IsFiniteVec2(collider.Offset) || !IsFiniteVec2(collider.Size) || !IsFiniteVec2(scaledSize))
				{
					return AppendFailure(failureReason,
						"2D box collider '" + GetEntityLabel(m_Registry, entityHandle) + "' contains non-finite dimensions.");
				}

				if (scaledSize.x <= 0.0f || scaledSize.y <= 0.0f)
				{
					return AppendFailure(failureReason,
						"2D box collider '" + GetEntityLabel(m_Registry, entityHandle) + "' resolves to a zero-sized collider.");
				}
			}
		}

		auto rigidbody3DView = m_Registry.view<Rigidbody3DComponent>();
		for (auto entityHandle : rigidbody3DView)
		{
			if (!m_Registry.all_of<TransformComponent>(entityHandle))
			{
				return AppendFailure(failureReason,
					"3D rigid body '" + GetEntityLabel(m_Registry, entityHandle) + "' is missing TransformComponent.");
			}

			const auto& transformComponent = m_Registry.get<TransformComponent>(entityHandle);
			const glm::vec3 translation = glm::vec3(transformComponent.Transform[3]);
			const glm::vec3 scale = ExtractWorldScale(transformComponent.Transform);

			if (!Physics3DValidation::IsFiniteVec3(translation) || !Physics3DValidation::IsFiniteVec3(scale))
			{
				return AppendFailure(failureReason,
					"3D rigid body '" + GetEntityLabel(m_Registry, entityHandle) + "' has a non-finite translation or scale.");
			}

			const auto& body = rigidbody3DView.get<Rigidbody3DComponent>(entityHandle);
			if (!IsFiniteFloat(body.Mass) || (body.Type == Rigidbody3DComponent::BodyType::Dynamic && body.Mass <= 0.0f))
			{
				return AppendFailure(failureReason,
					"3D rigid body '" + GetEntityLabel(m_Registry, entityHandle) + "' has an invalid mass.");
			}

			if (m_Registry.all_of<BoxCollider3DComponent>(entityHandle))
			{
				const auto& collider = m_Registry.get<BoxCollider3DComponent>(entityHandle);
				if (!Physics3DValidation::IsFiniteVec3(collider.Offset) || !Physics3DValidation::IsFiniteVec3(collider.HalfExtent))
				{
					return AppendFailure(failureReason,
						"3D box collider '" + GetEntityLabel(m_Registry, entityHandle) + "' contains non-finite values.");
				}

				const glm::vec3 sanitizedHalfExtent = Physics3DValidation::SanitizeHalfExtent(collider.HalfExtent, scale);
				if (!Physics3DValidation::IsFiniteVec3(sanitizedHalfExtent))
				{
					return AppendFailure(failureReason,
						"3D box collider '" + GetEntityLabel(m_Registry, entityHandle) + "' could not be sanitized to a valid shape.");
				}
			}
		}

		return true;
	}

	void Scene::OnRuntimeStop()
	{
		Physics2D::OnRuntimeStop(this);
		Physics3D::OnRuntimeStop(this);
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		if (ts > 0.1f) ts = 0.1f;
		if (ts > 0.0f)
		{
			Physics2D::OnUpdate(this, ts);
			Physics3D::OnUpdate(this, ts);
		}

		NpcModule::Get().Update(ts.GetSeconds());
		QuestEventModule::Get().Update(ts.GetSeconds());

		{
			auto animView = m_Registry.view<SpriteAnimationComponent>();
			for (auto entity : animView)
			{
				auto& anim = animView.get<SpriteAnimationComponent>(entity);
				if (anim.Animator)
					anim.Animator->Update(ts.GetSeconds());
			}
		}

		OnUpdate(ts);
	}

	void Scene::OnUpdate(Timestep ts)
	{
		auto spriteView = m_Registry.view<TransformComponent, SpriteTransformComponent>();

		for (auto entity : spriteView)
		{
			auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
			if (sprite.Texture)
			{
				Renderer2D::DrawQuad(transform.Transform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(transform.Transform, sprite.Color, (int)(uint32_t)entity);
			}
		}

		auto srView = m_Registry.view<TransformComponent, SpriteRendererComponent>();
		for (auto entity : srView)
		{
			auto [transform, src] = srView.get<TransformComponent, SpriteRendererComponent>(entity);

			Ref<Texture2D> texToDraw = src.SubTexture ? src.SubTexture->GetTexture() : src.Texture;

			auto* animComp = m_Registry.try_get<SpriteAnimationComponent>(entity);
			if (animComp && animComp->AtlasBinding)
			{
				auto subTex = animComp->GetCurrentSubTexture();
				if (subTex)
					texToDraw = subTex->GetTexture();
			}

			if (texToDraw)
			{
				Renderer2D::DrawQuad(transform.Transform, texToDraw,
					src.TilingFactor, src.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(transform.Transform, src.Color, (int)(uint32_t)entity);
			}
		}

		auto triView = m_Registry.view<TransformComponent, TriangleRendererComponent>();
		for (auto entity : triView)
		{
			auto [transform, sprite] = triView.get<TransformComponent, TriangleRendererComponent>(entity);
			if (sprite.Texture)
			{
				Renderer2D::DrawTriangle(transform.Transform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawTriangle(transform.Transform, sprite.Color, (int)(uint32_t)entity);
			}
		}

	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-Virtual Scene Cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}
}
