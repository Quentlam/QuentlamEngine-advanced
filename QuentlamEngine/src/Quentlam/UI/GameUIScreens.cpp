#include "qlpch.h"
#include "GameUIScreens.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Core/Application.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Quentlam
{

// =====================================================================
//  GameHUDOverlay
// =====================================================================

GameHUDOverlay::GameHUDOverlay()
	: UIScreen("game_hud")
{
	m_Stats.Money = 500;
	m_Stats.Stamina = 100.0f;
	m_Stats.MaxStamina = 100.0f;
	m_Stats.Health = 100;
	m_Stats.MaxHealth = 100;
	m_Stats.Water = 100.0f;
	m_Stats.MaxWater = 100.0f;
}

void GameHUDOverlay::OnShow()
{
	m_LastMoney = m_Stats.Money;
}

void GameHUDOverlay::OnTick(float deltaTime)
{
	if (m_Stats.Money != m_LastMoney)
	{
		m_MoneyBounceTimer = 0.3f;
		m_LastMoney = m_Stats.Money;
	}

	if (m_StaminaFlashTimer > 0.0f)
		m_StaminaFlashTimer -= deltaTime;
	if (m_HealthFlashTimer > 0.0f)
		m_HealthFlashTimer -= deltaTime;
	if (m_MoneyBounceTimer > 0.0f)
		m_MoneyBounceTimer -= deltaTime;

	if (!m_InteractionPrompt.empty())
	{
		m_PromptAlpha = 1.0f;
	}
	else
	{
		m_PromptAlpha = 0.0f;
	}
}

void GameHUDOverlay::OnRender()
{
	if (ImGui::GetCurrentContext() == nullptr) return;

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoFocusOnAppearing;

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	// ---- TOP BAR (time, season, day, weather, money) ----
	{
		float topH = 40.0f;
		if (ImGui::Begin("##HUDTopBar", nullptr, flags))
		{
			ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetWindowSize(ImVec2(displaySize.x, topH));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 barMin = ImGui::GetWindowPos();
			ImVec2 barMax = ImVec2(barMin.x + displaySize.x, barMin.y + topH);

			// Dark semi-transparent bar
			drawList->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 0, 160));

			float innerW = displaySize.x - 20.0f;
			float xPos = barMin.x + 10.0f;

			// -- Clock --
			auto& sim = SimulationModule::Get();
			auto& clock = sim.GetClock();
			std::string timeStr = clock.GetTimeString();
			std::string dateStr = sim.GetCalendar().GetDateString(
				sim.GetCurrentDay(), sim.GetCurrentSeason(), sim.GetCurrentYear());

			ImGui::SetCursorPos(ImVec2(xPos, (topH - 20.0f) * 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 240, 180, 255));
			ImGui::Text("%s", timeStr.c_str());
			xPos += ImGui::GetItemRectSize().x + 12.0f;
			ImGui::PopStyleColor();

			ImGui::SetCursorPos(ImVec2(xPos, (topH - 16.0f) * 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
			ImGui::Text("%s", dateStr.c_str());
			xPos += ImGui::GetItemRectSize().x + 12.0f;
			ImGui::PopStyleColor();

			// -- Weather icon (text-based) --
			ImGui::SetCursorPos(ImVec2(xPos, (topH - 16.0f) * 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 210, 255, 255));
			const char* weatherIcon = "?";
			switch (sim.GetCurrentWeather())
			{
			case EWeather::Sunny: weatherIcon = "sun:"; break;
			case EWeather::Cloudy: weatherIcon = "cloud:"; break;
			case EWeather::Rainy: weatherIcon = "rain:"; break;
			case EWeather::Storm: weatherIcon = "storm:"; break;
			case EWeather::Snow: weatherIcon = "snow:"; break;
			case EWeather::Windy: weatherIcon = "wind:"; break;
			}
			ImGui::Text("%s", weatherIcon);
			xPos += 50.0f;
			ImGui::PopStyleColor();

			// -- Clock pause indicator --
			if (clock.IsPaused())
			{
				ImGui::SetCursorPos(ImVec2(xPos, (topH - 16.0f) * 0.5f));
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
				ImGui::Text("[PAUSED]");
				xPos += 60.0f;
				ImGui::PopStyleColor();
			}

			// -- Money (right-aligned) --
			float moneyX = barMax.x - 10.0f;
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 80, 255));
			std::string moneyStr = "G " + std::to_string(m_Stats.Money);
			float moneyW = ImGui::CalcTextSize(moneyStr.c_str()).x;
			moneyX -= moneyW;
			float bounce = m_MoneyBounceTimer > 0.0f ? sinf(m_MoneyBounceTimer * 20.0f) * 3.0f : 0.0f;
			ImGui::SetCursorPos(ImVec2(moneyX, (topH - 16.0f) * 0.5f + bounce));
			ImGui::Text("%s", moneyStr.c_str());
			ImGui::PopStyleColor();
		}
		ImGui::End();
	}

	// ---- BOTTOM LEFT (stamina bar, health bar) ----
	{
		float barW = 200.0f;
		float barH = 20.0f;
		float margin = 10.0f;
		if (ImGui::Begin("##HUDBottomLeft", nullptr, flags))
		{
			ImGui::SetWindowPos(ImVec2(margin, displaySize.y - barH - margin - 28.0f), ImGuiCond_Always);
			ImGui::SetWindowSize(ImVec2(barW, barH * 2 + 6.0f));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 winPos = ImGui::GetWindowPos();

			// Stamina bar
			ImVec2 sBarMin(winPos.x, winPos.y);
			ImVec2 sBarMax(sBarMin.x + barW, sBarMin.y + barH);
			float staminaRatio = m_Stats.MaxStamina > 0 ? m_Stats.Stamina / m_Stats.MaxStamina : 0.0f;

			drawList->AddRectFilled(sBarMin, sBarMax, IM_COL32(30, 30, 30, 200));
			ImU32 staminaCol = IM_COL32(100, 200, 80, 230);
			if (m_StaminaFlashTimer > 0.0f && ((int)(m_StaminaFlashTimer * 10.0f) % 2 == 0))
				staminaCol = IM_COL32(255, 50, 50, 230);
			if (staminaRatio < 0.3f)
				staminaCol = IM_COL32(220, 150, 50, 230);
			drawList->AddRectFilled(sBarMin, ImVec2(sBarMin.x + barW * staminaRatio, sBarMax.y), staminaCol);
			drawList->AddRect(sBarMin, sBarMax, IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.5f);

			ImGui::SetCursorPos(ImVec2(4.0f, (barH - 14.0f) * 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 230));
			ImGui::Text("stamina:%.0f", m_Stats.Stamina);
			ImGui::PopStyleColor();

			// Health bar
			ImGui::SetCursorPos(ImVec2(0, barH + 3.0f));
			ImVec2 hBarMin(winPos.x, winPos.y + barH + 3.0f);
			ImVec2 hBarMax(hBarMin.x + barW, hBarMin.y + barH);
			float healthRatio = m_Stats.MaxHealth > 0 ? (float)m_Stats.Health / (float)m_Stats.MaxHealth : 0.0f;

			drawList->AddRectFilled(hBarMin, hBarMax, IM_COL32(30, 30, 30, 200));
			ImU32 healthCol = IM_COL32(200, 60, 60, 230);
			if (m_HealthFlashTimer > 0.0f)
				healthCol = IM_COL32(255, 255, 100, 230);
			drawList->AddRectFilled(hBarMin, ImVec2(hBarMin.x + barW * healthRatio, hBarMax.y), healthCol);
			drawList->AddRect(hBarMin, hBarMax, IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.5f);

			ImGui::SetCursorPos(ImVec2(4.0f, barH + 3.0f + (barH - 14.0f) * 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 230));
			ImGui::Text("HP:%d/%d", m_Stats.Health, m_Stats.MaxHealth);
			ImGui::PopStyleColor();
		}
		ImGui::End();
	}

	// ---- BOTTOM RIGHT (quick action icons) ----
	{
		float iconSize = 48.0f;
		float padding = 6.0f;
		float totalW = iconSize * 4 + padding * 3;
		float totalH = iconSize + 8.0f;
		if (ImGui::Begin("##HUDBottomRight", nullptr, flags))
		{
			ImGui::SetWindowPos(ImVec2(displaySize.x - totalW - 10.0f, displaySize.y - totalH - 10.0f), ImGuiCond_Always);
			ImGui::SetWindowSize(ImVec2(totalW, totalH));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 winPos = ImGui::GetWindowPos();

			ImU32 slotBg = IM_COL32(20, 20, 20, 180);
			ImU32 slotBorder = IM_COL32(120, 100, 60, 200);
			ImU32 slotBorderHot = IM_COL32(255, 220, 100, 255);
			ImU32 iconText = IM_COL32(200, 180, 100, 255);

			const char* icons[] = { "[I]", "[E]", "[Q]", "[K]" };
			const char* tooltips[] = { "Inventory (I)", "Eat (E)", "Quest (Q)", "Map (K)" };

			for (int i = 0; i < 4; ++i)
			{
				ImVec2 sMin(winPos.x + i * (iconSize + padding), winPos.y + 4.0f);
				ImVec2 sMax(ImVec2(sMin.x + iconSize, sMin.y + iconSize));
				bool hovered = ImGui::IsMouseHoveringRect(sMin, sMax);

				drawList->AddRectFilled(sMin, sMax, slotBg);
				drawList->AddRect(sMin, sMax, hovered ? slotBorderHot : slotBorder, 2.0f);

				ImVec2 textSize = ImGui::CalcTextSize(icons[i]);
				ImVec2 textPos(sMin.x + (iconSize - textSize.x) * 0.5f, sMin.y + (iconSize - textSize.y) * 0.5f);
				drawList->AddText(textPos, iconText, icons[i]);

				if (hovered)
				{
					ImVec2 tipMin(sMax.x + 4, sMin.y);
					ImVec2 tipMax(tipMin.x + ImGui::CalcTextSize(tooltips[i]).x + 8, sMin.y + 22);
					drawList->AddRectFilled(tipMin, tipMax, IM_COL32(10, 10, 10, 220));
					drawList->AddRect(tipMin, tipMax, IM_COL32(200, 180, 80, 255));
					drawList->AddText(ImVec2(tipMin.x + 4, tipMin.y + 3), IM_COL32(255, 255, 255, 255), tooltips[i]);
				}
			}
		}
		ImGui::End();
	}

	// ---- INTERACTION PROMPT ----
	{
		float promptH = 30.0f;
		float promptW = 250.0f;
		float centerX = (displaySize.x - promptW) * 0.5f;
		float bottomY = displaySize.y - 60.0f - promptH - 28.0f;

		if (!m_InteractionPrompt.empty() && m_PromptAlpha > 0.0f)
		{
			if (ImGui::Begin("##HUDInteractionPrompt", nullptr, flags))
			{
				ImGui::SetWindowPos(ImVec2(centerX, bottomY), ImGuiCond_Always);
				ImGui::SetWindowSize(ImVec2(promptW, promptH));

				ImDrawList* drawList = ImGui::GetWindowDrawList();
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 pMin(winPos.x, winPos.y);
				ImVec2 pMax(winPos.x + promptW, winPos.y + promptH);

				ImU32 bg = IM_COL32(0, 0, 0, (int)(180 * m_PromptAlpha));
				drawList->AddRectFilled(pMin, pMax, bg);
				drawList->AddRect(pMin, pMax, IM_COL32(220, 200, 100, (int)(200 * m_PromptAlpha)));

				ImGui::SetCursorPos(ImVec2(8.0f, (promptH - 16.0f) * 0.5f));
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 230, 150, (int)(255 * m_PromptAlpha)));
				ImGui::Text("%s", m_InteractionPrompt.c_str());
				ImGui::PopStyleColor();
			}
			ImGui::End();
		}
	}
}

