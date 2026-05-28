#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <variant>

namespace Quentlam
{
	enum class ECursorState : uint8_t
	{
		Normal = 0,
		Pointer = 1,
		Grab = 2,
		Grabbing = 3,
		Crosshair = 4,
		Help = 5,
		Forbidden = 6,
		TextInput = 7,
		VerticalResize = 8,
		HorizontalResize = 9,
		DiagonalResize = 10,
		Custom = 100
	};

	enum class EInputContext : uint8_t
	{
		Gameplay = 0,
		Menu = 1,
		Dialog = 2,
		Inventory = 3,
		Dialogue = 4,
		Fishing = 5,
		Busy = 6,
		Blocked = 7
	};

	enum class EUISound : uint8_t
	{
		None = 0,
		Click = 1,
		Hover = 2,
		Open = 3,
		Close = 4,
		Select = 5,
		Confirm = 6,
		Cancel = 7,
		Error = 8,
		Achievement = 9,
		Custom = 100
	};

	class UIScreen;

	class WidgetTree
	{
	public:
		WidgetTree() = default;
		~WidgetTree() = default;

		void AddChild(Ref<UIScreen> child);
		void RemoveChild(Ref<UIScreen> child);
		void ClearChildren();

		const std::vector<Ref<UIScreen>>& GetChildren() const { return m_Children; }
		Ref<UIScreen> GetParent() const { return m_Parent; }

		bool IsVisible() const { return m_Visible; }
		void SetVisible(bool visible) { m_Visible = visible; }

		int32_t GetZOrder() const { return m_ZOrder; }
		void SetZOrder(int32_t z) { m_ZOrder = z; }

		bool IsModal() const { return m_Modal; }
		void SetModal(bool modal) { m_Modal = modal; }

		void Tick(float deltaTime);
		void Render();

	protected:
		Ref<UIScreen> m_Parent;
		std::vector<Ref<UIScreen>> m_Children;
		int32_t m_ZOrder = 0;
		bool m_Visible = true;
		bool m_Modal = false;
	};

	class QUENTLAM_API UIScreen : public std::enable_shared_from_this<UIScreen>
	{
	public:
		explicit UIScreen(const std::string& id);
		virtual ~UIScreen() = default;

		const std::string& GetId() const { return m_Id; }
		virtual void Show();
		virtual void Hide();
		virtual void Toggle();
		bool IsShowing() const { return m_IsShowing; }

		virtual void OnShow() {}
		virtual void OnHide() {}

		virtual void OnTick(float deltaTime) {}
		virtual void OnRender() {}

		void AddChild(Ref<UIScreen> child);
		void RemoveChild(Ref<UIScreen> child);

		WidgetTree& GetWidgetTree() { return m_WidgetTree; }
		const WidgetTree& GetWidgetTree() const { return m_WidgetTree; }

		void SetFocusable(bool focusable) { m_Focusable = focusable; }
		bool IsFocusable() const { return m_Focusable; }
		bool IsFocused() const { return m_IsFocused; }
		void SetFocused(bool focused);

		void SetInputContext(EInputContext context) { m_InputContext = context; }
		EInputContext GetInputContext() const { return m_InputContext; }

		void SetBlocking(bool blocking) { m_Blocking = blocking; }
		bool IsBlocking() const { return m_Blocking; }

		virtual bool OnInputBubble(bool handled) { return handled; }
		virtual bool OnKeyInput(int32_t key, bool pressed) { return false; }
		virtual bool OnMouseMove(float x, float y) { return false; }
		virtual bool OnMouseClick(float x, float y, int32_t button, bool pressed) { return false; }

		virtual ECursorState GetCursorState() const { return ECursorState::Normal; }

	protected:
		std::string m_Id;
		bool m_IsShowing = false;
		bool m_IsFocused = false;
		bool m_Focusable = false;
		bool m_Blocking = false;
		EInputContext m_InputContext = EInputContext::Menu;
		WidgetTree m_WidgetTree;
		Ref<UIScreen> m_Parent;
	};

	class ScreenStack
	{
	public:
		ScreenStack() = default;
		static ScreenStack& Get();

		void Push(Ref<UIScreen> screen);
		void Pop();
		void PopAll();
		void Replace(Ref<UIScreen> screen);
		void Insert(int32_t index, Ref<UIScreen> screen);
		void Remove(Ref<UIScreen> screen);

		Ref<UIScreen> Top() const;
		bool IsEmpty() const { return m_Screens.empty(); }
		size_t Size() const { return m_Screens.size(); }

