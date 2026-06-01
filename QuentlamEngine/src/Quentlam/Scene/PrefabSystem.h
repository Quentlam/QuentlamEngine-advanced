#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Quentlam
{

class Scene;
class PrefabAsset;
struct PrefabEntityData;

class PrefabSystem
{
public:
	static PrefabSystem& Get();

	Ref<PrefabAsset> LoadPrefab(const std::string& filepath);
	bool CreatePrefabFromEntity(entt::entity entity, Scene* scene, const std::string& filepath);
	entt::entity InstantiatePrefab(Scene* scene, const std::string& filepath, const glm::vec3& position = { 0, 0, 0 });

	bool IsLoaded(const std::string& filepath) const;
	Ref<PrefabAsset> GetLoaded(const std::string& filepath);
	void Unload(const std::string& filepath);
	void UnloadAll();

private:
	PrefabSystem() = default;
	~PrefabSystem() = default;

	std::unordered_map<std::string, Ref<PrefabAsset>> m_LoadedPrefabs;

	entt::entity InstantiateFromJson(Scene* scene, const std::string& json, const glm::vec3& position);
};

}
