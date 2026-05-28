#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Scene.h"
#include <string>
#include <unordered_map>

namespace Quentlam
{
	class SceneManager
	{
	public:
		static SceneManager& Get();

		Scene* GetActiveScene() const { return m_ActiveScene.get(); }
		void SetActiveScene(Ref<Scene> scene);

		bool SaveScene(const std::string& filepath);
		Ref<Scene> LoadScene(const std::string& filepath);

		Ref<Scene> CreateScene(const std::string& name);
		void DestroyScene(const std::string& name);
		Scene* GetScene(const std::string& name) const;
		const std::unordered_map<std::string, Ref<Scene>>& GetAllScenes() const { return m_Scenes; }

	private:
		SceneManager() = default;

		std::unordered_map<std::string, Ref<Scene>> m_Scenes;
		Ref<Scene> m_ActiveScene;
	};
}