		const std::vector<Ref<UIScreen>>& GetAll() const { return m_Screens; }
		bool Contains(Ref<UIScreen> screen) const;

		Ref<UIScreen> Find(const std::string& screenId) const;
		bool IsTop(Ref<UIScreen> screen) const;

		void Tick(float deltaTime);
		void Render();

		void SetMousePosition(float x, float y) { m_MouseX = x; m_MouseY = y; }
		float GetMouseX() const { return m_MouseX; }
		float GetMouseY() const { return m_MouseY; }

		void SetInputEnabled(bool enabled) { m_InputEnabled = enabled; }
		bool IsInputEnabled() const { return m_InputEnabled; }

	private:
		std::vector<Ref<UIScreen>> m_Screens;
		float m_MouseX = 0.0f;
		float m_MouseY = 0.0f;
		bool m_InputEnabled = true;
	};

	class FocusManager
	{
	public:
		FocusManager() = default;
		static FocusManager& Get();

		void SetFocus(Ref<UIScreen> screen);
		void ClearFocus();
		Ref<UIScreen> GetFocused() const { return m_FocusedScreen; }
		bool HasFocus(Ref<UIScreen> screen) const;
		bool HasAnyFocus() const { return m_FocusedScreen != nullptr; }

		void LockFocus(Ref<UIScreen> screen);
		void UnlockFocus(Ref<UIScreen> screen);
		bool IsLocked() const { return m_LockedScreen != nullptr; }
		Ref<UIScreen> GetLockedScreen() const { return m_LockedScreen; }

		void AddFocusable(Ref<UIScreen> screen);
		void RemoveFocusable(Ref<UIScreen> screen);
		bool NavigateNext(bool reverse = false);

		std::function<void(Ref<UIScreen>)> OnFocusChanged;

	private:
		Ref<UIScreen> m_FocusedScreen;
		Ref<UIScreen> m_LockedScreen;
		std::vector<Ref<UIScreen>> m_FocusableScreens;
	};

	class InputContextManager
	{
	public:
		InputContextManager() = default;
		static InputContextManager& Get();

		void PushContext(EInputContext context);
		void PopContext();
		void ClearContexts();
		void SetDefaultContext(EInputContext context) { m_DefaultContext = context; }

		EInputContext GetCurrentContext() const;
		const std::vector<EInputContext>& GetAllContexts() const { return m_ContextStack; }

		bool CanReceiveInput() const;
		bool IsContextActive(EInputContext context) const;

		std::function<void(EInputContext, EInputContext)> OnContextChanged;

	private:
		std::vector<EInputContext> m_ContextStack;
		EInputContext m_DefaultContext = EInputContext::Gameplay;
	};

	class UIGameModule
	{
	public:
		UIGameModule() = default;
		static UIGameModule& Get();

		void Initialize();
		void Shutdown();

		void Update(float deltaTime);

		ScreenStack& GetScreenStack() { return ScreenStack::Get(); }
		FocusManager& GetFocusManager() { return FocusManager::Get(); }
		InputContextManager& GetInputContextManager() { return InputContextManager::Get(); }

		void SetGlobalUIEnabled(bool enabled) { m_UIEnabled = enabled; }
		bool IsGlobalUIEnabled() const { return m_UIEnabled; }

		void PlayUISound(EUISound sound);

		void SetMouseCursor(ECursorState state);
		ECursorState GetMouseCursor() const { return m_CursorState; }

		void SetScale(float scale) { m_UIScale = scale; }
		float GetScale() const { return m_UIScale; }

		void RegisterScreen(Ref<UIScreen> screen);
		void UnregisterScreen(const std::string& screenId);
		Ref<UIScreen> GetScreen(const std::string& screenId) const;
		bool HasScreen(const std::string& screenId) const;
		std::unordered_map<std::string, Ref<UIScreen>>& GetRegisteredScreens() { return m_RegisteredScreens; }

		std::function<void(EUISound)> OnPlaySound;

	private:
		float m_UIScale = 1.0f;
		ECursorState m_CursorState = ECursorState::Normal;
		bool m_UIEnabled = true;
		std::unordered_map<std::string, Ref<UIScreen>> m_RegisteredScreens;
	};

	inline UIScreen::UIScreen(const std::string& id)
		: m_Id(id) {}

	inline void UIScreen::Show()
	{
		if (m_IsShowing) return;
		m_IsShowing = true;
		OnShow();
	}

