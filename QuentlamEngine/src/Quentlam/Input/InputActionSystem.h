#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Core/KeyCodes.h"
#include "Quentlam/Core/MouseButtonCodes.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace Quentlam
{
	enum class EInputActionPhase : uint8_t
	{
		None = 0,
		Pressed = 1,
		Held = 2,
		Released = 3
	};

	enum class EInputModifier : uint8_t
	{
		None = 0,
		Shift = BIT(0),
		Control = BIT(1),
		Alt = BIT(2),
		Super = BIT(3)
	};

	enum class EGamepadAxis : uint8_t
	{
		LeftX = 0,
		LeftY = 1,
		RightX = 2,
		RightY = 3,
		LeftTrigger = 4,
		RightTrigger = 5
	};

	enum class EGamepadButton : uint16_t
	{
		A = 0,
		B = 1,
		X = 2,
		Y = 3,
		LeftBumper = 4,
		RightBumper = 5,
		LeftStick = 6,
		RightStick = 7,
		Start = 8,
		Select = 9,
		DPadUp = 10,
		DPadDown = 11,
		DPadLeft = 12,
		DPadRight = 13
	};

	struct KeyBinding
	{
		KeyCode Key = Key::UNKNOWN;
		EInputModifier Modifiers = EInputModifier::None;
		bool IsNegative = false;
	};

	struct MouseBinding
	{
		MouseCode Button = Mouse::Button0;
		bool IsNegative = false;
	};

	struct GamepadBinding
	{
		EGamepadButton Button = EGamepadButton::A;
		bool IsAxis = false;
		EGamepadAxis Axis = EGamepadAxis::LeftX;
		float AxisDeadzone = 0.2f;
		float AxisScale = 1.0f;
		bool IsNegative = false;
	};

	struct QUENTLAM_API InputBinding
	{
		std::string ActionName;
		std::vector<KeyBinding> Keys;
		std::vector<MouseBinding> MouseButtons;
		std::vector<GamepadBinding> GamepadButtons;

		bool IsEmpty() const { return Keys.empty() && MouseButtons.empty() && GamepadButtons.empty(); }
	};

	class QUENTLAM_API InputAction
	{
	public:
		InputAction() = default;
		InputAction(const std::string& name);

		const std::string& GetName() const { return m_Name; }

		EInputActionPhase GetPhase() const { return m_Phase; }
		float GetValue() const { return m_Value; }
		float GetPreviousValue() const { return m_PreviousValue; }

		bool IsPressed() const { return m_Phase == EInputActionPhase::Pressed; }
		bool IsHeld() const { return m_Phase == EInputActionPhase::Held || m_Phase == EInputActionPhase::Pressed; }
		bool IsReleased() const { return m_Phase == EInputActionPhase::Released; }
		bool IsActive() const { return m_Value != 0.0f; }

		float GetDelta() const { return m_Value - m_PreviousValue; }

		void SetPhase(EInputActionPhase phase) { m_Phase = phase; }
		void SetValue(float value) { m_PreviousValue = m_Value; m_Value = value; }

		const InputBinding& GetBinding() const { return m_Binding; }
		InputBinding& GetBinding() { return m_Binding; }

		void AddKeyBinding(KeyCode key, EInputModifier modifiers = EInputModifier::None);
		void AddMouseBinding(MouseCode button);
		void AddGamepadButtonBinding(EGamepadButton button);
		void AddGamepadAxisBinding(EGamepadAxis axis, float deadzone = 0.2f, float scale = 1.0f);

	private:
		std::string m_Name;
		EInputActionPhase m_Phase = EInputActionPhase::None;
		float m_Value = 0.0f;
		float m_PreviousValue = 0.0f;
		InputBinding m_Binding;
	};

	class QUENTLAM_API InputActionSystem
	{
	public:
		InputActionSystem() = default;
		static InputActionSystem& Get();

		void Initialize();
		void Shutdown();

		void OnKeyPressed(KeyCode key);
		void OnKeyReleased(KeyCode key);
		void OnMousePressed(MouseCode button);
		void OnMouseReleased(MouseCode button);
		void OnMouseMoved(float x, float y);
		void OnGamepadButtonPressed(EGamepadButton button);
		void OnGamepadButtonReleased(EGamepadButton button);
		void OnGamepadAxisMoved(EGamepadAxis axis, float value);

		InputAction* GetAction(const std::string& name);
		const InputAction* GetAction(const std::string& name) const;

		InputAction& CreateAction(const std::string& name);
		bool DestroyAction(const std::string& name);

		void Update(float deltaTime);

		void ClearKeyState();
		void ClearAllState();

		bool IsKeyPressed(KeyCode key) const;
		bool IsMouseButtonPressed(MouseCode button) const;

		float GetMouseDeltaX() const { return m_MouseDeltaX; }
		float GetMouseDeltaY() const { return m_MouseDeltaY; }
		float GetMouseWheelDelta() const { return m_MouseWheelDelta; }
		void ResetMouseDelta() { m_MouseDeltaX = 0.0f; m_MouseDeltaY = 0.0f; m_MouseWheelDelta = 0.0f; }

		void SetContext(const std::string& context);
		const std::string& GetContext() const { return m_CurrentContext; }
		void PushContext(const std::string& context);
		void PopContext();

		std::pair<float, float> GetMousePosition() const;

		bool IsModifierActive(EInputModifier modifier) const;
		bool IsShiftHeld() const { return IsModifierActive(EInputModifier::Shift); }
		bool IsCtrlHeld() const { return IsModifierActive(EInputModifier::Control); }
		bool IsAltHeld() const { return IsModifierActive(EInputModifier::Alt); }

		float GetGamepadAxis(EGamepadAxis axis) const;
		bool IsGamepadButtonPressed(EGamepadButton button) const;

		std::function<void(const std::string& actionName, EInputActionPhase phase)> OnActionPhaseChanged;

	private:
		float EvaluateBinding(const InputBinding& binding);
		float EvaluateKeyBinding(const KeyBinding& kb);
		float EvaluateMouseBinding(const MouseBinding& mb);
		float EvaluateGamepadBinding(const GamepadBinding& gb);

		std::string m_CurrentContext = "Default";
		std::vector<std::string> m_ContextStack;

		std::unordered_map<std::string, InputAction> m_Actions;

		std::unordered_set<KeyCode> m_KeysPressed;
		std::unordered_set<KeyCode> m_KeysJustPressed;
		std::unordered_set<KeyCode> m_KeysJustReleased;

		std::unordered_set<MouseCode> m_MousePressed;
		std::unordered_set<MouseCode> m_MouseJustPressed;
		std::unordered_set<MouseCode> m_MouseJustReleased;

		float m_MouseX = 0.0f;
		float m_MouseY = 0.0f;
		float m_MouseDeltaX = 0.0f;
		float m_MouseDeltaY = 0.0f;
		float m_MouseWheelDelta = 0.0f;

		std::unordered_map<EGamepadAxis, float> m_GamepadAxes;
		std::unordered_set<EGamepadButton> m_GamepadPressed;
		std::unordered_set<EGamepadButton> m_GamepadJustPressed;
		std::unordered_set<EGamepadButton> m_GamepadJustReleased;
	};

	inline InputAction::InputAction(const std::string& name)
		: m_Name(name) {}

	inline void InputAction::AddKeyBinding(KeyCode key, EInputModifier modifiers)
	{
		m_Binding.Keys.push_back({ key, modifiers, false });
	}

	inline void InputAction::AddMouseBinding(MouseCode button)
	{
		m_Binding.MouseButtons.push_back({ button, false });
	}

	inline void InputAction::AddGamepadButtonBinding(EGamepadButton button)
	{
		m_Binding.GamepadButtons.push_back({ button, false, EGamepadAxis::LeftX, 0.2f, 1.0f, false });
	}

	inline void InputAction::AddGamepadAxisBinding(EGamepadAxis axis, float deadzone, float scale)
	{
		m_Binding.GamepadButtons.push_back({ EGamepadButton::A, true, axis, deadzone, scale, false });
	}

	inline InputActionSystem& InputActionSystem::Get()
	{
		static InputActionSystem instance;
		return instance;
	}

	inline void InputActionSystem::Initialize()
	{
		m_Actions.clear();
		m_KeysPressed.clear();
		m_KeysJustPressed.clear();
		m_KeysJustReleased.clear();
		m_MousePressed.clear();
		m_MouseJustPressed.clear();
		m_MouseJustReleased.clear();
	}

	inline void InputActionSystem::Shutdown()
	{
		m_Actions.clear();
	}

	inline void InputActionSystem::OnKeyPressed(KeyCode key)
	{
		if (m_KeysPressed.find(key) == m_KeysPressed.end())
			m_KeysJustPressed.insert(key);
		m_KeysPressed.insert(key);
	}

	inline void InputActionSystem::OnKeyReleased(KeyCode key)
	{
		m_KeysPressed.erase(key);
		m_KeysJustReleased.insert(key);
	}

	inline void InputActionSystem::OnMousePressed(MouseCode button)
	{
		if (m_MousePressed.find(button) == m_MousePressed.end())
			m_MouseJustPressed.insert(button);
		m_MousePressed.insert(button);
	}

	inline void InputActionSystem::OnMouseReleased(MouseCode button)
	{
		m_MousePressed.erase(button);
		m_MouseJustReleased.insert(button);
	}

	inline void InputActionSystem::OnMouseMoved(float x, float y)
	{
		m_MouseDeltaX = x - m_MouseX;
		m_MouseDeltaY = y - m_MouseY;
		m_MouseX = x;
		m_MouseY = y;
	}

	inline void InputActionSystem::OnGamepadButtonPressed(EGamepadButton button)
	{
		if (m_GamepadPressed.find(button) == m_GamepadPressed.end())
			m_GamepadJustPressed.insert(button);
		m_GamepadPressed.insert(button);
	}

	inline void InputActionSystem::OnGamepadButtonReleased(EGamepadButton button)
	{
		m_GamepadPressed.erase(button);
		m_GamepadJustReleased.insert(button);
	}

	inline void InputActionSystem::OnGamepadAxisMoved(EGamepadAxis axis, float value)
	{
		m_GamepadAxes[axis] = value;
	}

	inline InputAction* InputActionSystem::GetAction(const std::string& name)
	{
		return const_cast<InputAction*>(static_cast<const InputActionSystem*>(this)->GetAction(name));
	}

	inline const InputAction* InputActionSystem::GetAction(const std::string& name) const
	{
		auto it = m_Actions.find(name);
		return it != m_Actions.end() ? &it->second : nullptr;
	}

	inline InputAction& InputActionSystem::CreateAction(const std::string& name)
	{
		return m_Actions[name];
	}

	inline bool InputActionSystem::DestroyAction(const std::string& name)
	{
		return m_Actions.erase(name) > 0;
	}

	inline void InputActionSystem::Update(float)
	{
		for (auto& [name, action] : m_Actions)
		{
			float newValue = EvaluateBinding(action.GetBinding());
			EInputActionPhase oldPhase = action.GetPhase();

			EInputActionPhase newPhase = EInputActionPhase::None;
			if (newValue > 0.0f)
			{
				if (oldPhase == EInputActionPhase::None || oldPhase == EInputActionPhase::Released)
					newPhase = EInputActionPhase::Pressed;
				else
					newPhase = EInputActionPhase::Held;
			}
			else
			{
				if (oldPhase == EInputActionPhase::Held || oldPhase == EInputActionPhase::Pressed)
					newPhase = EInputActionPhase::Released;
			}

			action.SetValue(newValue);
			if (oldPhase != newPhase)
			{
				action.SetPhase(newPhase);
				if (OnActionPhaseChanged)
					OnActionPhaseChanged(name, newPhase);
			}
		}

		m_KeysJustPressed.clear();
		m_KeysJustReleased.clear();
		m_MouseJustPressed.clear();
		m_MouseJustReleased.clear();
		m_GamepadJustPressed.clear();
		m_GamepadJustReleased.clear();
	}

	inline void InputActionSystem::ClearKeyState()
	{
		m_KeysPressed.clear();
		m_KeysJustPressed.clear();
		m_KeysJustReleased.clear();
	}

	inline void InputActionSystem::ClearAllState()
	{
		ClearKeyState();
		m_MousePressed.clear();
		m_MouseJustPressed.clear();
		m_MouseJustReleased.clear();
		m_GamepadPressed.clear();
		m_GamepadAxes.clear();
		m_MouseDeltaX = 0.0f;
		m_MouseDeltaY = 0.0f;
		m_MouseWheelDelta = 0.0f;
	}

	inline bool InputActionSystem::IsKeyPressed(KeyCode key) const
	{
		return m_KeysPressed.find(key) != m_KeysPressed.end();
	}

	inline bool InputActionSystem::IsMouseButtonPressed(MouseCode button) const
	{
		return m_MousePressed.find(button) != m_MousePressed.end();
	}

	inline void InputActionSystem::SetContext(const std::string& context)
	{
		m_CurrentContext = context;
	}

	inline void InputActionSystem::PushContext(const std::string& context)
	{
		m_ContextStack.push_back(m_CurrentContext);
		m_CurrentContext = context;
	}

	inline void InputActionSystem::PopContext()
	{
		if (!m_ContextStack.empty())
		{
			m_CurrentContext = m_ContextStack.back();
			m_ContextStack.pop_back();
		}
	}

	inline std::pair<float, float> InputActionSystem::GetMousePosition() const
	{
		return { m_MouseX, m_MouseY };
	}

	inline bool InputActionSystem::IsModifierActive(EInputModifier modifier) const
	{
		switch (modifier)
		{
		case EInputModifier::Shift:
			return IsKeyPressed(Key::LeftShift) || IsKeyPressed(Key::RightShift);
		case EInputModifier::Control:
			return IsKeyPressed(Key::LeftControl) || IsKeyPressed(Key::RightControl);
		case EInputModifier::Alt:
			return IsKeyPressed(Key::LeftAlt) || IsKeyPressed(Key::RightAlt);
		case EInputModifier::Super:
			return IsKeyPressed(Key::LeftSuper) || IsKeyPressed(Key::RightSuper);
		default:
			return false;
		}
	}

	inline float InputActionSystem::GetGamepadAxis(EGamepadAxis axis) const
	{
		auto it = m_GamepadAxes.find(axis);
		return it != m_GamepadAxes.end() ? it->second : 0.0f;
	}

	inline bool InputActionSystem::IsGamepadButtonPressed(EGamepadButton button) const
	{
		return m_GamepadPressed.find(button) != m_GamepadPressed.end();
	}

	inline float InputActionSystem::EvaluateBinding(const InputBinding& binding)
	{
		float maxValue = 0.0f;

		for (const auto& kb : binding.Keys)
		{
			float v = EvaluateKeyBinding(kb);
			maxValue = std::max(maxValue, v);
		}

		for (const auto& mb : binding.MouseButtons)
		{
			float v = EvaluateMouseBinding(mb);
			maxValue = std::max(maxValue, v);
		}

		for (const auto& gb : binding.GamepadButtons)
		{
			float v = EvaluateGamepadBinding(gb);
			maxValue = std::max(maxValue, v);
		}

		return maxValue;
	}

	inline float InputActionSystem::EvaluateKeyBinding(const KeyBinding& kb)
	{
		if (kb.Modifiers != EInputModifier::None && !IsModifierActive(kb.Modifiers))
			return 0.0f;
		float pressed = IsKeyPressed(kb.Key) ? 1.0f : 0.0f;
		return kb.IsNegative ? -pressed : pressed;
	}

	inline float InputActionSystem::EvaluateMouseBinding(const MouseBinding& mb)
	{
		float pressed = IsMouseButtonPressed(mb.Button) ? 1.0f : 0.0f;
		return mb.IsNegative ? -pressed : pressed;
	}

	inline float InputActionSystem::EvaluateGamepadBinding(const GamepadBinding& gb)
	{
		if (gb.IsAxis)
		{
			float axisValue = GetGamepadAxis(gb.Axis);
			if (std::abs(axisValue) < gb.AxisDeadzone)
				return 0.0f;
			float adjusted = (axisValue - std::copysign(gb.AxisDeadzone, axisValue)) / (1.0f - gb.AxisDeadzone);
			adjusted *= gb.AxisScale;
			return gb.IsNegative ? -adjusted : adjusted;
		}
		float pressed = IsGamepadButtonPressed(gb.Button) ? 1.0f : 0.0f;
		return gb.IsNegative ? -pressed : pressed;
	}
}