void GameHUDOverlay::RenderTopBar()
{
}

void GameHUDOverlay::RenderBottomLeft()
{
}

void GameHUDOverlay::RenderBottomRight()
{
}

void GameHUDOverlay::RenderMinimap()
{
}

void GameHUDOverlay::RenderInteractionPrompt()
{
}

// =====================================================================
//  InventoryScreen
// =====================================================================

InventoryScreen::InventoryScreen()
	: UIScreen("inventory")
{
	m_Inventory = CreateRef<Container>("player_inventory", COLS * ROWS);
	SetBlocking(true);
	SetFocusable(true);
	memset(m_ActiveFilters, 0, sizeof(m_ActiveFilters));
	m_ActiveFilterCount = 0;
}

void InventoryScreen::OnShow()
{
	FocusManager::Get().LockFocus(shared_from_this());
}

void InventoryScreen::OnHide()
{
	FocusManager::Get().UnlockFocus(shared_from_this());
	m_Dragging = false;
	m_DraggedSlot = -1;
	if (OnInventoryClosed)
		OnInventoryClosed();
}

void InventoryScreen::OnTick(float deltaTime)
{
}

void InventoryScreen::OnRender()
{
	if (ImGui::GetCurrentContext() == nullptr) return;

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SLOT_PADDING, SLOT_PADDING));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

	float totalW = COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING;
	float totalH = 60.0f + 30.0f + 20.0f + ROWS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 30.0f;

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImVec2 winPos((displaySize.x - totalW) * 0.5f, (displaySize.y - totalH) * 0.5f);

	ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(totalW, totalH));

	ImGuiWindowFlags winFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoFocusOnAppearing;

	if (ImGui::Begin("Inventory", nullptr, winFlags))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 headerBg = IM_COL32(35, 25, 15, 255);
		ImU32 headerBorder = IM_COL32(120, 90, 40, 255);
		ImVec2 headerMin(winPos.x, winPos.y);
		ImVec2 headerMax(winPos.x + totalW, winPos.y + 56.0f);
		drawList->AddRectFilled(headerMin, headerMax, headerBg);
		drawList->AddRect(headerMin, headerMax, headerBorder, 8.0f, 0, 2.0f);

		ImGui::SetCursorPos(ImVec2(8.0f, 18.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 120, 255));
		ImGui::Text("背包");
		ImGui::PopStyleColor();

		char moneyBuf[64];
		snprintf(moneyBuf, sizeof(moneyBuf), "G %d", m_PlayerMoney);
		float moneyW = ImGui::CalcTextSize(moneyBuf).x;
		ImGui::SetCursorPos(ImVec2(totalW - moneyW - 16.0f, 18.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 80, 255));
		ImGui::Text("%s", moneyBuf);
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(ImVec2(0, 56.0f));

		RenderCategoryFilter();

		float gridY = winPos.y + 56.0f + 32.0f;

		for (int row = 0; row < ROWS; ++row)
		{
			for (int col = 0; col < COLS; ++col)
			{
				int32_t slotIdx = row * COLS + col;
				if (col > 0)
					ImGui::SameLine();

				char slotLabel[16];
				snprintf(slotLabel, sizeof(slotLabel), "##slot%d", slotIdx);

				ImVec4 slotBg = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
				slotBg.w = 0.6f;

				ImGui::PushID(slotIdx);
				if (ImGui::Selectable(slotLabel, false, 0, ImVec2(SLOT_SIZE, SLOT_SIZE)))
				{
					if (m_Dragging && m_DraggedSlot >= 0)
					{
						if (m_Inventory)
							m_Inventory->MoveItem(m_DraggedSlot, slotIdx);
						m_Dragging = false;
						m_DraggedSlot = -1;
					}
					else
					{
						auto* slot = m_Inventory ? m_Inventory->GetSlot(slotIdx) : nullptr;
						if (slot && !slot->IsEmpty())
						{
							const auto* def = ItemDefLibrary::Get().GetDef(slot->Stack.ItemId);
							if (OnItemSelected && def)
								OnItemSelected(def->Id);
						}
					}
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				{
					if (m_Inventory)
					{
						auto* slot = m_Inventory->GetSlot(slotIdx);
						if (slot && !slot->IsEmpty())
						{
							m_Inventory->RemoveItem(slotIdx, 1);
							if (OnItemUsed)
								OnItemUsed(slot->Stack.ItemId);
						}
					}
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && !m_Dragging)
				{
					if (m_Inventory)
					{
						auto* slot = m_Inventory->GetSlot(slotIdx);
						if (slot && !slot->IsEmpty())
						{
							m_Dragging = true;
							m_DraggedSlot = slotIdx;
						}
					}
				}

				if (ImGui::IsItemHovered() && m_Dragging && m_DraggedSlot != slotIdx)
					ImGui::SetTooltip("\u6539\u4e3a\u7f6e\u6362");

				m_HoveredSlot = slotIdx;
				ImGui::PopID();
			}
		}

		if (m_Inventory)
		{
			for (int row = 0; row < ROWS; ++row)
			{
				for (int col = 0; col < COLS; ++col)
				{
					int32_t slotIdx = row * COLS + col;
					auto* slot = m_Inventory->GetSlot(slotIdx);
					if (!slot || slot->IsEmpty()) continue;

					const auto* def = ItemDefLibrary::Get().GetDef(slot->Stack.ItemId);
					if (!def) continue;

					if (m_ActiveFilterCount > 0 && !MatchesFilter(def))
						continue;

					float itemX = winPos.x + SLOT_PADDING + col * (SLOT_SIZE + SLOT_PADDING) + 4.0f;
					float itemY = gridY + row * (SLOT_SIZE + SLOT_PADDING) + 4.0f;

					ImU32 bg = IM_COL32(30, 25, 15, 200);
					if (slotIdx == m_HoveredSlot)
						bg = IM_COL32(60, 50, 25, 240);
					if (slotIdx == m_DraggedSlot)
						bg = IM_COL32(50, 40, 20, 200);
					drawList->AddRectFilled(
						ImVec2(itemX, itemY),
						ImVec2(itemX + SLOT_SIZE - 8.0f, itemY + SLOT_SIZE - 8.0f),
						bg
					);

					ImU32 border = IM_COL32(100, 80, 40, 200);
					drawList->AddRect(
						ImVec2(itemX, itemY),
						ImVec2(itemX + SLOT_SIZE - 8.0f, itemY + SLOT_SIZE - 8.0f),
						border, 2.0f, 0, 1.5f
					);

					ImU32 textColor = IM_COL32(255, 255, 255, 230);
					float iconSize = SLOT_SIZE - 14.0f;

					ImU32 catCol = GetCategoryColor(def->Category);
					ImVec2 catBoxMin(itemX + 2.0f, itemY + 2.0f);
					ImVec2 catBoxMax(itemX + 10.0f, itemY + 10.0f);
					drawList->AddRectFilled(catBoxMin, catBoxMax, catCol);
					drawList->AddRect(catBoxMin, catBoxMax, IM_COL32(80, 60, 30, 200), 1.0f);

					ImVec2 textPos(itemX + 12.0f, itemY + 8.0f);

					float nameLen = ImGui::CalcTextSize(def->Name.c_str()).x;
					std::string displayName = def->Name;
					if (nameLen > iconSize - 4.0f)
					{
						while (!displayName.empty() && ImGui::CalcTextSize(displayName.c_str()).x > iconSize - 4.0f)
							displayName.pop_back();
						displayName += "..";
					}

					drawList->AddText(textPos, textColor, displayName.c_str());

					if (def->MaxQuality > EItemQuality::Normal)
					{
						RenderQualityStars(def->MaxQuality,
							ImVec2(itemX + SLOT_SIZE - 8.0f, itemY + 2.0f), 6.0f);
					}

					if (slot->Stack.StackSize > 1)
					{
						char countBuf[16];
						snprintf(countBuf, sizeof(countBuf), "x%d", slot->Stack.StackSize);
						ImVec2 countSize = ImGui::CalcTextSize(countBuf);
						ImVec2 countPos(
							itemX + SLOT_SIZE - 8.0f - countSize.x - 2.0f,
							itemY + SLOT_SIZE - 8.0f - countSize.y - 2.0f
						);
						drawList->AddText(countPos, IM_COL32(255, 255, 255, 240), countBuf);
					}
				}
			}
		}

		float hintY = gridY + ROWS * (SLOT_SIZE + SLOT_PADDING) + 4.0f;
		ImGui::SetCursorPos(ImVec2(0, hintY));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(130, 130, 130, 255));
		ImGui::Text("Left-click: select/drag  |  Right-click: use  |  ESC: close");
		ImGui::PopStyleColor();
	}
	ImGui::End();

	if (m_Inventory && m_HoveredSlot >= 0 && !m_Dragging)
	{
		auto* slot = m_Inventory->GetSlot(m_HoveredSlot);
		if (slot && !slot->IsEmpty())
		{
			const auto* def = ItemDefLibrary::Get().GetDef(slot->Stack.ItemId);
			if (def)
			{
				ImDrawList* drawList = ImGui::GetBackgroundDrawList();
				float tipW = 200.0f;
				float tipH = def->IsEdible ? 90.0f : 70.0f;
				ImVec2 mousePos = ImGui::GetIO().MousePos;
				ImVec2 tipPos(mousePos.x + 16.0f, mousePos.y + 16.0f);
				if (tipPos.x + tipW > displaySize.x) tipPos.x = mousePos.x - tipW - 16.0f;
				if (tipPos.y + tipH > displaySize.y) tipPos.y = mousePos.y - tipH - 16.0f;

				drawList->AddRectFilled(tipPos, ImVec2(tipPos.x + tipW, tipPos.y + tipH), IM_COL32(12, 10, 6, 250));
				drawList->AddRect(tipPos, ImVec2(tipPos.x + tipW, tipPos.y + tipH), IM_COL32(200, 170, 80, 255), 4.0f, 0, 2.0f);

				ImU32 nameCol = IM_COL32(255, 230, 150, 255);
				drawList->AddText(ImVec2(tipPos.x + 8, tipPos.y + 8), nameCol, def->Name.c_str());

				if (!def->Description.empty())
				{
					ImU32 descCol = IM_COL32(180, 170, 150, 220);
					drawList->AddText(ImVec2(tipPos.x + 8, tipPos.y + 26), descCol, def->Description.c_str());
				}

				char priceBuf[32];
				snprintf(priceBuf, sizeof(priceBuf), "\u94b1\u94c2 %d", def->BaseValue);
				ImU32 priceCol = IM_COL32(255, 220, 80, 230);
				drawList->AddText(ImVec2(tipPos.x + 8, tipPos.y + (def->Description.empty() ? 28 : 46)), priceCol, priceBuf);

				if (def->IsEdible)
				{
					char edibBuf[64];
					snprintf(edibBuf, sizeof(edibBuf), "+\u80fd\u91cf %d  +\u5065\u5eb7 %d",
						def->EdibleEnergy, def->EdibleHealth);
					drawList->AddText(ImVec2(tipPos.x + 8, tipPos.y + 68), IM_COL32(150, 220, 150, 255), edibBuf);
				}
			}
		}
	}

	ImGui::PopStyleVar(3);
}