	inline void UIScreen::Hide()
	{
		if (!m_IsShowing) return;
		m_IsShowing = false;
		m_IsFocused = false;
		OnHide();
	}

	inline void UIScreen::Toggle()
	{
		if (m_IsShowing) Hide();
		else Show();
	}

	inline void UIScreen::SetFocused(bool focused)
	{
		if (!m_Focusable) return;
		if (focused)
			FocusManager::Get().SetFocus(shared_from_this());
		else if (m_IsFocused)
			FocusManager::Get().ClearFocus();
		m_IsFocused = focused;
	}

	inline void UIScreen::AddChild(Ref<UIScreen> child)
	{
		if (!child) return;
		child->m_Parent = shared_from_this();
		m_WidgetTree.AddChild(child);
	}

	inline void UIScreen::RemoveChild(Ref<UIScreen> child)
	{
		if (!child) return;
		child->m_Parent.reset();
		m_WidgetTree.RemoveChild(child);
	}

	inline ScreenStack& ScreenStack::Get()
	{
		static ScreenStack instance;
		return instance;
	}

	inline void ScreenStack::Push(Ref<UIScreen> screen)
	{
		if (!screen) return;
		if (!m_Screens.empty())
			Top()->Hide();
		m_Screens.push_back(screen);
		screen->Show();
	}

	inline void ScreenStack::Pop()
	{
		if (m_Screens.empty()) return;
		auto screen = m_Screens.back();
		screen->Hide();
		m_Screens.pop_back();
		if (!m_Screens.empty())
			Top()->Show();
	}

	inline void ScreenStack::PopAll()
	{
		for (auto& screen : m_Screens)
			screen->Hide();
		m_Screens.clear();
	}

	inline void ScreenStack::Replace(Ref<UIScreen> screen)
	{
		PopAll();
		Push(screen);
	}

	inline void ScreenStack::Insert(int32_t index, Ref<UIScreen> screen)
	{
		if (!screen || index < 0) return;
		size_t pos = static_cast<size_t>(index);
		if (pos >= m_Screens.size())
		{
			Push(screen);
			return;
		}
		m_Screens.insert(m_Screens.begin() + pos, screen);
		screen->Show();
	}

	inline void ScreenStack::Remove(Ref<UIScreen> screen)
	{
		if (!screen) return;
		auto it = std::find(m_Screens.begin(), m_Screens.end(), screen);
		if (it != m_Screens.end())
		{
			screen->Hide();
			m_Screens.erase(it);
		}
	}

	inline Ref<UIScreen> ScreenStack::Top() const
	{
		return m_Screens.empty() ? nullptr : m_Screens.back();
	}

	inline bool ScreenStack::Contains(Ref<UIScreen> screen) const
	{
		return std::find(m_Screens.begin(), m_Screens.end(), screen) != m_Screens.end();
	}

	inline Ref<UIScreen> ScreenStack::Find(const std::string& screenId) const
	{
		for (const auto& screen : m_Screens)
			if (screen->GetId() == screenId) return screen;
		return nullptr;
	}

	inline bool ScreenStack::IsTop(Ref<UIScreen> screen) const
	{
		return !m_Screens.empty() && m_Screens.back() == screen;
	}

	inline void ScreenStack::Tick(float deltaTime)
	{
		if (!m_InputEnabled) return;
		for (auto& screen : m_Screens)
		{
			if (screen->IsShowing())
				screen->OnTick(deltaTime);
		}
	}

	inline void ScreenStack::Render()
	{
		for (auto& screen : m_Screens)
		{
			if (screen->IsShowing())
				screen->OnRender();
		}
	}

	inline FocusManager& FocusManager::Get()
	{
		static FocusManager instance;
		return instance;
	}

	inline void FocusManager::SetFocus(Ref<UIScreen> screen)
	{
		if (m_LockedScreen && screen != m_LockedScreen) return;

		Ref<UIScreen> old = m_FocusedScreen;
		if (old == screen) return;

		if (old) old->SetFocused(false);

		m_FocusedScreen = screen;
		if (m_FocusedScreen) m_FocusedScreen->SetFocused(true);

		if (OnFocusChanged)
			OnFocusChanged(m_FocusedScreen);
	}

	inline void FocusManager::ClearFocus()
	{
		SetFocus(nullptr);
	}

	inline bool FocusManager::HasFocus(Ref<UIScreen> screen) const
	{
		return m_FocusedScreen == screen;
	}

