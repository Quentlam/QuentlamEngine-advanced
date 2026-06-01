#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Core/KeyCodes.h"
#include "Quentlam/Core/MouseButtonCodes.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <variant>

namespace Quentlam
{

enum class EInputActionPhase : uint8_t
{
	Waiting = 0,
	Started = 1,
	Performed = 2,
	Canceled = 3
};

enum class EInputModifier : uint8_t
{
	None = 0,
	Shift = 1 << 0,
	Control = 1 << 1,
	Alt = 1 << 2,
	Command = 1 << 3
};

using InputValue = std::variant<float, bool, glm::vec2, glm::vec3>;

struct InputBinding
{
	enum class EType { None = 0, Key, MouseButton, MouseAxis, GamepadButton, GamepadAxis };

	EType Type = EType::None;

	int32_t KeyCode = 0;
	int32_t MouseButton = 0;

	int32_t MouseAxisX = 0;
	int32_t MouseAxisY = 0;

	EInputModifier Modifiers = EInputModifier::None;
	float Deadzone = 0.0f;
};

class InputAction
{
public:
	InputAction() = default;
	explicit InputAction(const std::string& name) : m_Name(name) {}

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	void AddBinding(const InputBinding& binding);
	void RemoveBinding(const InputBinding& binding);
	const std::vector<InputBinding>& GetBindings() const { return m_Bindings; }

	void Update();
	void Reset();

	EInputActionPhase GetPhase() const { return m_Phase; }

	float GetFloatValue() const;
	bool GetBoolValue() const;
	glm::vec2 GetVector2Value() const;

	bool IsPressed() const { return m_Phase == EInputActionPhase::Started || m_Phase == EInputActionPhase::Performed; }
	bool IsHeld() const { return m_Phase == EInputActionPhase::Performed; }
	bool IsCanceled() const { return m_Phase == EInputActionPhase::Canceled; }

	using ActionCallback = std::function<void(EInputActionPhase phase, const InputValue& value)>;
	void SetCallback(ActionCallback callback) { m_Callback = std::move(callback); }

private:
	std::string m_Name;
	std::vector<InputBinding> m_Bindings;
	EInputActionPhase m_Phase = EInputActionPhase::Waiting;
	InputValue m_Value = 0.0f;
	ActionCallback m_Callback;
};

class InputActionMap
{
public:
	InputActionMap() = default;
	explicit InputActionMap(const std::string& name) : m_Name(name) {}

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	InputAction* CreateAction(const std::string& actionName);
	InputAction* GetAction(const std::string& actionName);
	bool HasAction(const std::string& actionName) const;
	void RemoveAction(const std::string& actionName);

	const std::unordered_map<std::string, Ref<InputAction>>& GetActions() const { return m_Actions; }

	void Enable() { m_Enabled = true; }
	void Disable() { m_Enabled = false; }
	bool IsEnabled() const { return m_Enabled; }

	void Update();

private:
	std::string m_Name;
	std::unordered_map<std::string, Ref<InputAction>> m_Actions;
	bool m_Enabled = true;
};

class InputSystem
{
public:
	static InputSystem& Get();

	void Initialize();
	void Shutdown();

	void Update();

	bool IsKeyPressed(int32_t keycode);
	bool IsKeyHeld(int32_t keycode);
	bool IsMouseButtonPressed(int32_t button);

	float GetMouseX() const;
	float GetMouseY() const;
	glm::vec2 GetMousePosition() const;

	Ref<InputActionMap> CreateActionMap(const std::string& name);
	Ref<InputActionMap> GetActionMap(const std::string& name);
	void RemoveActionMap(const std::string& name);

	void EnableActionMap(const std::string& name);
	void DisableActionMap(const std::string& name);

	void Enable();
	void Disable();
	bool IsEnabled() const { return m_Enabled; }

	InputAction* GetAction(const std::string& mapName, const std::string& actionName);

	bool SerializeToFile(const std::string& filepath) const;
	bool DeserializeFromFile(const std::string& filepath);

private:
	InputSystem() = default;
	~InputSystem() = default;

	bool m_Enabled = true;
	std::unordered_map<std::string, Ref<InputActionMap>> m_ActionMaps;
	std::unordered_set<std::string> m_EnabledMaps;
};

}