bool InventoryScreen::OnMouseClick(float x, float y, int32_t button, bool pressed)
{
	if (!pressed && IsShowing())
	{
		Hide();
		return true;
	}
	return false;
}

bool InventoryScreen::OnMouseMove(float x, float y)
{
	m_MousePos = { x, y };
	return false;
}

void InventoryScreen::RenderItemIcon(const ItemDef* def, const glm::vec2& pos, const glm::vec2& size)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	ImU32 bg = IM_COL32(40, 30, 15, 200);
	drawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), bg);
	drawList->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 80, 40, 200));
	if (def)
		drawList->AddText(ImVec2(pos.x + 4, pos.y + 2), IM_COL32(255, 255, 255, 230), def->Name.c_str());
}

void InventoryScreen::RenderItemTooltip(const ItemStack& stack, const ItemDef* def, const glm::vec2& mousePos)
{
}

ImU32 InventoryScreen::GetCategoryColor(EItemCategory category) const
{
	switch (category)
	{
	case EItemCategory::Tool:       return IM_COL32(150, 100, 50, 200);
	case EItemCategory::Seed:       return IM_COL32(100, 180, 80, 200);
	case EItemCategory::Crop:       return IM_COL32(80, 200, 60, 200);
	case EItemCategory::Material:    return IM_COL32(140, 100, 60, 200);
	case EItemCategory::Food:       return IM_COL32(220, 120, 60, 200);
	case EItemCategory::Clothing:   return IM_COL32(100, 80, 160, 200);
	case EItemCategory::Furniture:  return IM_COL32(160, 120, 80, 200);
	case EItemCategory::Quest:      return IM_COL32(200, 180, 60, 200);
	case EItemCategory::Fish:       return IM_COL32(60, 140, 200, 200);
	case EItemCategory::Mineral:    return IM_COL32(120, 120, 160, 200);
	default:                        return IM_COL32(100, 100, 100, 200);
	}
}

