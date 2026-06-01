#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <vector>
#include "Quentlam/Renderer/Camera.h"
#include "Quentlam/Renderer/PerspectiveCamera.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Renderer/SubTexture2D.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Gameplay/LODComponent.h"


namespace Quentlam
{
	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string tag)
			:Tag(tag) {
		}
	};


	struct TransformComponent
	{
		glm::mat4 Transform{ 1.0f };

		entt::entity Parent = entt::null;
		std::vector<entt::entity> Children;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::mat4& transform)
			:Transform(transform) {
		}

		glm::mat4 GetWorldTransform(entt::registry& registry) const;

		operator glm::mat4& () { return Transform; }
	};

	struct SceneGroupComponent
	{
		bool Expanded = true;
	};

	inline glm::mat4 TransformComponent::GetWorldTransform(entt::registry& registry) const
	{
		if (Parent != entt::null && registry.any_of<TransformComponent>(Parent))
		{
			const TransformComponent& parentTransform = registry.get<TransformComponent>(Parent);
			return parentTransform.GetWorldTransform(registry) * Transform;
		}
		return Transform;
	}

	struct SpriteTransformComponent
	{
		glm::vec4 Color{ 1.0f,1.0f,1.0f,1.0f };
		Ref<Texture2D> Texture = nullptr;
		bool BackfaceCulling = true;

		SpriteTransformComponent() = default;
		SpriteTransformComponent(const SpriteTransformComponent&) = default;
		SpriteTransformComponent(const glm::vec4& color)
			:Color(color) {
		}
	};

	struct TriangleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;

		TriangleRendererComponent() = default;
		TriangleRendererComponent(const TriangleRendererComponent&) = default;
		TriangleRendererComponent(const glm::vec4& color)
			: Color(color) {}
	};

	struct CubeRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float AmbientStrength = 0.3f;
		float DiffuseStrength = 0.8f;
		float SpecularStrength = 0.5f;
		float Shininess = 32.0f;

		CubeRendererComponent() = default;
		CubeRendererComponent(const CubeRendererComponent&) = default;
		CubeRendererComponent(const glm::vec4& color)
			: Color(color) {}
	};

	struct PrimitiveRendererComponent
	{
		enum class PrimitiveType { Cube, Sphere, Cylinder, Capsule, Cone, Torus };
		PrimitiveType Type = PrimitiveType::Cube;
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		int Segments = 16;
		float Radius = 0.5f;
		float Height = 1.0f;

		float AmbientStrength = 0.3f;
		float DiffuseStrength = 0.8f;
		float SpecularStrength = 0.5f;
		float Shininess = 32.0f;

		PrimitiveRendererComponent() = default;
		PrimitiveRendererComponent(const PrimitiveRendererComponent&) = default;
		PrimitiveRendererComponent(PrimitiveType type) : Type(type) {}
	};

	struct CameraComponent
	{
		Quentlam::PerspectiveCamera _camera;
		bool FixedAspectRatio = false;
		bool IsGameCamera = false;
		bool ShowFrustum = true;       // 是否在编辑器中显示视锥体
		float FrustumSize = 50.0f;     // 视锥体远平面大小倍数（相对于 nearClip）

		CameraComponent() : _camera(60.0f, 1.78f, 0.1f, 1000.0f) {}
		CameraComponent(const CameraComponent&) = default;
		CameraComponent(const glm::mat4& projection)
			:_camera(60.0f, 1.78f, 0.1f, 1000.0f) {
		}
	};

	struct CameraFollowComponent
	{
		entt::entity Target = entt::null;
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Smoothing = 5.0f;
		bool UseBounds = false;
		glm::vec2 BoundsMin = { 0.0f, 0.0f };
		glm::vec2 BoundsMax = { 100.0f, 100.0f };
		bool Enabled = true;

		CameraFollowComponent() = default;
		CameraFollowComponent(const CameraFollowComponent&) = default;
	};

	struct UIEntityComponent
	{
		std::string ScreenId;
		bool Visible = true;
		bool Blocking = false;
		bool Focusable = true;
		int32_t ZOrder = 0;
		EInputContext InputContext = EInputContext::Gameplay;

		UIEntityComponent() = default;
		UIEntityComponent(const UIEntityComponent&) = default;
	};

	struct PrefabReferenceComponent
	{
		std::string PrefabPath;
		std::string PrefabName;
		bool IsValid = false;

		PrefabReferenceComponent() = default;
		PrefabReferenceComponent(const PrefabReferenceComponent&) = default;
		PrefabReferenceComponent(const std::string& path, const std::string& name)
			: PrefabPath(path), PrefabName(name), IsValid(true) {}
	};

	// =========================================================================
	// Audio Components
	// =========================================================================

	struct AudioSourceComponent
	{
		std::string AudioPath;
		float Volume = 1.0f;
		float Pitch = 1.0f;
		bool Loop = false;
		bool PlayOnAwake = false;
		bool Is3D = false;
		float MinDistance = 1.0f;
		float MaxDistance = 100.0f;

		AudioSourceComponent() = default;
		AudioSourceComponent(const AudioSourceComponent&) = default;
	};

	struct AudioListenerComponent
	{
		bool Enabled = true;
		float Volume = 1.0f;

		AudioListenerComponent() = default;
		AudioListenerComponent(const AudioListenerComponent&) = default;
	};

	// =========================================================================
	// 2D Physics Components
	// =========================================================================

	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Support for Gravity Scale overrides
		float GravityScale = 1.0f;

		// Collision layer (bitmask, layer 0-31)
		uint32_t CollisionLayer = 1;
		uint32_t CollisionMask = 0xFFFFFFFF;

		// Storage for runtime body
		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO: Material properties
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		bool ShowCollider = true;

		// Storage for runtime fixture
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		bool ShowCollider = true;
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	struct TriangleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		bool ShowCollider = true;

		void* RuntimeFixture = nullptr;

		TriangleCollider2DComponent() = default;
		TriangleCollider2DComponent(const TriangleCollider2DComponent&) = default;
	};

	// =========================================================================
	// 3D Physics Components
	// =========================================================================

	// =========================================================================
	// Lua Script Component
	// =========================================================================

	struct LuaScriptComponent
	{
		std::string ScriptPath;

		Ref<class LuaScriptInstance> Instance;

		LuaScriptComponent() = default;
		LuaScriptComponent(const LuaScriptComponent&) = default;
		LuaScriptComponent(const std::string& path) : ScriptPath(path) {}
	};

	struct Rigidbody3DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;

		float Mass = 1.0f;
		bool FixedRotation = false;

		// Storage for runtime body (Jolt Physics Body ID)
		uint32_t RuntimeBodyID = 0xFFFFFFFF; // JPH::BodyID::cInvalidBodyID

		Rigidbody3DComponent() = default;
		Rigidbody3DComponent(const Rigidbody3DComponent&) = default;
	};

	struct BoxCollider3DComponent
	{
		glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
		glm::vec3 HalfExtent = { 0.5f, 0.5f, 0.5f };

		float Friction = 0.5f;
		float Restitution = 0.0f;

		bool ShowCollider = true;

		BoxCollider3DComponent() = default;
		BoxCollider3DComponent(const BoxCollider3DComponent&) = default;
	};

	struct UIComponent
	{
		std::string ScreenId;
		Ref<UIScreen> Screen;

		bool IsShowing = true;
		bool Focusable = false;
		bool Blocking = false;
		EInputContext InputContext = EInputContext::Menu;

		UIComponent() = default;
		UIComponent(const std::string& id) : ScreenId(id) {}
		UIComponent(const UIComponent&) = default;
	};

	struct SpriteAnimationComponent
	{
		Ref<class Animator> Animator;
		Ref<Texture2D> AtlasTexture;
		Ref<SpriteAtlasBinding> AtlasBinding;

		std::string DefaultClipName;
		std::string AnimationDataPath;

		bool AutoPlay = true;
		bool IsPlaying() const { return Animator && Animator->IsPlaying(); }

		enum class EDirection { Down = 0, Left = 1, Right = 2, Up = 3, DownLeft = 4, DownRight = 5, UpLeft = 6, UpRight = 7 };
		EDirection CurrentDirection = EDirection::Down;

		void Play(const std::string& clipName)
		{
			if (Animator)
				Animator->Play(clipName);
		}

		void Stop()
		{
			if (Animator)
				Animator->Stop();
		}

		void Pause()
		{
			if (Animator)
				Animator->Pause();
		}

		void Resume()
		{
			if (Animator)
				Animator->Resume();
		}

		Ref<AnimationClip> GetCurrentClip() const
		{
			return Animator ? Animator->GetCurrentClip() : nullptr;
		}

		int32_t GetCurrentFrameIndex() const
		{
			return Animator ? Animator->GetCurrentFrame() : -1;
		}

		Ref<SubTexture2D> GetCurrentSubTexture() const
		{
			if (!Animator || !AtlasBinding || !GetCurrentClip() || !AtlasTexture)
				return nullptr;

			const auto* frameData = Animator->GetCurrentFrameData();
			if (!frameData)
				return nullptr;

			int32_t spriteIdx = frameData->SpriteIndex;
			int32_t col = spriteIdx % AtlasBinding->AtlasColumns;
			int32_t row = spriteIdx / AtlasBinding->AtlasColumns;

			return SubTexture2D::CreateFromCoords(
				AtlasTexture,
				{ static_cast<float>(col), static_cast<float>(row) },
				{ 1.0f, 1.0f },
				{ 1, 1 }
			);
		}

		SpriteAnimationComponent() = default;
		SpriteAnimationComponent(const SpriteAnimationComponent&) = default;
	};

	struct InteractableComponent
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

		EInteractionType InteractionType = EInteractionType::Click;
		EInteractionPriority Priority = EInteractionPriority::Normal;

		float InteractionRadius = 1.5f;
		float InteractionCooldown = 0.0f;
		float LastInteractionTime = -999.0f;

		std::string InteractionId;
		std::string DisplayName;
		std::string PromptText = "Press E";
		std::string PromptKeybind = "E";

		bool IsEnabled = true;
		bool ShowPrompt = true;

		bool IsInRangeOf(const glm::vec2& actorPos, const glm::vec2& worldPos) const
		{
			return glm::distance(actorPos, worldPos) <= InteractionRadius;
		}

		InteractableComponent() = default;
		InteractableComponent(const InteractableComponent&) = default;
		InteractableComponent(const std::string& id) : InteractionId(id) {}
	};

	struct ParticleSystem2DComponent
	{
		Ref<class ParticleEmitter2D> Emitter;

		float EmissionRate = 20.0f;
		int32_t MaxParticles = 200;
		float ParticleLifetime = 1.0f;
		float ParticleSpeed = 2.0f;
		glm::vec2 ParticleVelocityMin = { -1.0f, -1.0f };
		glm::vec2 ParticleVelocityMax = { 1.0f, 1.0f };
		glm::vec2 ParticleSizeMin = { 0.1f, 0.1f };
		glm::vec2 ParticleSizeMax = { 0.3f, 0.3f };
		glm::vec4 ParticleColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 ParticleColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
		float ParticleRotationMin = 0.0f;
		float ParticleRotationMax = 360.0f;
		float ParticleAngularVelocityMin = -180.0f;
		float ParticleAngularVelocityMax = 180.0f;
		float GravityModifier = 0.0f;
		bool IsPlaying = true;
		bool IsLooping = true;
		float StartDelay = 0.0f;
		float Duration = 5.0f;
		bool PlayOnAwake = true;

		Ref<Texture2D> ParticleTexture;

		bool IsActive() const { return IsPlaying && Emitter; }

		ParticleSystem2DComponent() = default;
		ParticleSystem2DComponent(const ParticleSystem2DComponent&) = default;
	};

	struct NavAgentComponent
	{
		enum class EAgentState : uint8_t
		{
			Idle = 0,
			Moving = 1,
			Paused = 2
		};

		glm::ivec2 TargetGridPos = { 0, 0 };
		bool HasTarget = false;
		bool AllowDiagonal = true;
		float Speed = 3.0f;
		float ArrivalThreshold = 0.1f;

		EAgentState State = EAgentState::Idle;
		std::vector<glm::ivec2> CurrentPath;
		int32_t PathIndex = 0;
		float MoveProgress = 0.0f;

		NavAgentComponent() = default;
		NavAgentComponent(const NavAgentComponent&) = default;
	};

	struct SceneReferenceComponent
	{
		enum class ELoadMode : uint8_t
		{
			Additive = 0,
			Single = 1
		};

		enum class ELoadState : uint8_t
		{
			None = 0,
			Loading = 1,
			Loaded = 2,
			Failed = 3
		};

		std::string ScenePath;
		ELoadMode LoadMode = ELoadMode::Additive;
		bool AutoLoad = false;
		bool AutoActivate = true;

		ELoadState State = ELoadState::None;
		float LoadProgress = 0.0f;
		std::string ErrorMessage;

		Ref<class Scene> LoadedScene;

		SceneReferenceComponent() = default;
		SceneReferenceComponent(const SceneReferenceComponent&) = default;
		SceneReferenceComponent(const std::string& path) : ScenePath(path) {}
	};

	struct AnimatorControllerComponent
	{
		Ref<class AnimatorController> Controller;
		std::string ControllerPath;
		bool AutoPlay = true;

		AnimatorControllerComponent() = default;
		AnimatorControllerComponent(const AnimatorControllerComponent&) = default;
	};

	// =========================================================================
	// Environment Components
	// =========================================================================

	struct DirectionalLightComponent
	{
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		glm::vec3 Direction = { -0.5f, -1.0f, -0.3f };

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
	};

	struct SkyboxComponent
	{
		std::string CubemapPath;
		bool Visible = true;
		enum class SkyMode { Procedural = 0, Cubemap = 1 };
		SkyMode Mode = SkyMode::Procedural;

		SkyboxComponent() = default;
		SkyboxComponent(const SkyboxComponent&) = default;
	};

	struct WeatherSystemComponent
	{
		float TimeOfDay = 0.4f;   // 0.0-1.0, 0.4 = ~10am
		float WeatherIntensity = 0.0f;
		float RainIntensity = 0.0f;
		bool Enabled = true;

		WeatherSystemComponent() = default;
		WeatherSystemComponent(const WeatherSystemComponent&) = default;
	};

	// =========================================================================
	// Trigger Components
	// =========================================================================

	struct TriggerComponent
	{
		bool IsTrigger = false;
		std::string EnterLuaFunction;
		std::string ExitLuaFunction;
		std::string StayLuaFunction;
		int32_t CollisionLayer = 1;

		TriggerComponent() = default;
		TriggerComponent(const TriggerComponent&) = default;
	};
}