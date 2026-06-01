#include "qlpch.h"
#include "InputAction.h"
#include "Quentlam/Core/Input.h"
#include <algorithm>

namespace Quentlam
{

void InputAction::AddBinding(const InputBinding& binding)
{
	m_Bindings.push_back(binding);
}

void InputAction::RemoveBinding(const InputBinding& binding)
{
	m_Bindings.erase(
		std::remove_if(m_Bindings.begin(), m_Bindings.end(),
			[&binding](const InputBinding& b) {
				return b.Type == binding.Type && b.KeyCode == binding.KeyCode;
			}),
		m_Bindings.end());
}

void InputAction::Update()
{
	EInputActionPhase oldPhase = m_Phase;

	bool anyPressed = false;
	bool anyHeld = false;
	float maxValue = 0.0f;

	for (const auto& binding : m_Bindings)
	{
		switch (binding.Type)
		{
		case InputBinding::EType::Key:
			if (Input::IsKeyPressed(binding.KeyCode))
				anyPressed = true;
			anyHeld = anyPressed;
			break;
		case InputBinding::EType::MouseButton:
			if (Input::IsMouseButtonPressed(binding.MouseButton))
				anyPressed = true;
			anyHeld = anyPressed;
			break;
		case InputBinding::EType::MouseAxis:
		{
			auto [mx, my] = Input::GetMousePosition();
			if (binding.MouseAxisX != 0)
				maxValue = std::max(maxValue, std::abs(mx));
			if (binding.MouseAxisY != 0)
				maxValue = std::max(maxValue, std::abs(my));
			anyPressed = anyPressed || maxValue > 0.0f;
			anyHeld = true;
			break;
		}
		}
	}

	EInputActionPhase newPhase = EInputActionPhase::Waiting;
	if (anyPressed && oldPhase == EInputActionPhase::Waiting)
		newPhase = EInputActionPhase::Started;
	else if (anyHeld)
		newPhase = EInputActionPhase::Performed;
	else if (oldPhase != EInputActionPhase::Waiting)
		newPhase = EInputActionPhase::Canceled;

	m_Phase = newPhase;
	m_Value = maxValue;

	if (newPhase != oldPhase && m_Callback)
		m_Callback(newPhase, m_Value);
}

void InputAction::Reset()
{
	m_Phase = EInputActionPhase::Waiting;
	m_Value = 0.0f;
}

float InputAction::GetFloatValue() const
{
	return std::get<float>(m_Value);
}

bool InputAction::GetBoolValue() const
{
	return std::get<bool>(m_Value);
}

glm::vec2 InputAction::GetVector2Value() const
{
	if (m_Value.index() == 3)
		return std::get<glm::vec2>(m_Value);
	return glm::vec2(0.0f);
}

InputAction* InputActionMap::CreateAction(const std::string& actionName)
{
	if (HasAction(actionName))
		return GetAction(actionName);
	auto action = CreateRef<InputAction>(actionName);
	m_Actions[actionName] = action;
	return action.get();
}

InputAction* InputActionMap::GetAction(const std::string& actionName)
{
	auto it = m_Actions.find(actionName);
	if (it != m_Actions.end())
		return it->second.get();
	return nullptr;
}

bool InputActionMap::HasAction(const std::string& actionName) const
{
	return m_Actions.find(actionName) != m_Actions.end();
}

void InputActionMap::RemoveAction(const std::string& actionName)
{
	m_Actions.erase(actionName);
}

void InputActionMap::Update()
{
	if (!m_Enabled)
		return;
	for (auto& [name, action] : m_Actions)
	{
		if (action)
			action->Update();
	}
}

InputSystem& InputSystem::Get()
{
	static InputSystem instance;
	return instance;
}

void InputSystem::Initialize()
{
}

void InputSystem::Shutdown()
{
	m_ActionMaps.clear();
}

void InputSystem::Update()
{
	if (!m_Enabled)
		return;
	for (const auto& mapName : m_EnabledMaps)
	{
		auto it = m_ActionMaps.find(mapName);
		if (it != m_ActionMaps.end() && it->second)
			it->second->Update();
	}
}

bool InputSystem::IsKeyPressed(int32_t keycode)
{
	return Input::IsKeyPressed(keycode);
}

bool InputSystem::IsKeyHeld(int32_t keycode)
{
	return Input::IsKeyPressed(keycode);
}

bool InputSystem::IsMouseButtonPressed(int32_t button)
{
	return Input::IsMouseButtonPressed(button);
}

float InputSystem::GetMouseX() const
{
	return Input::GetMouseX();
}

float InputSystem::GetMouseY() const
{
	return Input::GetMouseY();
}

glm::vec2 InputSystem::GetMousePosition() const
{
	auto [x, y] = Input::GetMousePosition();
	return { x, y };
}

Ref<InputActionMap> InputSystem::CreateActionMap(const std::string& name)
{
	if (m_ActionMaps.find(name) != m_ActionMaps.end())
		return m_ActionMaps[name];
	auto map = CreateRef<InputActionMap>(name);
	m_ActionMaps[name] = map;
	return map;
}

Ref<InputActionMap> InputSystem::GetActionMap(const std::string& name)
{
	auto it = m_ActionMaps.find(name);
	if (it != m_ActionMaps.end())
		return it->second;
	return nullptr;
}

void InputSystem::RemoveActionMap(const std::string& name)
{
	m_EnabledMaps.erase(name);
	m_ActionMaps.erase(name);
}

void InputSystem::EnableActionMap(const std::string& name)
{
	m_EnabledMaps.insert(name);
	auto it = m_ActionMaps.find(name);
	if (it != m_ActionMaps.end() && it->second)
		it->second->Enable();
}

void InputSystem::DisableActionMap(const std::string& name)
{
	m_EnabledMaps.erase(name);
	auto it = m_ActionMaps.find(name);
	if (it != m_ActionMaps.end() && it->second)
		it->second->Disable();
}

void InputSystem::Enable()
{
	m_Enabled = true;
}

void InputSystem::Disable()
{
	m_Enabled = false;
}

InputAction* InputSystem::GetAction(const std::string& mapName, const std::string& actionName)
{
	auto it = m_ActionMaps.find(mapName);
	if (it == m_ActionMaps.end() || !it->second)
		return nullptr;
	return it->second->GetAction(actionName);
}

bool InputSystem::SerializeToFile(const std::string& filepath) const
{
	return false;
}

bool InputSystem::DeserializeFromFile(const std::string& filepath)
{
	return false;
}

}