void InventoryScreen::RenderQualityStars(EItemQuality quality, ImVec2 pos, float size)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	int starCount = 0;
	switch (quality)
	{
	case EItemQuality::Silver:  starCount = 1; break;
	case EItemQuality::Gold:    starCount = 2; break;
	case EItemQuality::Iridium: starCount = 3; break;
	default: return;
	}
	ImU32 starCol = IM_COL32(255, 220, 80, 255);
	for (int i = 0; i < starCount; ++i)
	{
		float x = pos.x - (starCount - 1) * size * 0.5f + i * size;
		drawList->AddCircleFilled(ImVec2(x, pos.y + size * 0.5f), size * 0.4f, starCol);
	}
}

void InventoryScreen::RenderCategoryFilter()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

	const char* catNames[] = {
		"All", "Tool", "Seed", "Crop", "Food", "Material", "Fish", "Mineral"
	};
	EItemCategory catValues[] = {
		EItemCategory::None, EItemCategory::Tool, EItemCategory::Seed, EItemCategory::Crop,
		EItemCategory::Food, EItemCategory::Material, EItemCategory::Fish, EItemCategory::Mineral
	};
	int catCount = 8;

	for (int i = 0; i < catCount; ++i)
	{
		bool isActive = false;
		if (i == 0)
			isActive = (m_ActiveFilterCount == 0);
		else
		{
			for (int j = 0; j < m_ActiveFilterCount; ++j)
				if (m_ActiveFilters[j] == catValues[i]) { isActive = true; break; }
		}

		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 60, 30, 255));
		else
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 35, 20, 200));

		ImGui::PushID(i);
		if (ImGui::Button(catNames[i], ImVec2(60.0f, 22.0f)))
		{
			if (i == 0)
			{
				m_ActiveFilterCount = 0;
				memset(m_ActiveFilters, 0, sizeof(m_ActiveFilters));
			}
			else
			{
				EItemCategory target = catValues[i];
				bool found = false;
				for (int j = 0; j < m_ActiveFilterCount; ++j)
				{
					if (m_ActiveFilters[j] == target)
					{
						for (int k = j; k < m_ActiveFilterCount - 1; ++k)
							m_ActiveFilters[k] = m_ActiveFilters[k + 1];
						m_ActiveFilterCount--;
						found = true;
						break;
					}
				}
				if (!found && m_ActiveFilterCount < MAX_CATEGORY_FILTERS)
				{
					m_ActiveFilters[m_ActiveFilterCount++] = target;
				}
			}
		}
		ImGui::PopID();
		ImGui::PopStyleColor();

		if (i < catCount - 1)
			ImGui::SameLine();
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);

	char searchBuf[64] = { 0 };
	if (ImGui::InputText("##search", m_SearchBuffer, sizeof(m_SearchBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
	{
	}

	ImGui::PopStyleVar(2);
}

bool InventoryScreen::MatchesFilter(const ItemDef* def) const
{
	if (!def) return false;

	if (m_ActiveFilterCount == 0)
		return true;

	for (int i = 0; i < m_ActiveFilterCount; ++i)
	{
		if (def->Category == m_ActiveFilters[i])
			return true;
	}
	return false;
}

// =====================================================================
//  DialogueScreen
// =====================================================================

DialogueScreen::DialogueScreen()
	: UIScreen("dialogue")
{
	SetBlocking(true);
	SetFocusable(true);
}

void DialogueScreen::OnShow()
{
	m_Active = true;
	m_Complete = false;
	m_WaitingForInput = false;
	m_VisibleCharCount = 0;
	m_TypewriterTimer = 0.0f;
	m_SelectedResponse = -1;
	FocusManager::Get().LockFocus(shared_from_this());
}

void DialogueScreen::OnHide()
{
	m_Active = false;
	FocusManager::Get().UnlockFocus(shared_from_this());
	if (OnDialogueClosed)
		OnDialogueClosed();
}

void DialogueScreen::OnTick(float deltaTime)
{
	if (!m_Active || m_Complete) return;

	if (!m_WaitingForInput && m_VisibleCharCount < (int)m_FullText.length())
	{
		m_TypewriterTimer += deltaTime;
		int charsToShow = (int)(m_TypewriterTimer * m_TypewriterSpeed);
		m_VisibleCharCount = std::min(charsToShow, (int)m_FullText.length());

		if (m_VisibleCharCount >= (int)m_FullText.length())
		{
			m_WaitingForInput = true;
		}
	}
}

void DialogueScreen::OnRender()
{
	if (ImGui::GetCurrentContext() == nullptr) return;
	if (!m_Active) return;

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	float boxW = displaySize.x * 0.8f;
	float boxH = m_BoxHeight;
	float boxX = (displaySize.x - boxW) * 0.5f;
	float boxY = displaySize.y - boxH - 20.0f;
	float portraitW = m_PortraitWidth;

	ImGuiWindowFlags winFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoFocusOnAppearing;

	ImGui::SetNextWindowPos(ImVec2(boxX, boxY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(boxW, boxH));

	if (ImGui::Begin("##DialogueBox", nullptr, winFlags))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 winPos = ImGui::GetWindowPos();

		// Dialogue box background
		drawList->AddRectFilled(winPos, ImVec2(winPos.x + boxW, winPos.y + boxH),
			IM_COL32(12, 8, 4, 240));
		drawList->AddRect(winPos, ImVec2(winPos.x + boxW, winPos.y + boxH),
			IM_COL32(200, 170, 80, 255), 0.0f, 0, 3.0f);

		// Portrait area
		float portraitAreaW = portraitW + 10.0f;
		drawList->AddRectFilled(
			ImVec2(winPos.x + 6, winPos.y + 6),
			ImVec2(winPos.x + portraitAreaW, winPos.y + boxH - 6),
			IM_COL32(8, 6, 3, 200)
		);
		drawList->AddRect(
			ImVec2(winPos.x + 6, winPos.y + 6),
			ImVec2(winPos.x + portraitAreaW, winPos.y + boxH - 6),
			IM_COL32(120, 100, 50, 200), 0.0f, 0, 1.5f
		);

		// Portrait text
		ImVec2 portTextPos(winPos.x + 10.0f, winPos.y + boxH * 0.5f - 10.0f);
		ImU32 portraitTextCol = IM_COL32(200, 180, 100, 200);
		if (!m_NpcName.empty())
			drawList->AddText(portTextPos, portraitTextCol, m_NpcName.c_str());

		// Speaker indicator
		ImVec2 indicatorPos(winPos.x + portraitAreaW + 10.0f, winPos.y + 10.0f);
		drawList->AddText(indicatorPos, IM_COL32(255, 240, 180, 255),
			(!m_NpcName.empty() ? m_NpcName.c_str() : "???"));

		// Dialogue text with typewriter effect
		float textX = winPos.x + portraitAreaW + 16.0f;
		float textY = winPos.y + 36.0f;
		float maxTextW = boxW - portraitAreaW - 30.0f;

		std::string visibleText = m_FullText.substr(0, m_VisibleCharCount);
		if (!m_WaitingForInput && m_VisibleCharCount < (int)m_FullText.length())
			visibleText += "|";

		ImFont* font = ImGui::GetFont();
		ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, visibleText.c_str(), nullptr, nullptr);

		float lineHeight = font->FontSize * 1.4f;
		float wrappedH = textSize.y + lineHeight;
		float textAreaY = textY;
		if (textAreaY + wrappedH > winPos.y + boxH - 10.0f)
			textAreaY = winPos.y + boxH - wrappedH - 10.0f;

		ImVec2 finalTextPos(textX, textAreaY);
		ImU32 textCol = m_WaitingForInput ? IM_COL32(255, 250, 220, 255) : IM_COL32(240, 230, 200, 255);
		drawList->AddText(font, font->FontSize, finalTextPos, textCol, visibleText.c_str(),
			nullptr, maxTextW);

		// Advance hint
		if (m_WaitingForInput)
		{
			float hintY = winPos.y + boxH - 28.0f;
			const char* hint = m_Responses.empty() ? "[Click or Space to continue]"
				: "[Click a response]";
			ImVec2 hintSize = ImGui::CalcTextSize(hint);
			float hintX = winPos.x + boxW - hintSize.x - 14.0f;
			float pulse = sinf((float)ImGui::GetTime() * 3.0f) * 0.3f + 0.7f;
			ImU32 hintCol = IM_COL32(200, 200, 200, (int)(200 * pulse));
			drawList->AddText(ImVec2(hintX, hintY), hintCol, hint);
		}
	}
	ImGui::End();

	// Responses panel
	if (m_WaitingForInput && !m_Responses.empty())
	{
		float respW = boxW * 0.4f;
		float respH = (float)m_Responses.size() * 30.0f + 10.0f;
		float respX = boxX + boxW - respW - 10.0f;
		float respY = boxY - respH - 5.0f;

		ImGui::SetNextWindowPos(ImVec2(respX, respY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(respW, respH));

		if (ImGui::Begin("##DialogueResponses", nullptr, winFlags))
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 wPos = ImGui::GetWindowPos();

			drawList->AddRectFilled(wPos, ImVec2(wPos.x + respW, wPos.y + respH),
				IM_COL32(10, 8, 4, 245));
			drawList->AddRect(wPos, ImVec2(wPos.x + respW, wPos.y + respH),
				IM_COL32(200, 170, 80, 255), 0.0f, 0, 2.0f);

			for (size_t i = 0; i < m_Responses.size(); ++i)
			{
				float rowY = wPos.y + 6.0f + i * 30.0f;
				ImGui::SetCursorPos(ImVec2(8.0f, 6.0f + i * 30.0f));

				char label[256];
				snprintf(label, sizeof(label), " %s ##resp%zu", m_Responses[i].c_str(), i);

				bool selected = (m_SelectedResponse == (int)i);
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 60, 20, 255));

				if (ImGui::Button(label, ImVec2(respW - 16.0f, 26.0f)))
				{
					m_SelectedResponse = (int)i;
					if (OnResponseSelected)
						OnResponseSelected((int)i);
					CloseDialogue();
				}

				if (selected)
					ImGui::PopStyleColor();
			}
		}
		ImGui::End();
	}
}

