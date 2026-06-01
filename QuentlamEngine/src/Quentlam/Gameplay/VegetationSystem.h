#pragma once
#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include "Quentlam/Renderer/Texture.h"
#include <vector>
#include <unordered_map>

namespace Quentlam
{

struct VegetationInstance
{
	glm::vec3 Position;
	glm::vec3 Rotation;
	glm::vec3 Scale;
	uint32_t ColorIndex;
};

struct VegetationType
{
	std::string Name;
	Ref<class Mesh> Mesh;
	Ref<Texture2D> BillboardTexture;
	float Density = 1.0f;
	float Radius = 1.0f;
	float WindInfluence = 0.5f;
	bool UseGPUInstancing = true;
	std::vector<VegetationInstance> Instances;
};

class VegetationSystem
{
public:
	static VegetationSystem& Get();

	void Initialize();
	void Shutdown();

	VegetationType* CreateVegetationType(const std::string& name);
	VegetationType* GetVegetationType(const std::string& name);
	void RemoveVegetationType(const std::string& name);

	void AddInstance(const std::string& typeName, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
	void RemoveInstancesInRadius(const glm::vec3& center, float radius);

	void Update(float deltaTime);
	void Render(class Scene* scene);

	int32_t GetTotalInstanceCount() const;

private:
	VegetationSystem() = default;
	~VegetationSystem() = default;

	std::unordered_map<std::string, Ref<VegetationType>> m_Types;
};

}
