#pragma once
#include "Quentlam/Core/Layer.h"
#include "Quentlam/Events/KeyEvent.h"
#include "Quentlam/Events/MouseEvent.h"
#include "Quentlam/UI/GameUIScreens.h"
#include "Quentlam/Input/InputActionSystem.h"
#include <memory>

namespace Quentlam
{

class GameUILayer : public Layer
{
public:
	GameUILayer();
	~GameUILayer() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& event) override;
	void OnImGuiLayer() override;

	GameHUDOverlay* GetHUD() const { return m_HUD.get(); }
	InventoryScreen* GetInventory() const { return m_Inventory.get(); }
	DialogueScreen* GetDialogue() const { return m_Dialogue.get(); }

	PlayerStats& GetPlayerStats() { return m_PlayerStats; }

	void OpenInventory();
	void CloseInventory();
	bool IsInventoryOpen() const { return m_HUD && m_HUD->IsInventoryOpen(); }

	void ShowDialogue(const std::string& npcName, const std::string& text,
		const std::vector<std::string>& responses = {});
	void CloseDialogue();

	void SetInventoryContainer(Ref<Container> inv) { m_InventoryContainer = inv; }

private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	Ref<GameHUDOverlay> m_HUD;
	Ref<InventoryScreen> m_Inventory;
	Ref<DialogueScreen> m_Dialogue;

	PlayerStats m_PlayerStats;
	Ref<Container> m_InventoryContainer;
};

}
