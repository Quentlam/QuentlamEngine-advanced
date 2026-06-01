#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <entt/entt.hpp>

namespace Quentlam
{

struct PrefabComponentData
{
	std::string ComponentType;
	std::string ComponentData;
};

struct PrefabEntityData
{
	std::string Name;
	std::vector<PrefabComponentData> Components;
	std::vector<PrefabEntityData> Children;
};

class PrefabAsset
{
public:
	PrefabAsset() = default;
	explicit PrefabAsset(const std::string& name) : m_Name(name) {}

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	const std::vector<PrefabEntityData>& GetEntities() const { return m_Entities; }
	void SetEntities(const std::vector<PrefabEntityData>& entities) { m_Entities = entities; }
	bool IsValid() const { return !m_RawJson.empty(); }

	const std::string& GetRawJson() const { return m_RawJson; }
	void SetRawJson(const std::string& json) { m_RawJson = json; }

private:
	std::string m_Name;
	std::vector<PrefabEntityData> m_Entities;
	std::string m_RawJson;
};

}