bool DialogueScreen::OnMouseClick(float x, float y, int32_t button, bool pressed)
{
	if (!m_Active) return false;
	if (!pressed) return false;

	if (m_WaitingForInput)
	{
		if (m_SelectedResponse < 0)
		{
			CloseDialogue();
		}
	}
	else
	{
		m_VisibleCharCount = (int)m_FullText.length();
		m_WaitingForInput = true;
	}
	return true;
}

bool DialogueScreen::OnKeyInput(int32_t key, bool pressed)
{
	if (!m_Active || !pressed) return false;

	if (key == 32 || key == 13) // Space or Enter
	{
		if (m_WaitingForInput)
			CloseDialogue();
		else
		{
			m_VisibleCharCount = (int)m_FullText.length();
			m_WaitingForInput = true;
		}
		return true;
	}
	return false;
}

void DialogueScreen::StartDialogue(const std::string& npcName, const std::string& portraitPath,
	const std::string& text, const std::vector<std::string>& responses)
{
	m_NpcName = npcName;
	m_PortraitPath = portraitPath;
	m_FullText = text;
	m_Responses = responses;
	m_SelectedResponse = -1;
	m_VisibleCharCount = 0;
	m_TypewriterTimer = 0.0f;
	m_WaitingForInput = false;
	m_Active = true;
	m_Complete = false;

	Show();
}

void DialogueScreen::AdvanceText()
{
	if (!m_WaitingForInput)
	{
		m_VisibleCharCount = (int)m_FullText.length();
		m_WaitingForInput = true;
	}
	else
	{
		CloseDialogue();
	}
}

void DialogueScreen::CloseDialogue()
{
	m_Complete = true;
	m_Active = false;
	Hide();
}

}
