#include "qlpch.h"

#include "Scene.h"
#include "Quentlam/Renderer/Renderer2D.h"
#include "Quentlam/Renderer/Renderer3D.h"
#include "Quentlam/Renderer/Material.h"
#include "SpriteRendererComponent.h"
#include "SpriteAnimationComponent.h"
#include "Entity.h"
#include "Components.h"
#include "Quentlam/Physics/Physics2D.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include "Quentlam/Physics/Physics3D.h"
#include "Quentlam/Physics/Physics3DValidation.h"
#include "Quentlam/Gameplay/NpcModule.h"
#include "Quentlam/Gameplay/QuestEventModule.h"
#include "Quentlam/Gameplay/InteractionModule.h"
#include "Quentlam/Gameplay/NavigationModule.h"
#include "Quentlam/Renderer/ParticleSystem2D.h"
#include "Quentlam/Modding/LuaScriptInstance.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/UI/ImageComponent.h"
#include "Quentlam/UI/TextComponent.h"
#include "Quentlam/UI/ButtonComponent.h"
#include "Quentlam/UI/RectTransformComponent.h"
#include "Quentlam/UI/CanvasComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
		: m_Name(name)
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

	void Scene::CreateDefaultScene(Entity* outSceneCamera, Entity* outGameCamera)
	{
		auto sky = CreateEntity("天空");
		sky.AddComponent<WeatherSystemComponent>();
		auto& dlc = sky.AddComponent<DirectionalLightComponent>();
		dlc.Color = { 1.0f, 1.0f, 1.0f };
		dlc.Intensity = 1.0f;
		dlc.Direction = { -0.5f, -1.0f, -0.3f };
		sky.AddComponent<SkyboxComponent>();

		// UE-like sky sphere: large sphere mesh with gradient sky material
		auto skySphere = CreateEntity("SkySphere");
		auto& ssTC = skySphere.GetComponent<TransformComponent>();
		ssTC.Transform = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f, 500.0f, 500.0f));
		auto& ssSkyComp = skySphere.AddComponent<SkyboxComponent>();
		ssSkyComp.Mode = SkyboxComponent::SkyMode::Procedural;
		ssSkyComp.Visible = true;

		// Ground: static rigid body with box collider and 2D sprite
		auto groundEnt = CreateEntity("Ground");
		auto& gTC = groundEnt.GetComponent<TransformComponent>();
		gTC.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 1.0f));
		auto& gSprite = groundEnt.AddComponent<SpriteTransformComponent>();
		gSprite.Color = { 0.5f, 0.5f, 0.5f, 1.0f };
		auto& gRB = groundEnt.AddComponent<Rigidbody2DComponent>();
		gRB.Type = Rigidbody2DComponent::BodyType::Static;
		gRB.CollisionLayer = 1;
		gRB.CollisionMask = 0xFFFFFFFF;
		auto& gBC = groundEnt.AddComponent<BoxCollider2DComponent>();
		gBC.Offset = { 0.0f, 0.0f };
		gBC.Size = { 1.0f, 1.0f };
		gBC.Friction = 0.8f;
		gBC.Density = 1.0f;

		// Default cube for editor
		auto cubeEnt = CreateEntity("Default Cube");
		auto& cubeTC = cubeEnt.GetComponent<TransformComponent>();
		cubeTC.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		cubeEnt.AddComponent<CubeRendererComponent>(glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));

		auto sceneCameraEnt = CreateEntity("场景摄像机");
		auto& sceneCam = sceneCameraEnt.AddComponent<CameraComponent>();
		float aspect = (float)m_ViewportWidth / (float)m_ViewportHeight;
		if (aspect <= 0.0f) aspect = 1280.0f / 720.0f;
		sceneCam._camera.SetOrthographic(-aspect * 0.5f, aspect * 0.5f, -0.5f, 0.5f);
		sceneCam.FixedAspectRatio = false;
		auto& sceneCube = sceneCameraEnt.AddComponent<CubeRendererComponent>();
		sceneCube.Color = { 0.2f, 0.6f, 1.0f, 0.5f }; // semi-transparent blue

		auto gameCameraEnt = CreateEntity("游戏摄像机");
		auto& gameCam = gameCameraEnt.AddComponent<CameraComponent>();
		gameCam._camera.SetPerspective(60.0f, aspect, 0.1f, 1000.0f);
		gameCam.FixedAspectRatio = false;
		gameCam.IsGameCamera = true;

		if (outSceneCamera) *outSceneCamera = sceneCameraEnt;
		if (outGameCamera) *outGameCamera = gameCameraEnt;
		m_ActiveGameCamera = static_cast<entt::entity>(gameCameraEnt);
	}

	void Scene::FindDefaultCameraEntities(Entity* outSceneCamera, Entity* outGameCamera)
	{
		if (outSceneCamera) *outSceneCamera = Entity();
		if (outGameCamera) *outGameCamera = Entity();

		auto view = m_Registry.view<TagComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == "场景摄像机" && outSceneCamera)
				*outSceneCamera = Entity(entity, this);
			else if (tag.Tag == "游戏摄像机" && outGameCamera)
				*outGameCamera = Entity(entity, this);
		}
	}

	void Scene::SyncSceneCameraFromController(const glm::vec3& position, bool is3D, float aspect)
	{
		auto view = m_Registry.view<TagComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == "场景摄像机")
			{
				auto& tc = m_Registry.get<TransformComponent>(entity);
				tc.Transform[3][0] = position.x;
				tc.Transform[3][1] = position.y;
				tc.Transform[3][2] = is3D ? position.z : 10.0f;

				auto& camComp = m_Registry.get<CameraComponent>(entity);
				if (is3D)
					camComp._camera.SetPerspective(35.0f, aspect, 0.1f, 1000.0f);
				else
					camComp._camera.SetOrthographic(-aspect * 0.5f, aspect * 0.5f, -0.5f, 0.5f);
				break;
			}
		}
	}

	void Scene::DestroyEntity(entt::entity entity)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto e : view)
		{
			if (e == entity)
			{
				auto& tag = view.get<TagComponent>(e);
				if (tag.Tag == "场景摄像机")
				{
					QL_CORE_ERROR("Cannot delete the Scene Camera entity! It is a protected system entity.");
					return;
				}
				if (tag.Tag == "游戏摄像机")
				{
					QL_CORE_ERROR("Cannot delete the Game Camera entity! It is a protected system entity.");
					return;
				}
				break;
			}
		}
		m_Registry.destroy(entity);
	}

	Entity Scene::GetOrCreateSceneCamera()
	{
		auto view = m_Registry.view<TagComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == "场景摄像机")
				return Entity(entity, this);
		}
		QL_CORE_WARN("Scene camera not found, recreating...");
		Entity cam = { m_Registry.create(), this };
		cam.AddComponent<TransformComponent>();
		cam.AddComponent<TagComponent>("场景摄像机");
		auto& camComp = cam.AddComponent<CameraComponent>();
		float aspect = (float)m_ViewportWidth / (float)m_ViewportHeight;
		if (aspect <= 0.0f) aspect = 1280.0f / 720.0f;
		camComp._camera.SetOrthographic(-aspect * 0.5f, aspect * 0.5f, -0.5f, 0.5f);
		camComp.FixedAspectRatio = false;
		return cam;
	}

	Entity Scene::GetOrCreateGameCamera()
	{
		auto view = m_Registry.view<TagComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == "游戏摄像机")
				return Entity(entity, this);
		}
		QL_CORE_WARN("Game camera not found, recreating...");
		Entity cam = { m_Registry.create(), this };
		cam.AddComponent<TransformComponent>();
		cam.AddComponent<TagComponent>("游戏摄像机");
		auto& camComp = cam.AddComponent<CameraComponent>();
		float aspect = (float)m_ViewportWidth / (float)m_ViewportHeight;
		if (aspect <= 0.0f) aspect = 1280.0f / 720.0f;
		camComp._camera.SetOrthographic(-aspect * 0.5f, aspect * 0.5f, -0.5f, 0.5f);
		camComp.FixedAspectRatio = false;
		camComp.IsGameCamera = true;
		m_ActiveGameCamera = static_cast<entt::entity>(cam);
		return cam;
	}

	Entity Scene::GetActiveGameCamera()
	{
		if (m_ActiveGameCamera != entt::null && m_Registry.valid(m_ActiveGameCamera))
			return Entity(m_ActiveGameCamera, this);
		return GetOrCreateGameCamera();
	}

	void Scene::SetActiveGameCamera(Entity entity)
	{
		if (entity && m_Registry.valid(static_cast<entt::entity>(entity)))
		{
			m_ActiveGameCamera = static_cast<entt::entity>(entity);
		}
		else
		{
			m_ActiveGameCamera = entt::null;
		}
	}

	Entity Scene::CreateEntityWithData(const std::string& name, const std::string& entityJson)
	{
		SceneSerializer serializer(this);
		return serializer.DeserializeEntity(this, entityJson);
	}

	std::string Scene::SerializeEntityToString(entt::entity entity)
	{
		if (!m_Registry.valid(entity))
			return {};
		auto& reg = m_Registry;
		auto& tag = reg.get<TagComponent>(entity);
		std::ostringstream ss;
		ss << "{\"entity_id\": " << static_cast<uint32_t>(entity) << ", \"tag\": \"" << tag.Tag << "\", \"components\": {";
		bool firstComp = true;
		auto writeHeader = [&](const char* name) {
			if (!firstComp) ss << ", ";
			firstComp = false;
			ss << "\"" << name << "\": ";
		};
		if (reg.any_of<TransformComponent>(entity)) {
			auto& tc = reg.get<TransformComponent>(entity);
			writeHeader("TransformComponent");
			ss << "{ \"matrix\": [";
			for (int c = 0; c < 4; c++) { ss << "["; for (int r = 0; r < 4; r++) { ss << tc.Transform[c][r]; if (r < 3) ss << ", "; } ss << "]"; if (c < 3) ss << ", "; }
			ss << "] }";
		}
		if (reg.any_of<SpriteRendererComponent>(entity)) {
			auto& src = reg.get<SpriteRendererComponent>(entity);
			writeHeader("SpriteRendererComponent");
			ss << "{ \"color\": [" << src.Color.r << ", " << src.Color.g << ", " << src.Color.b << ", " << src.Color.a << "], \"size\": [" << src.Size.x << ", " << src.Size.y << "], \"tiling_factor\": " << src.TilingFactor << ", \"sorting_order\": " << src.SortingOrder << ", \"flip_x\": " << (src.FlipX ? "true" : "false") << ", \"flip_y\": " << (src.FlipY ? "true" : "false") << " }";
		}
		if (reg.any_of<SpriteTransformComponent>(entity)) {
			auto& stc = reg.get<SpriteTransformComponent>(entity);
			writeHeader("SpriteTransformComponent");
			ss << "{ \"color\": [" << stc.Color.r << ", " << stc.Color.g << ", " << stc.Color.b << ", " << stc.Color.a << "] }";
		}
		if (reg.any_of<TriangleRendererComponent>(entity)) {
			auto& trc = reg.get<TriangleRendererComponent>(entity);
			writeHeader("TriangleRendererComponent");
			ss << "{ \"color\": [" << trc.Color.r << ", " << trc.Color.g << ", " << trc.Color.b << ", " << trc.Color.a << "] }";
		}
		if (reg.any_of<CubeRendererComponent>(entity)) {
			auto& crc = reg.get<CubeRendererComponent>(entity);
			writeHeader("CubeRendererComponent");
			ss << "{ \"color\": [" << crc.Color.r << ", " << crc.Color.g << ", " << crc.Color.b << ", " << crc.Color.a << "], \"ambient\": " << crc.AmbientStrength << ", \"diffuse\": " << crc.DiffuseStrength << ", \"specular\": " << crc.SpecularStrength << ", \"shininess\": " << crc.Shininess << " }";
		}
		if (reg.any_of<PrimitiveRendererComponent>(entity)) {
			auto& prc = reg.get<PrimitiveRendererComponent>(entity);
			writeHeader("PrimitiveRendererComponent");
			ss << "{ \"color\": [" << prc.Color.r << ", " << prc.Color.g << ", " << prc.Color.b << ", " << prc.Color.a << "], \"primitive_type\": " << static_cast<int>(prc.Type) << " }";
		}
		if (reg.any_of<Rigidbody2DComponent>(entity)) {
			auto& rb = reg.get<Rigidbody2DComponent>(entity);
			writeHeader("Rigidbody2DComponent");
			ss << "{ \"type\": " << static_cast<int>(rb.Type) << ", \"fixed_rotation\": " << (rb.FixedRotation ? "true" : "false") << ", \"gravity_scale\": " << rb.GravityScale << " }";
		}
		if (reg.any_of<BoxCollider2DComponent>(entity)) {
			auto& bc = reg.get<BoxCollider2DComponent>(entity);
			writeHeader("BoxCollider2DComponent");
			ss << "{ \"offset\": [" << bc.Offset.x << ", " << bc.Offset.y << "], \"size\": [" << bc.Size.x << ", " << bc.Size.y << "], \"density\": " << bc.Density << ", \"friction\": " << bc.Friction << ", \"restitution\": " << bc.Restitution << ", \"show_collider\": " << (bc.ShowCollider ? "true" : "false") << " }";
		}
		if (reg.any_of<CircleCollider2DComponent>(entity)) {
			auto& cc = reg.get<CircleCollider2DComponent>(entity);
			writeHeader("CircleCollider2DComponent");
			ss << "{ \"offset\": [" << cc.Offset.x << ", " << cc.Offset.y << "], \"radius\": " << cc.Radius << ", \"density\": " << cc.Density << ", \"friction\": " << cc.Friction << ", \"restitution\": " << cc.Restitution << " }";
		}
		if (reg.any_of<TriangleCollider2DComponent>(entity)) {
			auto& tc2d = reg.get<TriangleCollider2DComponent>(entity);
			writeHeader("TriangleCollider2DComponent");
			ss << "{ \"offset\": [" << tc2d.Offset.x << ", " << tc2d.Offset.y << "], \"size\": [" << tc2d.Size.x << ", " << tc2d.Size.y << "], \"density\": " << tc2d.Density << ", \"friction\": " << tc2d.Friction << " }";
		}
		if (reg.any_of<Rigidbody3DComponent>(entity)) {
			auto& rb3d = reg.get<Rigidbody3DComponent>(entity);
			writeHeader("Rigidbody3DComponent");
			ss << "{ \"type\": " << static_cast<int>(rb3d.Type) << ", \"mass\": " << rb3d.Mass << " }";
		}
		if (reg.any_of<BoxCollider3DComponent>(entity)) {
			auto& bc3d = reg.get<BoxCollider3DComponent>(entity);
			writeHeader("BoxCollider3DComponent");
			ss << "{ \"half_extent\": [" << bc3d.HalfExtent.x << ", " << bc3d.HalfExtent.y << ", " << bc3d.HalfExtent.z << "] }";
		}
		if (reg.any_of<AudioSourceComponent>(entity)) {
			auto& au = reg.get<AudioSourceComponent>(entity);
			writeHeader("AudioSourceComponent");
			ss << "{ \"audio_path\": \"" << au.AudioPath << "\", \"volume\": " << au.Volume << ", \"pitch\": " << au.Pitch << ", \"loop\": " << (au.Loop ? "true" : "false") << ", \"play_on_awake\": " << (au.PlayOnAwake ? "true" : "false") << ", \"is_3d\": " << (au.Is3D ? "true" : "false") << ", \"min_distance\": " << au.MinDistance << ", \"max_distance\": " << au.MaxDistance << " }";
		}
		if (reg.any_of<AudioListenerComponent>(entity)) {
			auto& al = reg.get<AudioListenerComponent>(entity);
			writeHeader("AudioListenerComponent");
			ss << "{ \"enabled\": " << (al.Enabled ? "true" : "false") << ", \"volume\": " << al.Volume << " }";
		}
		if (reg.any_of<LuaScriptComponent>(entity)) {
			auto& lc = reg.get<LuaScriptComponent>(entity);
			writeHeader("LuaScriptComponent");
			ss << "{ \"script_path\": \"" << lc.ScriptPath << "\" }";
		}
		if (reg.any_of<SpriteAnimationComponent>(entity)) {
			auto& sac = reg.get<SpriteAnimationComponent>(entity);
			writeHeader("SpriteAnimationComponent");
			ss << "{ \"animation_data_path\": \"" << sac.AnimationDataPath << "\", \"default_clip\": \"" << sac.DefaultClipName << "\", \"auto_play\": " << (sac.AutoPlay ? "true" : "false") << " }";
		}
		if (reg.any_of<UIEntityComponent>(entity)) {
			auto& ui = reg.get<UIEntityComponent>(entity);
			writeHeader("UIEntityComponent");
			ss << "{ \"screen_id\": \"" << ui.ScreenId << "\", \"visible\": " << (ui.Visible ? "true" : "false") << ", \"blocking\": " << (ui.Blocking ? "true" : "false") << ", \"focusable\": " << (ui.Focusable ? "true" : "false") << ", \"z_order\": " << ui.ZOrder << " }";
		}
		if (reg.any_of<CanvasComponent>(entity)) {
			writeHeader("CanvasComponent");
			ss << "{ }";
		}
		if (reg.any_of<ImageComponent>(entity)) {
			auto& ic = reg.get<ImageComponent>(entity);
			writeHeader("ImageComponent");
			ss << "{ \"sprite_path\": \"" << ic.SpritePath << "\", \"color\": [" << ic.Color.r << ", " << ic.Color.g << ", " << ic.Color.b << ", " << ic.Color.a << "], \"image_type\": " << static_cast<int>(ic.ImageType) << ", \"fill_amount\": " << ic.FillAmount << " }";
		}
		if (reg.any_of<TextComponent>(entity)) {
			auto& txc = reg.get<TextComponent>(entity);
			writeHeader("TextComponent");
			ss << "{ \"text\": \"" << txc.Text << "\", \"color\": [" << txc.Color.r << ", " << txc.Color.g << ", " << txc.Color.b << ", " << txc.Color.a << "], \"font_size\": " << txc.FontSize << " }";
		}
		if (reg.any_of<ButtonComponent>(entity)) {
			auto& btc = reg.get<ButtonComponent>(entity);
			writeHeader("ButtonComponent");
			ss << "{ \"normal_sprite\": \"" << btc.NormalSpritePath << "\", \"highlighted_sprite\": \"" << btc.HighlightedSpritePath << "\", \"pressed_sprite\": \"" << btc.PressedSpritePath << "\", \"disabled_sprite\": \"" << btc.DisabledSpritePath << "\", \"transition\": " << static_cast<int>(btc.Transition) << " }";
		}
		if (reg.any_of<PrefabReferenceComponent>(entity)) {
			auto& pr = reg.get<PrefabReferenceComponent>(entity);
			writeHeader("PrefabReferenceComponent");
			ss << "{ \"prefab_path\": \"" << pr.PrefabPath << "\", \"prefab_name\": \"" << pr.PrefabName << "\" }";
		}
		if (reg.any_of<SceneReferenceComponent>(entity)) {
			auto& src = reg.get<SceneReferenceComponent>(entity);
			writeHeader("SceneReferenceComponent");
			ss << "{ \"scene_path\": \"" << src.ScenePath << "\" }";
		}
		ss << "}}";
		return ss.str();
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

		InstantiateLuaScripts();

		NpcModule::Get().OnDayStart();
		QuestEventModule::Get().AdvanceDay();

		// UI components are managed via Scene's ECS system.
		// For GameUILayer-integrated projects, UI screen rendering
		// is handled by GameUILayer::OnImGuiLayer() instead.
		// TODO: Uncomment when GameUILayer is refactored to consume Scene UI components
		// InstantiateUIComponents();

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
	DestroyLuaScripts();
	// TODO: Uncomment when GameUILayer is refactored to consume Scene UI components
	// DestroyUIComponents();
}

	void Scene::InstantiateLuaScripts()
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto entity : view)
		{
			auto& lc = m_Registry.get<LuaScriptComponent>(entity);
			if (lc.ScriptPath.empty())
				continue;

			uint32_t id = static_cast<uint32_t>(entity);
			lc.Instance = CreateRef<LuaScriptInstance>(this, id, lc.ScriptPath);
			if (!lc.Instance->Instantiate())
			{
				QL_CORE_ERROR("Failed to instantiate script for entity {0}: {1}",
					id, lc.ScriptPath);
				lc.Instance.reset();
				continue;
			}
			lc.Instance->OnLoad();
			QL_CORE_INFO("Lua script instantiated for entity {0}: {1}", id, lc.ScriptPath);
		}
	}

	void Scene::UpdateLuaScripts(Timestep ts)
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto entity : view)
		{
			auto& lc = m_Registry.get<LuaScriptComponent>(entity);
			if (!lc.Instance)
				continue;
			lc.Instance->OnUpdate(ts.GetSeconds());
		}
	}

	void Scene::DestroyLuaScripts()
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto entity : view)
		{
			auto& lc = m_Registry.get<LuaScriptComponent>(entity);
			if (lc.Instance)
			{
				lc.Instance->OnDestroy();
				lc.Instance.reset();
			}
		}
	}

	void Scene::InstantiateUIComponents()
	{
		auto view = m_Registry.view<UIComponent>();
		for (auto entity : view)
		{
			auto& ui = m_Registry.get<UIComponent>(entity);
			if (!ui.Screen || ui.ScreenId.empty())
				continue;
			UIGameModule::Get().RegisterScreen(ui.Screen);
			if (ui.IsShowing)
				ScreenStack::Get().Push(ui.Screen);
		}
	}

	void Scene::UpdateUIComponents(Timestep ts)
	{
		UIGameModule::Get().Update(ts.GetSeconds());
	}

	void Scene::RenderUIComponents()
	{
		ScreenStack::Get().Render();
	}

	void Scene::DestroyUIComponents()
	{
		auto view = m_Registry.view<UIComponent>();
		for (auto entity : view)
		{
			auto& ui = m_Registry.get<UIComponent>(entity);
			if (ui.Screen && !ui.ScreenId.empty())
				UIGameModule::Get().UnregisterScreen(ui.ScreenId);
		}
		ScreenStack::Get().PopAll();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		if (ts > 0.1f) ts = 0.1f;
		if (ts > 0.0f)
		{
			Physics2D::OnUpdate(this, ts);
			Physics3D::OnUpdate(this, ts);
		}

		// E7-5: Sync DirectionalLightComponent to Renderer3D
		{
			auto dlView = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : dlView)
			{
				auto& tc = dlView.get<TransformComponent>(entity);
				auto& dlc = dlView.get<DirectionalLightComponent>(entity);
				glm::mat4 world = tc.GetWorldTransform(m_Registry);
				glm::vec3 worldDir = glm::normalize(glm::mat3(world) * dlc.Direction);
				Renderer3D::SetDirectionalLight(worldDir, dlc.Color, dlc.Intensity);
				break;
			}
		}

		NpcModule::Get().Update(ts.GetSeconds());
		QuestEventModule::Get().Update(ts.GetSeconds());
		InteractionModule::Get().OnUpdate(this, ts.GetSeconds());
		NavigationModule::Get().Update(this, ts.GetSeconds());

		{
			auto animView = m_Registry.view<SpriteAnimationComponent>();
			for (auto entity : animView)
			{
				auto& anim = animView.get<SpriteAnimationComponent>(entity);
				if (anim.Animator)
					anim.Animator->Update(ts.GetSeconds());
			}
		}

		{
			auto psView = m_Registry.view<TransformComponent, ParticleSystem2DComponent>();
			for (auto entity : psView)
			{
				auto [transform, ps] = psView.get<TransformComponent, ParticleSystem2DComponent>(entity);
				if (!ps.Emitter)
				{
					ParticleSystemConfig config;
					config.EmissionRate = ps.EmissionRate;
					config.MaxParticles = ps.MaxParticles;
					config.ParticleLifetime = ps.ParticleLifetime;
					config.ParticleSpeed = ps.ParticleSpeed;
					config.VelocityMin = ps.ParticleVelocityMin;
					config.VelocityMax = ps.ParticleVelocityMax;
					config.SizeMin = ps.ParticleSizeMin;
					config.SizeMax = ps.ParticleSizeMax;
					config.ColorBegin = ps.ParticleColorBegin;
					config.ColorEnd = ps.ParticleColorEnd;
					config.RotationMin = ps.ParticleRotationMin;
					config.RotationMax = ps.ParticleRotationMax;
					config.AngularVelocityMin = ps.ParticleAngularVelocityMin;
					config.AngularVelocityMax = ps.ParticleAngularVelocityMax;
					config.GravityModifier = ps.GravityModifier;
					config.Looping = ps.IsLooping;
					config.StartDelay = ps.StartDelay;
					config.Duration = ps.Duration;
					config.Texture = ps.ParticleTexture;
					ps.Emitter = CreateRef<ParticleEmitter2D>(config);
				}
				if (ps.IsPlaying && !ps.Emitter->IsPlaying())
					ps.Emitter->Play();
				if (!ps.IsPlaying && ps.Emitter->IsPlaying())
					ps.Emitter->Stop();

				glm::vec3 pos3 = glm::vec3(transform.GetWorldTransform(m_Registry)[3]);
				ps.Emitter->SetPosition({ pos3.x, pos3.y });
				ps.Emitter->Update(ts.GetSeconds());
			}
		}

		{
			auto camView = m_Registry.view<CameraComponent, CameraFollowComponent>();
			for (auto entity : camView)
			{
				auto& follow = camView.get<CameraFollowComponent>(entity);
				if (!follow.Enabled || follow.Target == entt::null)
					continue;

				auto targetEntity = Entity(follow.Target, this);
				if (!targetEntity)
					continue;

				auto& targetTransform = targetEntity.GetComponent<TransformComponent>();
				glm::mat4 targetWorld = targetTransform.GetWorldTransform(m_Registry);
				glm::vec3 targetPos(targetWorld[3]);
				glm::vec2 desired = { targetPos.x + follow.Offset.x, targetPos.y + follow.Offset.y };

				auto& camComp = camView.get<CameraComponent>(entity);
				glm::vec3 current = camComp._camera.GetPosition();
				glm::vec2 current2 = { current.x, current.y };
				float t = 1.0f - exp(-follow.Smoothing * ts.GetSeconds());
				glm::vec2 newPos = glm::mix(current2, desired, t);

				if (follow.UseBounds)
				{
					newPos.x = std::clamp(newPos.x, follow.BoundsMin.x, follow.BoundsMax.x);
					newPos.y = std::clamp(newPos.y, follow.BoundsMin.y, follow.BoundsMax.y);
				}

				camComp._camera.SetPosition({ newPos.x, newPos.y, current.z });
			}
		}

		UpdateLuaScripts(ts);
		// TODO: Uncomment when GameUILayer is refactored
		// UpdateUIComponents(ts);

		OnUpdate(ts);
	}

	void Scene::OnStepFrame()
	{
		Timestep ts(1.0f / 60.0f);
		if (ts > 0.0f)
		{
			Physics2D::OnUpdate(this, ts);
			Physics3D::OnUpdate(this, ts);
		}

		NpcModule::Get().Update(ts.GetSeconds());
		QuestEventModule::Get().Update(ts.GetSeconds());
		InteractionModule::Get().OnUpdate(this, ts.GetSeconds());
		NavigationModule::Get().Update(this, ts.GetSeconds());

		{
			auto animView = m_Registry.view<SpriteAnimationComponent>();
			for (auto entity : animView)
			{
				auto& anim = animView.get<SpriteAnimationComponent>(entity);
				if (anim.Animator)
					anim.Animator->Update(ts.GetSeconds());
			}
		}

		{
			auto psView = m_Registry.view<TransformComponent, ParticleSystem2DComponent>();
			for (auto entity : psView)
			{
				auto [transform, ps] = psView.get<TransformComponent, ParticleSystem2DComponent>(entity);
				if (!ps.Emitter)
				{
					ParticleSystemConfig config;
					config.EmissionRate = ps.EmissionRate;
					config.MaxParticles = ps.MaxParticles;
					config.ParticleLifetime = ps.ParticleLifetime;
					config.ParticleSpeed = ps.ParticleSpeed;
					config.VelocityMin = ps.ParticleVelocityMin;
					config.VelocityMax = ps.ParticleVelocityMax;
					config.SizeMin = ps.ParticleSizeMin;
					config.SizeMax = ps.ParticleSizeMax;
					config.ColorBegin = ps.ParticleColorBegin;
					config.ColorEnd = ps.ParticleColorEnd;
					config.RotationMin = ps.ParticleRotationMin;
					config.RotationMax = ps.ParticleRotationMax;
					config.AngularVelocityMin = ps.ParticleAngularVelocityMin;
					config.AngularVelocityMax = ps.ParticleAngularVelocityMax;
					config.GravityModifier = ps.GravityModifier;
					config.Looping = ps.IsLooping;
					config.StartDelay = ps.StartDelay;
					config.Duration = ps.Duration;
					config.Texture = ps.ParticleTexture;
					ps.Emitter = CreateRef<ParticleEmitter2D>(config);
				}
				if (ps.IsPlaying && !ps.Emitter->IsPlaying())
					ps.Emitter->Play();
				if (!ps.IsPlaying && ps.Emitter->IsPlaying())
					ps.Emitter->Stop();

				glm::vec3 pos3 = glm::vec3(transform.GetWorldTransform(m_Registry)[3]);
				ps.Emitter->SetPosition({ pos3.x, pos3.y });
				ps.Emitter->Update(ts.GetSeconds());
			}
		}

		UpdateLuaScripts(ts);
		OnUpdate(ts);
	}

	void Scene::OnRender2DOnly(Timestep ts)
	{
		auto spriteView = m_Registry.view<TransformComponent, SpriteTransformComponent>();

		for (auto entity : spriteView)
		{
			auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (sprite.Texture)
			{
				Renderer2D::DrawQuad(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(worldTransform, sprite.Color, (int)(uint32_t)entity);
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

			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (texToDraw)
			{
				Renderer2D::DrawQuad(worldTransform, texToDraw,
					src.TilingFactor, src.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(worldTransform, src.Color, (int)(uint32_t)entity);
			}
		}

		auto triView = m_Registry.view<TransformComponent, TriangleRendererComponent>();
		for (auto entity : triView)
		{
			auto [transform, tri] = triView.get<TransformComponent, TriangleRendererComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (tri.Texture)
			{
				Renderer2D::DrawTriangle(worldTransform, tri.Texture, 1.0f, tri.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawTriangle(worldTransform, tri.Color, (int)(uint32_t)entity);
			}
		}

		{
			auto psView = m_Registry.view<TransformComponent, ParticleSystem2DComponent>();
			for (auto entity : psView)
			{
				auto [transform, ps] = psView.get<TransformComponent, ParticleSystem2DComponent>(entity);
				if (!ps.Emitter || ps.Emitter->GetParticleCount() == 0)
					continue;

				const auto& particles = ps.Emitter->GetParticles();
				Ref<Texture2D> tex = ps.ParticleTexture ? ps.ParticleTexture : Renderer2D::GetWhiteTexture();

				for (const auto& p : particles)
				{
					float lifeRatio = p.MaxLife > 0.0f ? p.Life / p.MaxLife : 1.0f;
					glm::vec4 color = glm::mix(ps.ParticleColorEnd, ps.ParticleColorBegin, lifeRatio);

					glm::mat4 transformMat =
						glm::translate(glm::mat4(1.0f), { p.Position.x, p.Position.y, 0.0f })
						* glm::rotate(glm::mat4(1.0f), glm::radians(p.Rotation), { 0.0f, 0.0f, 1.0f })
						* glm::scale(glm::mat4(1.0f), { p.Size.x, p.Size.y, 1.0f });

					Renderer2D::DrawParticle(transformMat, p, tex, (int)(uint32_t)entity);
				}
			}
		}

		RenderLuaEntities();
	}

	void Scene::OnUpdate(Timestep ts)
	{
		auto spriteView = m_Registry.view<TransformComponent, SpriteTransformComponent>();

		for (auto entity : spriteView)
		{
			auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (sprite.Texture)
			{
				Renderer2D::DrawQuad(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(worldTransform, sprite.Color, (int)(uint32_t)entity);
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

			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (texToDraw)
			{
				Renderer2D::DrawQuad(worldTransform, texToDraw,
					src.TilingFactor, src.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawQuad(worldTransform, src.Color, (int)(uint32_t)entity);
			}
		}

		auto triView = m_Registry.view<TransformComponent, TriangleRendererComponent>();
		for (auto entity : triView)
		{
			auto [transform, sprite] = triView.get<TransformComponent, TriangleRendererComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_Registry);
			if (sprite.Texture)
			{
				Renderer2D::DrawTriangle(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			}
			else
			{
				Renderer2D::DrawTriangle(worldTransform, sprite.Color, (int)(uint32_t)entity);
			}
		}

		{
			auto psView = m_Registry.view<TransformComponent, ParticleSystem2DComponent>();
			for (auto entity : psView)
			{
				auto [transform, ps] = psView.get<TransformComponent, ParticleSystem2DComponent>(entity);
				if (!ps.Emitter || ps.Emitter->GetParticleCount() == 0)
					continue;

				const auto& particles = ps.Emitter->GetParticles();
				Ref<Texture2D> tex = ps.ParticleTexture ? ps.ParticleTexture : Renderer2D::GetWhiteTexture();

				for (const auto& p : particles)
				{
					float lifeRatio = p.MaxLife > 0.0f ? p.Life / p.MaxLife : 1.0f;
					glm::vec4 color = glm::mix(ps.ParticleColorEnd, ps.ParticleColorBegin, lifeRatio);

					glm::mat4 transformMat =
						glm::translate(glm::mat4(1.0f), { p.Position.x, p.Position.y, 0.0f })
						* glm::rotate(glm::mat4(1.0f), glm::radians(p.Rotation), { 0.0f, 0.0f, 1.0f })
						* glm::scale(glm::mat4(1.0f), { p.Size.x, p.Size.y, 1.0f });

					Renderer2D::DrawParticle(transformMat, p, tex, (int)(uint32_t)entity);
				}
			}
		}

		// Render entities driven by Lua scripts
		RenderLuaEntities();
		// TODO: Uncomment when GameUILayer is refactored
		// RenderUIComponents();

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
				cameraComponent._camera.SetViewportSize(width, height);
		}
	}

	void Scene::RenderLuaEntities()
	{
		auto scriptView = m_Registry.view<LuaScriptComponent>();
		for (auto entity : scriptView)
		{
			auto& lc = m_Registry.get<LuaScriptComponent>(entity);
			if (!lc.Instance || !lc.Instance->IsValid())
				continue;

			auto positions = lc.Instance->GetAllEntityPositions();
			auto colors = lc.Instance->GetAllEntityColors();
			auto visibility = lc.Instance->GetAllEntityVisibility();

			for (const auto& [eid, pos] : positions)
			{
				auto visIt = visibility.find(eid);
				if (visIt != visibility.end() && !visIt->second)
					continue;

				auto colIt = colors.find(eid);
				glm::vec4 color = colIt != colors.end() ? colIt->second : glm::vec4(1.0f);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos);
				Renderer2D::DrawQuad(transform, color, (int)eid);
			}
		}
	}

}
