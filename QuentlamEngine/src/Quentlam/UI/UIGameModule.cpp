#include "qlpch.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Audio/AudioModule.h"

namespace Quentlam
{

void UIGameModule::PlayUISound(EUISound sound)
{
	if (!m_UIEnabled || sound == EUISound::None)
		return;

	if (OnPlaySound)
		OnPlaySound(sound);
}

void UIGameModule::RegisterScreenForEntity(entt::entity entity, Ref<UIScreen> screen)
{
	if (!screen || screen->GetId().empty())
		return;

	std::string id = screen->GetId();
	m_EntityToScreenId[entity] = id;
	m_ScreenIdToEntity[id] = entity;
	m_RegisteredScreens[id] = screen;
}

void UIGameModule::RegisterScreen(Ref<UIScreen> screen)
{
	if (screen)
		m_RegisteredScreens[screen->GetId()] = screen;
}

void UIGameModule::UnregisterScreen(const std::string& screenId)
{
	auto entityIt = m_ScreenIdToEntity.find(screenId);
	if (entityIt != m_ScreenIdToEntity.end())
	{
		m_EntityToScreenId.erase(entityIt->second);
		m_ScreenIdToEntity.erase(entityIt);
	}
	m_RegisteredScreens.erase(screenId);
}

void UIGameModule::UnregisterScreenForEntity(entt::entity entity)
{
	auto it = m_EntityToScreenId.find(entity);
	if (it == m_EntityToScreenId.end())
		return;

	std::string id = it->second;
	m_ScreenIdToEntity.erase(id);
	m_EntityToScreenId.erase(it);
	m_RegisteredScreens.erase(id);
}

Ref<UIScreen> UIGameModule::GetScreenForEntity(entt::entity entity) const
{
	auto it = m_EntityToScreenId.find(entity);
	if (it == m_EntityToScreenId.end())
		return nullptr;
	auto screenIt = m_RegisteredScreens.find(it->second);
	if (screenIt == m_RegisteredScreens.end())
		return nullptr;
	return screenIt->second;
}

entt::entity UIGameModule::GetEntityForScreen(const std::string& screenId) const
{
	auto it = m_ScreenIdToEntity.find(screenId);
	if (it == m_ScreenIdToEntity.end())
		return entt::null;
	return it->second;
}

}
