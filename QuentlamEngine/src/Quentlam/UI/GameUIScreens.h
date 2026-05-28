#pragma once
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Gameplay/Simulation/SimulationModule.h"
#include "Quentlam/Gameplay/Inventory/InventoryModule.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace Quentlam
{

struct PlayerStats
{
	int32_t Money = 500;
	int32_t MaxHealth = 100;
	int32_t Health = 100;
	float MaxStamina = 100.0f;
	float Stamina = 100.0f;
	float MaxWater = 100.0f;
	float Water = 100.0f;
};

class GameHUDOverlay : public UIScreen
{
public:
	GameHUDOverlay();
	~GameHUDOverlay() override = default;

	void OnShow() override;
	void OnTick(float deltaTime) override;
	void OnRender() override;

	void SetStats(const PlayerStats& stats) { m_Stats = stats; }
	const PlayerStats& GetStats() const { return m_Stats; }

	void SetInventoryOpen(bool open) { m_InventoryOpen = open; }
	bool IsInventoryOpen() const { return m_InventoryOpen; }

	void SetDialogueOpen(bool open) { m_DialogueOpen = open; }
	bool IsDialogueOpen() const { return m_DialogueOpen; }

	void TriggerStaminaFlash() { m_StaminaFlashTimer = 0.5f; }

	void SetInteractionPrompt(const std::string& prompt) { m_InteractionPrompt = prompt; }

private:
	void RenderTopBar();
	void RenderBottomLeft();
	void RenderBottomRight();
	void RenderMinimap();
	void RenderInteractionPrompt();

	PlayerStats m_Stats;
	float m_StaminaFlashTimer = 0.0f;
	float m_HealthFlashTimer = 0.0f;
	float m_MoneyBounceTimer = 0.0f;
	int32_t m_LastMoney = 0;
	std::string m_InteractionPrompt;
	float m_PromptAlpha = 0.0f;
	bool m_InventoryOpen = false;
	bool m_DialogueOpen = false;
};

class InventoryScreen : public UIScreen
{
public:
	InventoryScreen();
	~InventoryScreen() override = default;

	void OnShow() override;
	void OnHide() override;
	void OnTick(float deltaTime) override;
	void OnRender() override;

	bool OnMouseClick(float x, float y, int32_t button, bool pressed) override;
	bool OnMouseMove(float x, float y) override;

	void SetInventory(Ref<Container> inv) { m_Inventory = inv; }
	Ref<Container> GetInventory() const { return m_Inventory; }

	void SetPlayerMoney(int32_t money) { m_PlayerMoney = money; }
	int32_t GetPlayerMoney() const { return m_PlayerMoney; }

	void ForceClose() { Hide(); }

	std::function<void(const std::string& itemId)> OnItemSelected;
	std::function<void(const std::string& itemId)> OnItemUsed;
	std::function<void()> OnInventoryClosed;

private:
	void RenderInventoryGrid();
	void RenderItemTooltip(const ItemStack& stack, const ItemDef* def, const glm::vec2& mousePos);
	void RenderItemIcon(const ItemDef* def, const glm::vec2& pos, const glm::vec2& size);
	ImU32 GetCategoryColor(EItemCategory category) const;
	void RenderQualityStars(EItemQuality quality, ImVec2 pos, float size);
	void RenderCategoryFilter();
	bool MatchesFilter(const ItemDef* def) const;

	Ref<Container> m_Inventory;
	int32_t m_PlayerMoney = 0;

	int32_t m_DraggedSlot = -1;
	int32_t m_HoveredSlot = -1;
	glm::vec2 m_MousePos;
	glm::vec2 m_MousePosStart;
	bool m_Dragging = false;
	glm::vec2 m_DragOffset = { 0, 0 };

	static constexpr int32_t COLS = 12;
	static constexpr int32_t ROWS = 4;
	static constexpr float SLOT_SIZE = 52.0f;
	static constexpr float SLOT_PADDING = 4.0f;

	const ItemDef* m_TooltipDef = nullptr;
	ItemStack m_TooltipStack;
	glm::vec2 m_TooltipPos;

	static constexpr int MAX_CATEGORY_FILTERS = 8;
	int32_t m_ActiveFilterCount = 0;
	EItemCategory m_ActiveFilters[MAX_CATEGORY_FILTERS];
	char m_SearchBuffer[64] = { 0 };
	bool m_ShowSearchBar = false;
};

class DialogueScreen : public UIScreen
{
public:
	DialogueScreen();
	~DialogueScreen() override = default;

	void OnShow() override;
	void OnHide() override;
	void OnTick(float deltaTime) override;
	void OnRender() override;

	bool OnMouseClick(float x, float y, int32_t button, bool pressed) override;
	bool OnKeyInput(int32_t key, bool pressed) override;

	void StartDialogue(
		const std::string& npcName,
		const std::string& portraitPath,
		const std::string& text,
		const std::vector<std::string>& responses = {}
	);

	void AdvanceText();
	void CloseDialogue();

	bool IsActive() const { return m_Active; }
	bool IsComplete() const { return m_Complete; }

	std::function<void(int32_t responseIndex)> OnResponseSelected;
	std::function<void()> OnDialogueClosed;

private:
	void RenderDialogueBox();
	void RenderPortrait();
	void RenderResponses();
	void RenderNameTag();

	bool m_Active = false;
	bool m_Complete = false;
	bool m_WaitingForInput = false;
	float m_TypewriterTimer = 0.0f;
	float m_TypewriterSpeed = 30.0f;
	int32_t m_VisibleCharCount = 0;
	std::string m_FullText;

	std::string m_NpcName;
	std::string m_PortraitPath;
	std::vector<std::string> m_Responses;
	int32_t m_SelectedResponse = -1;

	float m_BoxHeight = 200.0f;
	float m_PortraitWidth = 120.0f;
	float m_NameTagHeight = 30.0f;

	glm::vec2 m_MousePos;
};

}
