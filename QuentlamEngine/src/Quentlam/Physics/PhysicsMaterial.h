#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Quentlam
{

struct PhysicsMaterial2D
{
	std::string Name = "Default";
	float Density = 1.0f;
	float Friction = 0.5f;
	float Restitution = 0.0f;
	float RestitutionThreshold = 0.5f;
	uint32_t CollisionLayer = 1;
	uint32_t CollisionMask = 0xFFFFFFFF;
};

struct PhysicsMaterial3D
{
	std::string Name = "Default";
	float Density = 1.0f;
	float Friction = 0.5f;
	float Restitution = 0.0f;
	uint32_t CollisionLayer = 1;
	uint32_t CollisionMask = 0xFFFFFFFF;
};

class PhysicsMaterialLibrary
{
public:
	static PhysicsMaterialLibrary& Get();

	void Initialize();

	Ref<PhysicsMaterial2D> Create2D(const std::string& name);
	Ref<PhysicsMaterial2D> Get2D(const std::string& name);
	Ref<PhysicsMaterial2D> GetDefault2D();
	void Remove2D(const std::string& name);

	Ref<PhysicsMaterial3D> Create3D(const std::string& name);
	Ref<PhysicsMaterial3D> Get3D(const std::string& name);
	Ref<PhysicsMaterial3D> GetDefault3D();
	void Remove3D(const std::string& name);

	std::vector<Ref<PhysicsMaterial2D>> GetAll2D() const;
	std::vector<Ref<PhysicsMaterial3D>> GetAll3D() const;

	bool SerializeToFile(const std::string& filepath) const;
	bool DeserializeFromFile(const std::string& filepath);

private:
	PhysicsMaterialLibrary() = default;
	~PhysicsMaterialLibrary() = default;

	std::unordered_map<std::string, Ref<PhysicsMaterial2D>> m_Materials2D;
	std::unordered_map<std::string, Ref<PhysicsMaterial3D>> m_Materials3D;
	Ref<PhysicsMaterial2D> m_Default2D;
	Ref<PhysicsMaterial3D> m_Default3D;
};

struct TriggerComponent
{
	bool IsTrigger = false;
	std::string EnterLuaFunction;
	std::string ExitLuaFunction;
	std::string StayLuaFunction;
	int32_t CollisionLayer = 1;
};

}
