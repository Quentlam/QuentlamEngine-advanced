#include "qlpch.h"
#include "Quentlam/Gameplay/VegetationSystem.h"
#include "Quentlam/Scene/Scene.h"
#include "Quentlam/Renderer/Renderer3D.h"

namespace Quentlam
{

VegetationSystem& VegetationSystem::Get()
{
	static VegetationSystem instance;
	return instance;
}

void VegetationSystem::Initialize()
{
	QL_CORE_INFO("VegetationSystem initialized.");
}

void VegetationSystem::Shutdown()
{
	m_Types.clear();
	QL_CORE_INFO("VegetationSystem shut down.");
}

VegetationType* VegetationSystem::CreateVegetationType(const std::string& name)
{
	if (m_Types.find(name) != m_Types.end())
	{
		QL_CORE_WARN("Vegetation type '{}' already exists.", name);
		return nullptr;
	}
	auto type = CreateRef<VegetationType>();
	type->Name = name;
	m_Types[name] = type;
	QL_CORE_INFO("Created vegetation type: {}", name);
	return type.get();
}

VegetationType* VegetationSystem::GetVegetationType(const std::string& name)
{
	auto it = m_Types.find(name);
	if (it != m_Types.end())
		return it->second.get();
	return nullptr;
}

void VegetationSystem::RemoveVegetationType(const std::string& name)
{
	if (m_Types.erase(name) > 0)
		QL_CORE_INFO("Removed vegetation type: {}", name);
}

void VegetationSystem::AddInstance(const std::string& typeName, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
	VegetationType* type = GetVegetationType(typeName);
	if (!type)
	{
		QL_CORE_WARN("Cannot add instance to unknown vegetation type: {}", typeName);
		return;
	}
	VegetationInstance inst;
	inst.Position = position;
	inst.Rotation = rotation;
	inst.Scale = scale;
	type->Instances.push_back(inst);
}

void VegetationSystem::RemoveInstancesInRadius(const glm::vec3& center, float radius)
{
	float radiusSq = radius * radius;
	for (auto& [name, type] : m_Types)
	{
		auto& instances = type->Instances;
		instances.erase(
			std::remove_if(instances.begin(), instances.end(),
				[&](const VegetationInstance& inst) {
					return glm::dot(inst.Position - center, inst.Position - center) < radiusSq;
				}), instances.end());
	}
}

void VegetationSystem::Update(float deltaTime)
{
}

void VegetationSystem::Render(Scene* scene)
{
}

int32_t VegetationSystem::GetTotalInstanceCount() const
{
	int32_t total = 0;
	for (auto& [name, type] : m_Types)
		total += static_cast<int32_t>(type->Instances.size());
	return total;
}

}
