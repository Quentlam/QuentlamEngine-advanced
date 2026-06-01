#include "qlpch.h"
#include "Quentlam/Physics/PhysicsJoints.h"
#include "Quentlam/Physics/Physics2D.h"
#include "Quentlam/Scene/Scene.h"

namespace Quentlam
{

Joint::Joint()
{
}

Joint::~Joint()
{
}

void Joint::SetEntities(entt::entity a, entt::entity b)
{
	m_EntityA = a;
	m_EntityB = b;
}

DistanceJoint::DistanceJoint()
{
}

RevoluteJoint::RevoluteJoint()
{
}

PhysicsJointSystem& PhysicsJointSystem::Get()
{
	static PhysicsJointSystem instance;
	return instance;
}

void PhysicsJointSystem::Initialize()
{
}

void PhysicsJointSystem::Shutdown()
{
	m_Joints.clear();
	m_EntityJoints.clear();
}

void PhysicsJointSystem::Update(Scene* scene)
{
}

Ref<Joint> PhysicsJointSystem::CreateJoint(Ref<Joint> joint)
{
	m_Joints.push_back(joint);
	return joint;
}

void PhysicsJointSystem::RemoveJoint(entt::entity entity)
{
	if (auto it = m_EntityJoints.find(entity); it != m_EntityJoints.end())
	{
		auto joint = it->second;
		m_EntityJoints.erase(it);
		auto jt = std::find(m_Joints.begin(), m_Joints.end(), joint);
		if (jt != m_Joints.end())
			m_Joints.erase(jt);
	}
}

Ref<Joint> PhysicsJointSystem::GetJoint(entt::entity entity)
{
	if (auto it = m_EntityJoints.find(entity); it != m_EntityJoints.end())
		return it->second;
	return nullptr;
}

}
