#pragma once
#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include <functional>
#include <vector>
#include <string>
#include <entt/entt.hpp>

namespace Quentlam
{

enum class EUIEventType
{
	PointerEnter,
	PointerExit,
	PointerDown,
	PointerUp,
	PointerClick,
	BeginDrag,
	Drag,
	EndDrag,
	Select,
	Deselect,
	ValueChanged
};

struct UIEventData
{
	EUIEventType Type = EUIEventType::PointerClick;
	glm::vec2 PointerPosition;
	glm::vec2 Delta;
	uint32_t PointerId = 0;
	float DragThreshold = 5.0f;
};

using UIEventCallback = std::function<void(UIEventData)>;

class UIElement
{
public:
	void AddEventListener(EUIEventType type, UIEventCallback callback)
	{
		m_EventListeners[type].push_back(callback);
	}

	void RemoveEventListeners(EUIEventType type)
	{
		m_EventListeners[type].clear();
	}

	bool HandleEvent(const UIEventData& data)
	{
		auto it = m_EventListeners.find(data.Type);
		if (it != m_EventListeners.end())
		{
			for (auto& cb : it->second)
			{
				cb(data);
			}
			return true;
		}
		return false;
	}

private:
	std::unordered_map<EUIEventType, std::vector<UIEventCallback>> m_EventListeners;
};

class EventSystem
{
public:
	static EventSystem& Get();

	void ProcessPointerDown(const glm::vec2& pos, uint32_t pointerId);
	void ProcessPointerMove(const glm::vec2& pos, uint32_t pointerId);
	void ProcessPointerUp(const glm::vec2& pos, uint32_t pointerId);

	void RegisterElement(entt::entity entity);
	void UnregisterElement(entt::entity entity);

private:
	EventSystem() = default;

	struct PointerState
	{
		glm::vec2 Position;
		glm::vec2 StartPosition;
		entt::entity HoveredElement = entt::null;
		entt::entity DraggedElement = entt::null;
		bool IsDragging = false;
	};

	std::unordered_map<uint32_t, PointerState> m_PointerStates;
	std::vector<entt::entity> m_RegisteredElements;
};

}
