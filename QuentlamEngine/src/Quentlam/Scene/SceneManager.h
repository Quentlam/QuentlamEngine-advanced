#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Scene.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Quentlam
{

enum class ESceneLoadMode : uint8_t
{
	Single = 0,
	Additive = 1
};

struct SceneLoadRequest
{
	std::string ScenePath;
	std::string SceneName;
	ESceneLoadMode Mode = ESceneLoadMode::Single;
	bool ShowLoadingScreen = false;
	float LoadingScreenDuration = 0.5f;

	std::function<void(Ref<Scene>)> OnLoaded;
	std::function<void(float progress)> OnProgress;
	std::function<void(const std::string& error)> OnError;

	float Progress = 0.0f;
	bool IsAsync = false;
	bool IsComplete = false;
	bool IsCancelled = false;
	std::string Error;
};

class SceneManager
{
public:
	static SceneManager& Get();

	Scene* GetActiveScene() const { return m_ActiveScene.get(); }
	void SetActiveScene(Ref<Scene> scene);
	void SetActiveSceneByName(const std::string& name);

	bool SaveScene(const std::string& filepath);
	bool SaveScene(Scene* scene, const std::string& filepath);
	Ref<Scene> LoadScene(const std::string& filepath);
	Ref<Scene> LoadScene(const std::string& filepath, ESceneLoadMode mode);

	Ref<Scene> CreateScene(const std::string& name);
	void DestroyScene(const std::string& name);
	Scene* GetScene(const std::string& name) const;
	const std::unordered_map<std::string, Ref<Scene>>& GetAllScenes() const { return m_Scenes; }

	Ref<Scene> InstantiateScene(const std::string& filepath);
	Ref<Scene> InstantiateScene(Scene* sourceScene);

	void LoadSceneAsync(const SceneLoadRequest& request);
	void CancelLoad(const std::string& scenePath);
	bool IsLoading() const { return !m_LoadQueue.empty(); }
	float GetLoadingProgress() const;
	const std::string& GetLoadingSceneName() const;

	void UnloadScene(const std::string& name);
	void UnloadAllScenes();

	Scene* GetEntityScene(entt::entity entity) const;
	entt::entity DuplicateEntity(entt::entity source, Scene* targetScene);

	std::vector<std::string> GetLoadedSceneNames() const;

private:
	std::unordered_map<std::string, Ref<Scene>> m_Scenes;
	Ref<Scene> m_ActiveScene;
	std::vector<SceneLoadRequest> m_LoadQueue;
	std::vector<SceneLoadRequest> m_CompletedLoads;
};

}
