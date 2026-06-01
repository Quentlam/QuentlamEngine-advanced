#pragma once
#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Quentlam
{

enum class EJointType
{
	Distance,
	Revolute,
	Prismatic,
	Weld,
	Friction,
	Motor,
	Wheel
};

class Joint
{
public:
	Joint();
	virtual ~Joint();

	virtual EJointType GetType() const = 0;

	entt::entity GetEntityA() const { return m_EntityA; }
	entt::entity GetEntityB() const { return m_EntityB; }
	void SetEntities(entt::entity a, entt::entity b);

	bool IsEnabled() const { return m_Enabled; }
	void SetEnabled(bool enabled) { m_Enabled = enabled; }

protected:
	entt::entity m_EntityA = entt::null;
	entt::entity m_EntityB = entt::null;
	bool m_Enabled = true;
};

class DistanceJoint : public Joint
{
public:
	DistanceJoint();
	EJointType GetType() const override { return EJointType::Distance; }

	float GetLength() const { return m_Length; }
	void SetLength(float length) { m_Length = length; }

	float GetMinLength() const { return m_MinLength; }
	void SetMinLength(float min) { m_MinLength = min; }

	float GetMaxLength() const { return m_MaxLength; }
	void SetMaxLength(float max) { m_MaxLength = max; }

	float GetStiffness() const { return m_Stiffness; }
	void SetStiffness(float s) { m_Stiffness = s; }

	float GetDamping() const { return m_Damping; }
	void SetDamping(float d) { m_Damping = d; }

private:
	float m_Length = 1.0f;
	float m_MinLength = 0.0f;
	float m_MaxLength = 1000.0f;
	float m_Stiffness = 0.0f;
	float m_Damping = 0.0f;
};

class RevoluteJoint : public Joint
{
public:
	RevoluteJoint();
	EJointType GetType() const override { return EJointType::Revolute; }

	float GetMotorSpeed() const { return m_MotorSpeed; }
	void SetMotorSpeed(float speed) { m_MotorSpeed = speed; }

	float GetMaxMotorTorque() const { return m_MaxMotorTorque; }
	void SetMaxMotorTorque(float torque) { m_MaxMotorTorque = torque; }

	bool IsMotorEnabled() const { return m_MotorEnabled; }
	void SetMotorEnabled(bool enabled) { m_MotorEnabled = enabled; }

	bool HasLimits() const { return m_UseLimits; }
	void SetLimitsEnabled(bool enabled) { m_UseLimits = enabled; }

	float GetLowerAngle() const { return m_LowerAngle; }
	void SetLowerAngle(float angle) { m_LowerAngle = angle; }

	float GetUpperAngle() const { return m_UpperAngle; }
	void SetUpperAngle(float angle) { m_UpperAngle = angle; }

private:
	float m_MotorSpeed = 0.0f;
	float m_MaxMotorTorque = 1000.0f;
	bool m_MotorEnabled = false;
	bool m_UseLimits = false;
	float m_LowerAngle = 0.0f;
	float m_UpperAngle = 0.0f;
};

class PhysicsJointSystem
{
public:
	static PhysicsJointSystem& Get();

	void Initialize();
	void Shutdown();
	void Update(class Scene* scene);

	Ref<Joint> CreateJoint(Ref<Joint> joint);
	void RemoveJoint(entt::entity entity);

	Ref<Joint> GetJoint(entt::entity entity);
	std::vector<Ref<Joint>> GetJoints() const { return m_Joints; }

private:
	PhysicsJointSystem() = default;
	~PhysicsJointSystem() = default;

	std::vector<Ref<Joint>> m_Joints;
	std::unordered_map<entt::entity, Ref<Joint>> m_EntityJoints;
};

}
