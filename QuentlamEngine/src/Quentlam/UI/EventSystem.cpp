#include "qlpch.h"
#include "EventSystem.h"
#include "Quentlam/Core/Log.h"
#include <glm/glm.hpp>

namespace Quentlam
{

EventSystem& EventSystem::Get()
{
	static EventSystem instance;
	return instance;
}

void EventSystem::ProcessPointerDown(const glm::vec2& pos, uint32_t pointerId)
{
	auto& state = m_PointerStates[pointerId];
	state.Position = pos;
	state.StartPosition = pos;

	for (auto it = m_RegisteredElements.rbegin(); it != m_RegisteredElements.rend(); ++it)
	{
	}
}

void EventSystem::ProcessPointerMove(const glm::vec2& pos, uint32_t pointerId)
{
	auto& state = m_PointerStates[pointerId];
	state.Position = pos;
}

void EventSystem::ProcessPointerUp(const glm::vec2& pos, uint32_t pointerId)
{
	auto it = m_PointerStates.find(pointerId);
	if (it != m_PointerStates.end())
		m_PointerStates.erase(it);
}

void EventSystem::RegisterElement(entt::entity entity)
{
	m_RegisteredElements.push_back(entity);
}

void EventSystem::UnregisterElement(entt::entity entity)
{
	auto it = std::find(m_RegisteredElements.begin(), m_RegisteredElements.end(), entity);
	if (it != m_RegisteredElements.end())
		m_RegisteredElements.erase(it);
}

}
