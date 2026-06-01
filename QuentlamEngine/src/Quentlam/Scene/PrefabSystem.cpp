#include "qlpch.h"
#include "PrefabSystem.h"
#include "Scene.h"
#include "Entity.h"
#include "PrefabAsset.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/Scene/SpriteRendererComponent.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include <fstream>
#include <sstream>

namespace Quentlam
{

PrefabSystem& PrefabSystem::Get()
{
	static PrefabSystem instance;
	return instance;
}

static std::string ReadText(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) return "";
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

static bool WriteText(const std::string& path, const std::string& content)
{
	std::ofstream f(path);
	if (!f.is_open()) return false;
	f << content;
	return true;
}

static std::string GetBaseName(const std::string& path)
{
	size_t p = path.find_last_of("/\\");
	std::string name = (p == std::string::npos) ? path : path.substr(p + 1);
	size_t dot = name.find_last_of('.');
	return (dot == std::string::npos) ? name : name.substr(0, dot);
}

Ref<PrefabAsset> PrefabSystem::LoadPrefab(const std::string& filepath)
{
	auto it = m_LoadedPrefabs.find(filepath);
	if (it != m_LoadedPrefabs.end())
		return it->second;

	std::string content = ReadText(filepath);
	if (content.empty())
	{
		QL_CORE_WARN("PrefabSystem: Cannot read: {0}", filepath);
		return nullptr;
	}

	auto prefab = CreateRef<PrefabAsset>();
	prefab->SetName(GetBaseName(filepath));
	prefab->SetRawJson(std::move(content));

	m_LoadedPrefabs[filepath] = prefab;
	return prefab;
}

bool PrefabSystem::CreatePrefabFromEntity(entt::entity entity, Scene* scene, const std::string& filepath)
{
	if (entity == entt::null || !scene)
		return false;

	Scene tempScene;
	auto newEntity = tempScene.CreateEntity("Prefab");

	auto& tc = scene->GetRegistry().get<TransformComponent>(entity);
	newEntity.AddComponent<TransformComponent>(tc.Transform);

	auto& srcReg = scene->GetRegistry();

	if (srcReg.any_of<SpriteRendererComponent>(entity))
		newEntity.AddComponent<SpriteRendererComponent>(srcReg.get<SpriteRendererComponent>(entity));
	if (srcReg.any_of<LuaScriptComponent>(entity))
		newEntity.AddComponent<LuaScriptComponent>(srcReg.get<LuaScriptComponent>(entity));
	if (srcReg.any_of<Rigidbody2DComponent>(entity))
		newEntity.AddComponent<Rigidbody2DComponent>(srcReg.get<Rigidbody2DComponent>(entity));
	if (srcReg.any_of<Rigidbody3DComponent>(entity))
		newEntity.AddComponent<Rigidbody3DComponent>(srcReg.get<Rigidbody3DComponent>(entity));
	if (srcReg.any_of<BoxCollider2DComponent>(entity))
		newEntity.AddComponent<BoxCollider2DComponent>(srcReg.get<BoxCollider2DComponent>(entity));
	if (srcReg.any_of<CircleCollider2DComponent>(entity))
		newEntity.AddComponent<CircleCollider2DComponent>(srcReg.get<CircleCollider2DComponent>(entity));
	if (srcReg.any_of<TriangleCollider2DComponent>(entity))
		newEntity.AddComponent<TriangleCollider2DComponent>(srcReg.get<TriangleCollider2DComponent>(entity));
	if (srcReg.any_of<BoxCollider3DComponent>(entity))
		newEntity.AddComponent<BoxCollider3DComponent>(srcReg.get<BoxCollider3DComponent>(entity));
	if (srcReg.any_of<ParticleSystem2DComponent>(entity))
		newEntity.AddComponent<ParticleSystem2DComponent>(srcReg.get<ParticleSystem2DComponent>(entity));
	if (srcReg.any_of<NavAgentComponent>(entity))
		newEntity.AddComponent<NavAgentComponent>(srcReg.get<NavAgentComponent>(entity));
	if (srcReg.any_of<SpriteAnimationComponent>(entity))
		newEntity.AddComponent<SpriteAnimationComponent>(srcReg.get<SpriteAnimationComponent>(entity));
	if (srcReg.any_of<UIEntityComponent>(entity))
		newEntity.AddComponent<UIEntityComponent>(srcReg.get<UIEntityComponent>(entity));
	if (srcReg.any_of<PrefabReferenceComponent>(entity))
		newEntity.AddComponent<PrefabReferenceComponent>(srcReg.get<PrefabReferenceComponent>(entity));
	if (srcReg.any_of<AudioSourceComponent>(entity))
		newEntity.AddComponent<AudioSourceComponent>(srcReg.get<AudioSourceComponent>(entity));
	if (srcReg.any_of<AudioListenerComponent>(entity))
		newEntity.AddComponent<AudioListenerComponent>(srcReg.get<AudioListenerComponent>(entity));
	if (srcReg.any_of<CameraFollowComponent>(entity))
		newEntity.AddComponent<CameraFollowComponent>(srcReg.get<CameraFollowComponent>(entity));
	if (srcReg.any_of<SceneReferenceComponent>(entity))
		newEntity.AddComponent<SceneReferenceComponent>(srcReg.get<SceneReferenceComponent>(entity));

	SceneSerializer ser(&tempScene);
	std::string json = ser.SerializeToString();

	if (!WriteText(filepath, json))
	{
		QL_CORE_ERROR("PrefabSystem: Failed to write: {0}", filepath);
		return false;
	}

	QL_CORE_INFO("PrefabSystem: Created prefab '{0}' at {1}", GetBaseName(filepath), filepath);
	return true;
}

entt::entity PrefabSystem::InstantiatePrefab(Scene* scene, const std::string& filepath, const glm::vec3& position)
{
	auto prefab = LoadPrefab(filepath);
	if (!prefab || !prefab->IsValid())
		return entt::null;

	return InstantiateFromJson(scene, prefab->GetRawJson(), position);
}

entt::entity PrefabSystem::InstantiateFromJson(Scene* scene, const std::string& json, const glm::vec3& position)
{
	Scene tempScene;
	SceneSerializer ser(&tempScene);

	if (!ser.DeserializeFromString(json))
	{
		QL_CORE_WARN("PrefabSystem: Failed to deserialize prefab JSON");
		return entt::null;
	}

	auto& srcReg = tempScene.GetRegistry();

	auto view = srcReg.view<TransformComponent>();
	if (view.empty())
		return entt::null;

	entt::entity srcRoot = entt::null;
	uint32_t minId = UINT32_MAX;
	for (auto e : view)
	{
		if ((uint32_t)e < minId)
		{
			minId = (uint32_t)e;
			srcRoot = e;
		}
	}

	if (srcRoot == entt::null)
		return entt::null;

	auto dstRoot = scene->CreateEntity("PrefabInstance");
	auto& dstReg = scene->GetRegistry();

	if (srcReg.any_of<SpriteRendererComponent>(srcRoot))
		dstRoot.AddComponent<SpriteRendererComponent>(srcReg.get<SpriteRendererComponent>(srcRoot));
	if (srcReg.any_of<LuaScriptComponent>(srcRoot))
		dstRoot.AddComponent<LuaScriptComponent>(srcReg.get<LuaScriptComponent>(srcRoot));
	if (srcReg.any_of<Rigidbody2DComponent>(srcRoot))
		dstRoot.AddComponent<Rigidbody2DComponent>(srcReg.get<Rigidbody2DComponent>(srcRoot));
	if (srcReg.any_of<Rigidbody3DComponent>(srcRoot))
		dstRoot.AddComponent<Rigidbody3DComponent>(srcReg.get<Rigidbody3DComponent>(srcRoot));
	if (srcReg.any_of<BoxCollider2DComponent>(srcRoot))
		dstRoot.AddComponent<BoxCollider2DComponent>(srcReg.get<BoxCollider2DComponent>(srcRoot));
	if (srcReg.any_of<CircleCollider2DComponent>(srcRoot))
		dstRoot.AddComponent<CircleCollider2DComponent>(srcReg.get<CircleCollider2DComponent>(srcRoot));
	if (srcReg.any_of<TriangleCollider2DComponent>(srcRoot))
		dstRoot.AddComponent<TriangleCollider2DComponent>(srcReg.get<TriangleCollider2DComponent>(srcRoot));
	if (srcReg.any_of<BoxCollider3DComponent>(srcRoot))
		dstRoot.AddComponent<BoxCollider3DComponent>(srcReg.get<BoxCollider3DComponent>(srcRoot));
	if (srcReg.any_of<ParticleSystem2DComponent>(srcRoot))
		dstRoot.AddComponent<ParticleSystem2DComponent>(srcReg.get<ParticleSystem2DComponent>(srcRoot));
	if (srcReg.any_of<NavAgentComponent>(srcRoot))
		dstRoot.AddComponent<NavAgentComponent>(srcReg.get<NavAgentComponent>(srcRoot));
	if (srcReg.any_of<SpriteAnimationComponent>(srcRoot))
		dstRoot.AddComponent<SpriteAnimationComponent>(srcReg.get<SpriteAnimationComponent>(srcRoot));
	if (srcReg.any_of<UIEntityComponent>(srcRoot))
		dstRoot.AddComponent<UIEntityComponent>(srcReg.get<UIEntityComponent>(srcRoot));
	if (srcReg.any_of<AudioSourceComponent>(srcRoot))
		dstRoot.AddComponent<AudioSourceComponent>(srcReg.get<AudioSourceComponent>(srcRoot));
	if (srcReg.any_of<AudioListenerComponent>(srcRoot))
		dstRoot.AddComponent<AudioListenerComponent>(srcReg.get<AudioListenerComponent>(srcRoot));
	if (srcReg.any_of<CameraFollowComponent>(srcRoot))
		dstRoot.AddComponent<CameraFollowComponent>(srcReg.get<CameraFollowComponent>(srcRoot));
	if (srcReg.any_of<SceneReferenceComponent>(srcRoot))
		dstRoot.AddComponent<SceneReferenceComponent>(srcReg.get<SceneReferenceComponent>(srcRoot));

	if (position != glm::vec3(0, 0, 0))
	{
		if (dstRoot.HasComponent<TransformComponent>())
		{
			auto& tc = dstRoot.GetComponent<TransformComponent>();
			tc.Transform[3] = glm::vec4(position, 1.0f);
		}
	}

	if (dstRoot.HasComponent<PrefabReferenceComponent>())
	{
		auto& pr = dstRoot.GetComponent<PrefabReferenceComponent>();
		pr.PrefabPath = "";
		pr.PrefabName = "Instance";
		pr.IsValid = true;
	}
	else
	{
		auto& pr = dstRoot.AddComponent<PrefabReferenceComponent>();
		pr.PrefabPath = "";
		pr.PrefabName = "Instance";
		pr.IsValid = true;
	}

	return dstRoot;
}

bool PrefabSystem::IsLoaded(const std::string& filepath) const
{
	return m_LoadedPrefabs.find(filepath) != m_LoadedPrefabs.end();
}

Ref<PrefabAsset> PrefabSystem::GetLoaded(const std::string& filepath)
{
	auto it = m_LoadedPrefabs.find(filepath);
	return (it != m_LoadedPrefabs.end()) ? it->second : nullptr;
}

void PrefabSystem::Unload(const std::string& filepath)
{
	m_LoadedPrefabs.erase(filepath);
}

void PrefabSystem::UnloadAll()
{
	m_LoadedPrefabs.clear();
}

}
