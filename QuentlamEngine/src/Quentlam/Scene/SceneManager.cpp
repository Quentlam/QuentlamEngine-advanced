#include "qlpch.h"
#include "Quentlam/Scene/SceneManager.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{
	SceneManager& SceneManager::Get()
	{
		static SceneManager instance;
		return instance;
	}

	void SceneManager::SetActiveScene(Ref<Scene> scene)
	{
		m_ActiveScene = scene;
		QL_CORE_INFO("Active scene set");
	}

	bool SceneManager::SaveScene(const std::string& filepath)
	{
		if (!m_ActiveScene)
		{
			QL_CORE_WARN("No active scene to save");
			return false;
		}

		QL_CORE_INFO("Saving scene to: {0}", filepath);
		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(filepath);
		return true;
	}

	Ref<Scene> SceneManager::LoadScene(const std::string& filepath)
	{
		QL_CORE_INFO("Loading scene from: {0}", filepath);
		Ref<Scene> scene = CreateRef<Scene>("LoadedScene");
		SceneSerializer serializer(scene);
		if (serializer.Deserialize(filepath))
		{
			SetActiveScene(scene);
			return scene;
		}
		QL_CORE_ERROR("Failed to load scene: {0}", filepath);
		return nullptr;
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
}
