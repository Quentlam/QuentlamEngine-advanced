#include "qlpch.h"
#include "GameUILayer.h"
#include "Quentlam/Core/Application.h"
#include "Quentlam/Events/KeyEvent.h"
#include "Quentlam/Events/MouseEvent.h"
#include <imgui.h>

namespace Quentlam
{

GameUILayer::GameUILayer()
	: Layer("GameUI")
{
}

void GameUILayer::OnAttach()
{
	m_HUD = CreateRef<GameHUDOverlay>();
	m_Inventory = CreateRef<InventoryScreen>();
	m_Dialogue = CreateRef<DialogueScreen>();

	m_InventoryContainer = CreateRef<Container>("player_inventory", 48);
	m_InventoryContainer->SetPlayerInventory(true);

	m_Inventory->SetInventory(m_InventoryContainer);
	m_Inventory->SetPlayerMoney(m_PlayerStats.Money);
	m_Inventory->OnInventoryClosed = [this]() {
		m_HUD->SetInventoryOpen(false);
		InputContextManager::Get().PopContext();
		InputContextManager::Get().PushContext(EInputContext::Gameplay);
	};

	m_Dialogue->OnDialogueClosed = [this]() {
		m_HUD->SetDialogueOpen(false);
		InputContextManager::Get().PopContext();
		InputContextManager::Get().PushContext(EInputContext::Gameplay);
	};
	m_Dialogue->OnResponseSelected = [this](int32_t index) {
		QL_CORE_INFO("Dialogue response selected: {0}", index);
	};

	m_HUD->SetStats(m_PlayerStats);

	UIGameModule::Get().RegisterScreen(m_HUD);
	UIGameModule::Get().RegisterScreen(m_Inventory);
	UIGameModule::Get().RegisterScreen(m_Dialogue);

	ScreenStack::Get().Push(m_HUD);

	UIGameModule::Get().Initialize();
	InputContextManager::Get().PushContext(EInputContext::Gameplay);

	m_PlayerStats.Money = 500;
	m_PlayerStats.Stamina = 100.0f;
	m_PlayerStats.Health = 100;
}

void GameUILayer::OnDetach()
{
	ScreenStack::Get().PopAll();
	UIGameModule::Get().Shutdown();
}

void GameUILayer::OnUpdate(Timestep ts)
{
	m_PlayerStats.Stamina = std::max(0.0f, m_PlayerStats.Stamina - ts * 0.5f);
	m_PlayerStats.Water = std::max(0.0f, m_PlayerStats.Water - ts * 0.3f);
	m_HUD->SetStats(m_PlayerStats);

	if (m_InventoryContainer)
		m_Inventory->SetInventory(m_InventoryContainer);
	m_Inventory->SetPlayerMoney(m_PlayerStats.Money);

	UIGameModule::Get().Update(ts.GetSeconds());
}

void GameUILayer::OnEvent(Event& event)
{
	if (event.GetEventType() == EventType::KeyPressed)
	{
		auto& e = (KeyPressedEvent&)event;
		OnKeyPressed(e);
	}
	if (event.GetEventType() == EventType::MouseButtonPressed)
	{
		auto& e = (MouseButtonPressedEvent&)event;
		OnMouseButtonPressed(e);
	}
}

void GameUILayer::OnImGuiLayer()
{
	ScreenStack::Get().Render();
}

bool GameUILayer::OnKeyPressed(KeyPressedEvent& e)
{
	int key = e.GetKeyCode();

	if (key == Key::Escape)
	{
		if (ScreenStack::Get().IsTop(m_Inventory))
		{
			CloseInventory();
			return true;
		}
		if (ScreenStack::Get().IsTop(m_Dialogue))
		{
			CloseDialogue();
			return true;
		}
	}

	if (key == Key::I)
	{
		if (!ScreenStack::Get().IsTop(m_Inventory))
		{
			OpenInventory();
			return true;
		}
	}

	return false;
}

bool GameUILayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	if (ScreenStack::Get().IsTop(m_Inventory))
	{
		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			CloseInventory();
			return true;
		}
	}

	if (ScreenStack::Get().IsTop(m_Dialogue))
	{
		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			CloseDialogue();
			return true;
		}
	}

	return false;
}

void GameUILayer::OpenInventory()
{
	if (ScreenStack::Get().IsTop(m_Inventory))
		return;

	ScreenStack::Get().Push(m_Inventory);
	m_HUD->SetInventoryOpen(true);
	InputContextManager::Get().PopContext();
	InputContextManager::Get().PushContext(EInputContext::Inventory);
}

void GameUILayer::CloseInventory()
{
	if (ScreenStack::Get().IsTop(m_Inventory))
	{
		ScreenStack::Get().Pop();
		m_HUD->SetInventoryOpen(false);
		InputContextManager::Get().PopContext();
		InputContextManager::Get().PushContext(EInputContext::Gameplay);
	}
}

void GameUILayer::ShowDialogue(const std::string& npcName, const std::string& text,
	const std::vector<std::string>& responses)
{
	m_Dialogue->StartDialogue(npcName, "", text, responses);
	m_HUD->SetDialogueOpen(true);
	InputContextManager::Get().PopContext();
	InputContextManager::Get().PushContext(EInputContext::Dialogue);
}

void GameUILayer::CloseDialogue()
{
	if (ScreenStack::Get().IsTop(m_Dialogue))
	{
		m_Dialogue->CloseDialogue();
		m_HUD->SetDialogueOpen(false);
		InputContextManager::Get().PopContext();
		InputContextManager::Get().PushContext(EInputContext::Gameplay);
	}
}

}
