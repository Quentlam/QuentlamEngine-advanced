#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Quentlam
{

enum class EAnimatorControllerParameterType : uint8_t
{
	Float = 0,
	Int = 1,
	Bool = 2,
	Trigger = 3
};

struct AnimatorControllerParameter
{
	std::string Name;
	EAnimatorControllerParameterType Type = EAnimatorControllerParameterType::Float;
	float DefaultFloat = 0.0f;
	int32_t DefaultInt = 0;
	bool DefaultBool = false;

	bool operator==(const AnimatorControllerParameter& other) const
	{
		return Name == other.Name && Type == other.Type;
	}
};

struct AnimatorState
{
	std::string Name;
	std::string MotionPath;
	float Speed = 1.0f;
	float CycleOffset = 0.0f;
	bool WriteDefaults = true;
	std::string Tag;
	std::vector<std::string> Transitions;
	glm::vec2 Position = { 0.0f, 0.0f };
};

struct AnimatorTransition
{
	std::string Name;
	std::string SourceState;
	std::string DestinationState;
	float Duration = 0.0f;
	float ExitTime = 0.5f;
	bool HasExitTime = true;
	bool HasFixedDuration = true;
	float Offset = 0.0f;

	struct Condition
	{
		std::string Parameter;
		enum Mode { If, IfNot, Greater, Less, Equals, NotEquals } Mode = If;
		float Threshold = 0.0f;
	};
	std::vector<Condition> Conditions;
};

class QUENTLAM_API AnimatorController
{
public:
	AnimatorController() = default;
	explicit AnimatorController(const std::string& name) : m_Name(name) {}

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	AnimatorState* CreateState(const std::string& name);
	AnimatorState* GetState(const std::string& name);
	bool HasState(const std::string& name) const;
	void RemoveState(const std::string& name);
	const std::vector<AnimatorState>& GetStates() const { return m_States; }

	AnimatorTransition* CreateTransition(const std::string& from, const std::string& to);
	AnimatorTransition* GetTransition(const std::string& from, const std::string& to);
	void RemoveTransition(const std::string& from, const std::string& to);
	const std::vector<AnimatorTransition>& GetTransitions() const { return m_Transitions; }
	std::vector<AnimatorTransition*> GetTransitionsFrom(const std::string& stateName);

	void AddParameter(const AnimatorControllerParameter& param);
	void RemoveParameter(const std::string& name);
	AnimatorControllerParameter* GetParameter(const std::string& name);
	const std::vector<AnimatorControllerParameter>& GetParameters() const { return m_Parameters; }
	bool HasParameter(const std::string& name) const;

	const std::string& GetEntryState() const { return m_EntryState; }
	void SetEntryState(const std::string& name) { m_EntryState = name; }

	bool SerializeToFile(const std::string& filepath) const;
	bool DeserializeFromFile(const std::string& filepath);
	std::string SerializeToString() const;
	bool DeserializeFromString(const std::string& json);

private:
	std::string m_Name;
	std::vector<AnimatorState> m_States;
	std::vector<AnimatorTransition> m_Transitions;
	std::vector<AnimatorControllerParameter> m_Parameters;
	std::string m_EntryState;
};

}