	inline void FocusManager::LockFocus(Ref<UIScreen> screen)
	{
		m_LockedScreen = screen;
		if (screen && m_FocusedScreen != screen)
			SetFocus(screen);
	}

	inline void FocusManager::UnlockFocus(Ref<UIScreen> screen)
	{
		if (m_LockedScreen == screen)
			m_LockedScreen = nullptr;
	}

	inline void FocusManager::AddFocusable(Ref<UIScreen> screen)
	{
		if (!screen) return;
		if (std::find(m_FocusableScreens.begin(), m_FocusableScreens.end(), screen) == m_FocusableScreens.end())
			m_FocusableScreens.push_back(screen);
	}

	inline void FocusManager::RemoveFocusable(Ref<UIScreen> screen)
	{
		if (!screen) return;
		m_FocusableScreens.erase(
			std::remove(m_FocusableScreens.begin(), m_FocusableScreens.end(), screen),
			m_FocusableScreens.end()
		);
	}

	inline bool FocusManager::NavigateNext(bool reverse)
	{
		if (m_FocusableScreens.empty()) return false;

		auto it = std::find(m_FocusableScreens.begin(), m_FocusableScreens.end(), m_FocusedScreen);
		if (it == m_FocusableScreens.end())
		{
			if (!m_FocusableScreens.empty())
				SetFocus(m_FocusableScreens.front());
			return true;
		}

		if (reverse)
		{
			if (it == m_FocusableScreens.begin())
				SetFocus(m_FocusableScreens.back());
			else
				SetFocus(*(it - 1));
		}
		else
		{
			auto next = it + 1;
			if (next == m_FocusableScreens.end())
				SetFocus(m_FocusableScreens.front());
			else
				SetFocus(*next);
		}
		return true;
	}

	inline InputContextManager& InputContextManager::Get()
	{
		static InputContextManager instance;
		return instance;
	}

	inline void InputContextManager::PushContext(EInputContext context)
	{
		EInputContext old = GetCurrentContext();
		m_ContextStack.push_back(context);
		if (OnContextChanged)
			OnContextChanged(old, context);
	}

	inline void InputContextManager::PopContext()
	{
		if (m_ContextStack.empty()) return;
		EInputContext old = m_ContextStack.back();
		m_ContextStack.pop_back();
		EInputContext current = GetCurrentContext();
		if (OnContextChanged)
			OnContextChanged(old, current);
	}

	inline void InputContextManager::ClearContexts()
	{
		m_ContextStack.clear();
	}

	inline EInputContext InputContextManager::GetCurrentContext() const
	{
		if (!m_ContextStack.empty())
			return m_ContextStack.back();
		return m_DefaultContext;
	}

	inline bool InputContextManager::CanReceiveInput() const
	{
		return GetCurrentContext() != EInputContext::Blocked;
	}

	inline bool InputContextManager::IsContextActive(EInputContext context) const
	{
		if (GetCurrentContext() == context) return true;
		for (EInputContext ctx : m_ContextStack)
			if (ctx == context) return true;
		return false;
	}

	inline UIGameModule& UIGameModule::Get()
	{
		static UIGameModule instance;
		return instance;
	}

	inline void UIGameModule::Initialize()
	{
		InputContextManager::Get().SetDefaultContext(EInputContext::Gameplay);
	}

	inline void UIGameModule::Shutdown()
	{
		ScreenStack::Get().PopAll();
		m_RegisteredScreens.clear();
	}

	inline void UIGameModule::Update(float deltaTime)
	{
		ScreenStack::Get().Tick(deltaTime);
	}

	inline void UIGameModule::SetMouseCursor(ECursorState state)
	{
		m_CursorState = state;
	}

	inline void UIGameModule::RegisterScreen(Ref<UIScreen> screen)
	{
		if (screen)
			m_RegisteredScreens[screen->GetId()] = screen;
	}

	inline void UIGameModule::UnregisterScreen(const std::string& screenId)
	{
		m_RegisteredScreens.erase(screenId);
	}

	inline Ref<UIScreen> UIGameModule::GetScreen(const std::string& screenId) const
	{
		auto it = m_RegisteredScreens.find(screenId);
		return it != m_RegisteredScreens.end() ? it->second : nullptr;
	}

	inline bool UIGameModule::HasScreen(const std::string& screenId) const
	{
		return m_RegisteredScreens.find(screenId) != m_RegisteredScreens.end();
	}
}
