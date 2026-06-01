#include "qlpch.h"
#include "SceneManager.h"
#include "SceneSerializer.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Scene/Entity.h"
#include "Quentlam/Scene/Components.h"

namespace Quentlam
{

SceneManager& SceneManager::Get()
{
	static SceneManager instance;
	return instance;
}

void SceneManager::SetActiveScene(Ref<Scene> scene)
{
	if (m_ActiveScene == scene)
		return;

	if (m_ActiveScene)
		m_ActiveScene->OnRuntimeStop();

	m_ActiveScene = scene;

	if (m_ActiveScene)
	{
		QL_CORE_INFO("Active scene set: {0}", scene->m_Name);
	}
}

void SceneManager::SetActiveSceneByName(const std::string& name)
{
	auto it = m_Scenes.find(name);
	if (it != m_Scenes.end())
		SetActiveScene(it->second);
	else
		QL_CORE_WARN("Scene not found: {0}", name);
}

bool SceneManager::SaveScene(const std::string& filepath)
{
	if (!m_ActiveScene)
	{
		QL_CORE_WARN("No active scene to save");
		return false;
	}
	return SaveScene(m_ActiveScene.get(), filepath);
}

bool SceneManager::SaveScene(Scene* scene, const std::string& filepath)
{
	if (!scene)
		return false;

	QL_CORE_INFO("Saving scene to: {0}", filepath);
	SceneSerializer serializer(scene);
	serializer.Serialize(filepath);
	return true;
}

Ref<Scene> SceneManager::LoadScene(const std::string& filepath)
{
	return LoadScene(filepath, ESceneLoadMode::Single);
}

Ref<Scene> SceneManager::LoadScene(const std::string& filepath, ESceneLoadMode mode)
{
	QL_CORE_INFO("Loading scene from: {0} (mode: {1})", filepath,
		mode == ESceneLoadMode::Single ? "Single" : "Additive");

	Ref<Scene> scene = CreateRef<Scene>("LoadedScene");
	SceneSerializer serializer(scene);

	if (!serializer.Deserialize(filepath))
	{
		QL_CORE_ERROR("Failed to load scene: {0}", filepath);
		return nullptr;
	}

	std::string sceneName = filepath;
	size_t lastSlash = sceneName.find_last_of("/\\");
	if (lastSlash != std::string::npos)
		sceneName = sceneName.substr(lastSlash + 1);
	size_t lastDot = sceneName.find_last_of('.');
	if (lastDot != std::string::npos)
		sceneName = sceneName.substr(0, lastDot);

	m_Scenes[sceneName] = scene;

	if (mode == ESceneLoadMode::Single)
	{
		for (auto& [name, s] : m_Scenes)
		{
			if (s != scene)
				s->OnRuntimeStop();
		}
		SetActiveScene(scene);
	}

	return scene;
}

Ref<Scene> SceneManager::CreateScene(const std::string& name)
{
	if (m_Scenes.find(name) != m_Scenes.end())
	{
		QL_CORE_WARN("Scene with name '{0}' already exists", name);
		return m_Scenes[name];
	}

	auto scene = CreateRef<Scene>(name);
	m_Scenes[name] = scene;
	QL_CORE_INFO("Created scene: {0}", name);
	return scene;
}

void SceneManager::DestroyScene(const std::string& name)
{
	auto it = m_Scenes.find(name);
	if (it != m_Scenes.end())
	{
		if (m_ActiveScene == it->second)
			m_ActiveScene = nullptr;
		m_Scenes.erase(it);
		QL_CORE_INFO("Destroyed scene: {0}", name);
	}
}

Scene* SceneManager::GetScene(const std::string& name) const
{
	auto it = m_Scenes.find(name);
	return it != m_Scenes.end() ? it->second.get() : nullptr;
}

Ref<Scene> SceneManager::InstantiateScene(const std::string& filepath)
{
	Ref<Scene> scene = CreateRef<Scene>("Instantiated");
	SceneSerializer serializer(scene);
	if (!serializer.Deserialize(filepath))
	{
		QL_CORE_ERROR("Failed to instantiate scene: {0}", filepath);
		return nullptr;
	}

	QL_CORE_INFO("Instantiated scene from: {0}", filepath);
	return scene;
}

Ref<Scene> SceneManager::InstantiateScene(Scene* sourceScene)
{
	if (!sourceScene)
		return nullptr;

	std::string newName = sourceScene->m_Name + "_Copy";
	Ref<Scene> newScene = CreateRef<Scene>(newName);

	auto sourceView = sourceScene->GetRegistry().view<TransformComponent>();
	for (auto sourceEntity : sourceView)
	{
		Entity newEntity = newScene->CreateEntity();
		auto& targetTransform = newScene->GetRegistry().get<TransformComponent>(newEntity);
		targetTransform = sourceView.get<TransformComponent>(sourceEntity);
	}

	QL_CORE_INFO("Instantiated scene from: {0}", sourceScene->m_Name);
	return newScene;
}

void SceneManager::LoadSceneAsync(const SceneLoadRequest& request)
{
	QL_CORE_INFO("Queueing async scene load: {0}", request.ScenePath);
	m_LoadQueue.push_back(request);
}

void SceneManager::CancelLoad(const std::string& scenePath)
{
	for (auto& req : m_LoadQueue)
	{
		if (req.ScenePath == scenePath)
		{
			req.IsCancelled = true;
			QL_CORE_INFO("Cancelled load: {0}", scenePath);
			return;
		}
	}
}

float SceneManager::GetLoadingProgress() const
{
	if (m_LoadQueue.empty())
		return 1.0f;
	return m_LoadQueue.front().Progress;
}

const std::string& SceneManager::GetLoadingSceneName() const
{
	static std::string empty;
	if (m_LoadQueue.empty())
		return empty;
	return m_LoadQueue.front().SceneName.empty() ? m_LoadQueue.front().ScenePath : m_LoadQueue.front().SceneName;
}

void SceneManager::UnloadScene(const std::string& name)
{
	DestroyScene(name);
	QL_CORE_INFO("Unloaded scene: {0}", name);
}

void SceneManager::UnloadAllScenes()
{
	while (!m_Scenes.empty())
	{
		auto it = m_Scenes.begin();
		DestroyScene(it->first);
	}
}

Scene* SceneManager::GetEntityScene(entt::entity entity) const
{
	for (const auto& [name, scene] : m_Scenes)
	{
		if (scene->GetRegistry().valid(entity))
			return scene.get();
	}
	return nullptr;
}

entt::entity SceneManager::DuplicateEntity(entt::entity source, Scene* targetScene)
{
	if (!targetScene)
		return entt::null;

	Entity newEntity = targetScene->CreateEntity();
	return newEntity;
}

std::vector<std::string> SceneManager::GetLoadedSceneNames() const
{
	std::vector<std::string> names;
	for (const auto& [name, _] : m_Scenes)
		names.push_back(name);
	return names;
}

}
