#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Entity.h"
#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <variant>
#include <cstdio>

namespace Quentlam
{
	enum class EItemCategory : uint8_t
	{
		None = 0,
		Tool = 1,
		Seed = 2,
		Crop = 3,
		Material = 4,
		Food = 5,
		Clothing = 6,
		Furniture = 7,
		Quest = 8,
		BigCraftable = 9,
		Fish = 10,
		Mineral = 11,
		MonsterLoot = 12,
		Artifact = 13,
		Custom = 100
	};

	enum class EItemType : uint8_t
	{
		None = 0,
		Tool = 1,
		Seeds = 2,
		Harvest = 3,
		Ingredients = 4,
		Cooking = 5,
		Consumable = 6,
		Rings = 7,
		Weapon = 8,
		Clothing = 9,
		Boots = 10,
		Hat = 11,
		Furniture = 12,
		Decoration = 13,
		Lighting = 14,
		Custom = 100
	};

	enum class EItemQuality : uint8_t
	{
		Normal = 0,
		Silver = 1,
		Gold = 2,
		Iridium = 3
	};

	enum class EItemFlag : uint32_t
	{
		None            = 0,
		Infinite        = BIT(0),
		Tradable        = BIT(1),
		Edible          = BIT(2),
		FPlaceable      = BIT(3),
		IsGift          = BIT(4),
		CanBeGiant      = BIT(5),
		CanBeRainbow    = BIT(6),
		HasBeenGifted   = BIT(7),
		Found           = BIT(8),
		SpecialItem     = BIT(9),
		Disappears      = BIT(10)
	};

	struct ItemDef
	{
		std::string Id;
		std::string Name;
		std::string Description;
		EItemCategory Category = EItemCategory::None;
		EItemType Type = EItemType::None;
		int32_t BaseValue = 0;
		int32_t BaseStock = 1;
		float Weight = 1.0f;
		bool IsEdible = false;
		int32_t EdibleHealth = 0;
		int32_t EdibleEnergy = 0;
		float EdibleStamina = 0.0f;
		EItemQuality MaxQuality = EItemQuality::Normal;
		std::string SpritePath;
		int32_t SpriteIndex = 0;
		std::string AnimationName;
		std::string CustomData;
	};

	struct ItemStack
	{
		std::string ItemId;
		int32_t StackSize = 0;
		int32_t Quality = 0;
		int32_t Price = 0;
		float Durability = -1.0f;
		float UsesLeft = -1.0f;
		std::string Metadata;
		std::string InstanceId;

		bool IsValid() const { return !ItemId.empty() && StackSize > 0; }
		bool IsFull(int32_t maxStack) const { return StackSize >= maxStack; }
		bool IsEmpty() const { return StackSize <= 0 || ItemId.empty(); }
	};

	struct InventorySlot
	{
		int32_t SlotIndex = -1;
		ItemStack Stack;
		int32_t MaxStack = 999;
		bool IsLocked = false;
		bool IsSelected = false;

		bool IsEmpty() const { return !Stack.IsValid(); }
		bool HasItem() const { return Stack.IsValid(); }
	};

	class ItemDefLibrary
	{
	public:
		static ItemDefLibrary& Get();

		void RegisterDef(const ItemDef& def);
		void UnregisterDef(const std::string& itemId);
		const ItemDef* GetDef(const std::string& itemId) const;
		bool HasDef(const std::string& itemId) const;
		const std::unordered_map<std::string, ItemDef>& GetAllDefs() const { return m_Defs; }

		std::vector<const ItemDef*> FindByCategory(EItemCategory category) const;
		std::vector<const ItemDef*> FindByType(EItemType type) const;
		std::vector<const ItemDef*> FindByName(const std::string& search) const;

		bool LoadFromFile(const std::string& filepath);
		void Clear() { m_Defs.clear(); }

	private:
		ItemDef ParseItemDef(const rapidjson::Value& json) const;

		ItemDefLibrary() = default;
		std::unordered_map<std::string, ItemDef> m_Defs;
	};

	class Container
	{
	public:
		Container() = default;
		Container(const std::string& id, int32_t slotCount, int32_t maxStack = 999);
		explicit Container(const std::string& id);

		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }
		int32_t GetSlotCount() const { return static_cast<int32_t>(m_Slots.size()); }
		int32_t GetMaxStack() const { return m_MaxStack; }
		void SetMaxStack(int32_t max) { m_MaxStack = max; }

		bool AddItem(const ItemStack& stack);
		bool AddItemAt(int32_t slotIndex, const ItemStack& stack);
		ItemStack RemoveItem(int32_t slotIndex, int32_t count = -1);
		ItemStack RemoveItemById(const std::string& itemId, int32_t count = -1);
		bool SwapSlots(int32_t from, int32_t to);
		bool MoveItem(int32_t from, int32_t to);
		void Clear();

		InventorySlot* GetSlot(int32_t index);
		const InventorySlot* GetSlot(int32_t index) const;
		InventorySlot* GetSlotByItemId(const std::string& itemId);
		const InventorySlot* GetSlotByItemId(const std::string& itemId) const;

		int32_t CountItem(const std::string& itemId) const;
		int32_t CountFreeSlots() const;
		bool HasItem(const std::string& itemId, int32_t minCount = 1) const;
		bool IsFull() const;

		std::vector<InventorySlot>& GetAllSlots() { return m_Slots; }
		const std::vector<InventorySlot>& GetAllSlots() const { return m_Slots; }

		bool IsPlayerInventory() const { return m_IsPlayerInventory; }
		void SetPlayerInventory(bool isPlayer) { m_IsPlayerInventory = isPlayer; }

	private:
		int32_t FindSlotForItem(const std::string& itemId) const;
		bool CanAddItem(const ItemStack& stack) const;

		std::string m_Id;
		std::vector<InventorySlot> m_Slots;
		int32_t m_MaxStack = 999;
		bool m_IsPlayerInventory = false;
	};

	struct RecipeIngredient
	{
		std::string ItemId;
		int32_t Count = 1;
	};

	struct RecipeOutput
	{
		std::string ItemId;
		int32_t Count = 1;
		int32_t Quality = 0;
	};

	class Recipe
	{
	public:
		Recipe() = default;
		Recipe(const std::string& id, const std::string& name);

		const std::string& GetId() const { return m_Id; }
		const std::string& GetDisplayName() const { return m_DisplayName; }
		void SetDisplayName(const std::string& name) { m_DisplayName = name; }

		void AddIngredient(const std::string& itemId, int32_t count = 1);
		void SetOutput(const std::string& itemId, int32_t count = 1, int32_t quality = 0);
		void SetCraftingTime(float seconds) { m_CraftingTime = seconds; }

		const std::vector<RecipeIngredient>& GetIngredients() const { return m_Ingredients; }
		const RecipeOutput& GetOutput() const { return m_Output; }
		float GetCraftingTime() const { return m_CraftingTime; }

		bool CanCraft(const Container* inventory) const;
		bool Craft(Container* inventory) const;
		int32_t MaxCraftable(const Container* inventory) const;

		bool IsBigCraftable() const { return m_IsBigCraftable; }
		void SetBigCraftable(bool big) { m_IsBigCraftable = big; }

	private:
		std::string m_Id;
		std::string m_DisplayName;
		std::vector<RecipeIngredient> m_Ingredients;
		RecipeOutput m_Output;
		float m_CraftingTime = 0.0f;
		bool m_IsBigCraftable = false;
	};

	struct ShopStock
	{
		std::string ItemId;
		int32_t Stock = -1;
		int32_t Price = 0;
		bool Unlimited = false;
		std::string AvailableDays;
		std::string AvailableWeather;
		int32_t AvailableFromYear = 1;
		int32_t AvailableFromSeasonIndex = 0;
	};

	class Shop
	{
	public:
		Shop() = default;
		explicit Shop(const std::string& ownerId);

		const std::string& GetOwnerId() const { return m_OwnerId; }
		void SetOwnerId(const std::string& id) { m_OwnerId = id; }

		void AddStock(const ShopStock& stock);
		void RemoveStock(const std::string& itemId);
		const ShopStock* GetStock(const std::string& itemId) const;
		const std::vector<ShopStock>& GetAllStock() const { return m_Stock; }

		std::vector<const ShopStock*> GetAvailableStock() const;

		bool BuyItem(Container* buyerInventory, const std::string& itemId, int32_t count = 1, int32_t playerMoney = 0);

		void SetPortraitPath(const std::string& path) { m_PortraitPath = path; }
		const std::string& GetPortraitPath() const { return m_PortraitPath; }

	private:
		std::string m_OwnerId;
		std::vector<ShopStock> m_Stock;
		std::string m_PortraitPath;
	};

	inline ItemDefLibrary& ItemDefLibrary::Get()
	{
		static ItemDefLibrary instance;
		return instance;
	}

	inline void ItemDefLibrary::RegisterDef(const ItemDef& def)
	{
		m_Defs[def.Id] = def;
	}

	inline void ItemDefLibrary::UnregisterDef(const std::string& itemId)
	{
		m_Defs.erase(itemId);
	}

	inline const ItemDef* ItemDefLibrary::GetDef(const std::string& itemId) const
	{
		auto it = m_Defs.find(itemId);
		return it != m_Defs.end() ? &it->second : nullptr;
	}

	inline bool ItemDefLibrary::HasDef(const std::string& itemId) const
	{
		return m_Defs.find(itemId) != m_Defs.end();
	}

	inline std::vector<const ItemDef*> ItemDefLibrary::FindByCategory(EItemCategory category) const
	{
		std::vector<const ItemDef*> results;
		for (const auto& [id, def] : m_Defs)
			if (def.Category == category) results.push_back(&def);
		return results;
	}

	inline std::vector<const ItemDef*> ItemDefLibrary::FindByType(EItemType type) const
	{
		std::vector<const ItemDef*> results;
		for (const auto& [id, def] : m_Defs)
			if (def.Type == type) results.push_back(&def);
		return results;
	}

	inline std::vector<const ItemDef*> ItemDefLibrary::FindByName(const std::string& search) const
	{
		std::vector<const ItemDef*> results;
		for (const auto& [id, def] : m_Defs)
			if (def.Name.find(search) != std::string::npos) results.push_back(&def);
		return results;
	}

	inline ItemDef ItemDefLibrary::ParseItemDef(const rapidjson::Value& json) const
	{
		ItemDef def;
		def.Id = json.HasMember("Id") ? json["Id"].GetString() : "";
		def.Name = json.HasMember("Name") ? json["Name"].GetString() : def.Id;
		def.Description = json.HasMember("Description") ? json["Description"].GetString() : "";

		if (json.HasMember("Category"))
			def.Category = static_cast<EItemCategory>(json["Category"].GetInt());
		if (json.HasMember("Type"))
			def.Type = static_cast<EItemType>(json["Type"].GetInt());
		if (json.HasMember("BaseValue"))
			def.BaseValue = json["BaseValue"].GetInt();
		if (json.HasMember("BaseStock"))
			def.BaseStock = json["BaseStock"].GetInt();
		if (json.HasMember("Weight"))
			def.Weight = static_cast<float>(json["Weight"].GetDouble());
		if (json.HasMember("IsEdible"))
			def.IsEdible = json["IsEdible"].GetBool();
		if (json.HasMember("EdibleHealth"))
			def.EdibleHealth = json["EdibleHealth"].GetInt();
		if (json.HasMember("EdibleEnergy"))
			def.EdibleEnergy = json["EdibleEnergy"].GetInt();
		if (json.HasMember("EdibleStamina"))
			def.EdibleStamina = static_cast<float>(json["EdibleStamina"].GetDouble());
		if (json.HasMember("MaxQuality"))
			def.MaxQuality = static_cast<EItemQuality>(json["MaxQuality"].GetInt());
		if (json.HasMember("SpritePath"))
			def.SpritePath = json["SpritePath"].GetString();
		if (json.HasMember("SpriteIndex"))
			def.SpriteIndex = json["SpriteIndex"].GetInt();
		if (json.HasMember("AnimationName"))
			def.AnimationName = json["AnimationName"].GetString();
		if (json.HasMember("CustomData"))
			def.CustomData = json["CustomData"].GetString();
		return def;
	}

	inline bool ItemDefLibrary::LoadFromFile(const std::string& filepath)
	{
		FILE* fp;
		fopen_s(&fp, filepath.c_str(), "rb");
		if (!fp)
		{
			QL_CORE_ERROR("ItemDefLibrary: Failed to open file '{0}'", filepath);
			return false;
		}

		fseek(fp, 0, SEEK_END);
		long fsize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		std::vector<char> buffer(static_cast<size_t>(fsize) + 1);
		fread(buffer.data(), 1, static_cast<size_t>(fsize), fp);
		buffer[fsize] = '\0';
		fclose(fp);

		rapidjson::Document doc;
		doc.Parse(buffer.data(), buffer.size() - 1);
		if (doc.HasParseError())
		{
			QL_CORE_ERROR("ItemDefLibrary: JSON parse error at offset {0}", doc.GetErrorOffset());
			return false;
		}

		if (!doc.IsObject() || !doc.HasMember("items"))
		{
			QL_CORE_ERROR("ItemDefLibrary: Invalid JSON format - expected object with 'items' array");
			return false;
		}

		const rapidjson::Value& items = doc["items"];
		if (!items.IsArray())
		{
			QL_CORE_ERROR("ItemDefLibrary: 'items' must be an array");
			return false;
		}

		int loaded = 0;
		for (rapidjson::SizeType i = 0; i < items.Size(); ++i)
		{
			const rapidjson::Value& itemJson = items[i];
			if (!itemJson.IsObject())
				continue;
			if (!itemJson.HasMember("Id") || !itemJson["Id"].IsString())
				continue;

			ItemDef def = ParseItemDef(itemJson);
			RegisterDef(def);
			loaded++;
		}

		QL_CORE_INFO("ItemDefLibrary: Loaded {0} items from '{1}'", loaded, filepath);
		return true;
	}

	inline Container::Container(const std::string& id, int32_t slotCount, int32_t maxStack)
		: m_Id(id), m_MaxStack(maxStack)
	{
		m_Slots.resize(slotCount);
		for (int32_t i = 0; i < slotCount; ++i)
			m_Slots[i].SlotIndex = i;
	}

	inline Container::Container(const std::string& id)
		: m_Id(id) {}

	inline int32_t Container::FindSlotForItem(const std::string& itemId) const
	{
		for (int32_t i = 0; i < static_cast<int32_t>(m_Slots.size()); ++i)
		{
			const auto& slot = m_Slots[i];
			if (!slot.IsEmpty() && slot.Stack.ItemId == itemId && !slot.IsLocked)
				if (slot.Stack.StackSize < slot.MaxStack)
					return i;
		}
		return -1;
	}

	inline bool Container::CanAddItem(const ItemStack& stack) const
	{
		if (!stack.IsValid()) return false;
		int32_t remaining = stack.StackSize;
		for (const auto& slot : m_Slots)
		{
			if (slot.IsLocked) continue;
			if (slot.IsEmpty()) return true;
			if (slot.Stack.ItemId == stack.ItemId)
				remaining -= (slot.MaxStack - slot.Stack.StackSize);
		}
		return remaining <= 0;
	}

	inline bool Container::AddItem(const ItemStack& stack)
	{
		if (!stack.IsValid()) return false;

		ItemStack toAdd = stack;
		int32_t firstEmpty = -1;

		auto addToSlot = [&](int32_t slotIndex) -> bool {
			if (slotIndex < 0 || slotIndex >= static_cast<int32_t>(m_Slots.size())) return false;
			auto& slot = m_Slots[slotIndex];
			if (slot.IsLocked) return false;

			if (slot.IsEmpty())
			{
				if (firstEmpty == -1) firstEmpty = slotIndex;
				int32_t amount = std::min(toAdd.StackSize, slot.MaxStack);
				slot.Stack = toAdd;
				slot.Stack.StackSize = amount;
				toAdd.StackSize -= amount;
				if (toAdd.StackSize <= 0) return true;
			}
			else if (slot.Stack.ItemId == toAdd.ItemId)
			{
				int32_t amount = std::min(toAdd.StackSize, slot.MaxStack - slot.Stack.StackSize);
				slot.Stack.StackSize += amount;
				toAdd.StackSize -= amount;
				if (toAdd.StackSize <= 0) return true;
			}
			return false;
		};

		int32_t existingSlot = FindSlotForItem(toAdd.ItemId);
		if (existingSlot >= 0)
			if (addToSlot(existingSlot)) return true;

		while (firstEmpty >= 0)
		{
			if (addToSlot(firstEmpty)) return true;
			firstEmpty = -1;
			for (int32_t i = 0; i < static_cast<int32_t>(m_Slots.size()); ++i)
				if (m_Slots[i].IsEmpty() && !m_Slots[i].IsLocked)
				{
					firstEmpty = i;
					break;
				}
		}

		return toAdd.StackSize <= 0;
	}

	inline bool Container::AddItemAt(int32_t slotIndex, const ItemStack& stack)
	{
		if (slotIndex < 0 || slotIndex >= static_cast<int32_t>(m_Slots.size())) return false;
		auto& slot = m_Slots[slotIndex];
		if (slot.IsLocked) return false;

		if (slot.IsEmpty())
		{
			slot.Stack = stack;
			slot.Stack.StackSize = std::min(stack.StackSize, slot.MaxStack);
			return stack.StackSize <= slot.MaxStack;
		}
		else if (slot.Stack.ItemId == stack.ItemId)
		{
			int32_t amount = std::min(stack.StackSize, slot.MaxStack - slot.Stack.StackSize);
			slot.Stack.StackSize += amount;
			return amount >= stack.StackSize;
		}
		return false;
	}

	inline ItemStack Container::RemoveItem(int32_t slotIndex, int32_t count)
	{
		if (slotIndex < 0 || slotIndex >= static_cast<int32_t>(m_Slots.size())) return ItemStack{};
		auto& slot = m_Slots[slotIndex];
		if (slot.IsEmpty()) return ItemStack{};

		ItemStack removed = slot.Stack;
		if (count > 0 && count < slot.Stack.StackSize)
		{
			removed.StackSize = count;
			slot.Stack.StackSize -= count;
		}
		else
		{
			slot.Stack = ItemStack{};
		}
		return removed;
	}

	inline ItemStack Container::RemoveItemById(const std::string& itemId, int32_t count)
	{
		ItemStack totalRemoved{};
		int32_t toRemove = (count > 0) ? count : INT32_MAX;

		for (auto& slot : m_Slots)
		{
			if (!slot.IsEmpty() && slot.Stack.ItemId == itemId)
			{
				ItemStack removed = RemoveItem(slot.SlotIndex, toRemove);
				totalRemoved.StackSize += removed.StackSize;
				totalRemoved.ItemId = removed.ItemId;
				toRemove -= removed.StackSize;
				if (toRemove <= 0) break;
			}
		}
		return totalRemoved;
	}

	inline bool Container::SwapSlots(int32_t from, int32_t to)
	{
		if (from < 0 || from >= static_cast<int32_t>(m_Slots.size())) return false;
		if (to < 0 || to >= static_cast<int32_t>(m_Slots.size())) return false;
		if (from == to) return true;

		auto temp = m_Slots[from];
		m_Slots[from] = m_Slots[to];
		m_Slots[to] = temp;

		m_Slots[from].SlotIndex = from;
		m_Slots[to].SlotIndex = to;
		return true;
	}

	inline bool Container::MoveItem(int32_t from, int32_t to)
	{
		if (from < 0 || from >= static_cast<int32_t>(m_Slots.size())) return false;
		if (to < 0 || to >= static_cast<int32_t>(m_Slots.size())) return false;
		if (from == to) return true;

		auto& src = m_Slots[from];
		auto& dst = m_Slots[to];

		if (dst.IsEmpty())
		{
			dst = src;
			src = InventorySlot{};
			dst.SlotIndex = to;
			src.SlotIndex = from;
			return true;
		}
		else if (dst.Stack.ItemId == src.Stack.ItemId)
		{
			int32_t amount = std::min(src.Stack.StackSize, dst.MaxStack - dst.Stack.StackSize);
			dst.Stack.StackSize += amount;
			src.Stack.StackSize -= amount;
			if (src.Stack.StackSize <= 0) src = InventorySlot{};
			return true;
		}
		else
		{
			return SwapSlots(from, to);
		}
	}

	inline void Container::Clear()
	{
		for (auto& slot : m_Slots)
			slot.Stack = ItemStack{};
	}

	inline InventorySlot* Container::GetSlot(int32_t index)
	{
		if (index < 0 || index >= static_cast<int32_t>(m_Slots.size())) return nullptr;
		return &m_Slots[index];
	}

	inline const InventorySlot* Container::GetSlot(int32_t index) const
	{
		if (index < 0 || index >= static_cast<int32_t>(m_Slots.size())) return nullptr;
		return &m_Slots[index];
	}

	inline InventorySlot* Container::GetSlotByItemId(const std::string& itemId)
	{
		for (auto& slot : m_Slots)
			if (!slot.IsEmpty() && slot.Stack.ItemId == itemId) return &slot;
		return nullptr;
	}

	inline const InventorySlot* Container::GetSlotByItemId(const std::string& itemId) const
	{
		for (const auto& slot : m_Slots)
			if (!slot.IsEmpty() && slot.Stack.ItemId == itemId) return &slot;
		return nullptr;
	}

	inline int32_t Container::CountItem(const std::string& itemId) const
	{
		int32_t total = 0;
		for (const auto& slot : m_Slots)
			if (!slot.IsEmpty() && slot.Stack.ItemId == itemId)
				total += slot.Stack.StackSize;
		return total;
	}

	inline int32_t Container::CountFreeSlots() const
	{
		int32_t count = 0;
		for (const auto& slot : m_Slots)
			if (slot.IsEmpty() && !slot.IsLocked) count++;
		return count;
	}

	inline bool Container::HasItem(const std::string& itemId, int32_t minCount) const
	{
		return CountItem(itemId) >= minCount;
	}

	inline bool Container::IsFull() const
	{
		return CountFreeSlots() <= 0;
	}

	inline Recipe::Recipe(const std::string& id, const std::string& name)
		: m_Id(id), m_DisplayName(name) {}

	inline void Recipe::AddIngredient(const std::string& itemId, int32_t count)
	{
		m_Ingredients.push_back({ itemId, count });
	}

	inline void Recipe::SetOutput(const std::string& itemId, int32_t count, int32_t quality)
	{
		m_Output.ItemId = itemId;
		m_Output.Count = count;
		m_Output.Quality = quality;
	}

	inline bool Recipe::CanCraft(const Container* inventory) const
	{
		if (!inventory) return false;
		for (const auto& ing : m_Ingredients)
			if (!inventory->HasItem(ing.ItemId, ing.Count)) return false;
		return true;
	}

	inline bool Recipe::Craft(Container* inventory) const
	{
		if (!CanCraft(inventory)) return false;
		for (const auto& ing : m_Ingredients)
			inventory->RemoveItemById(ing.ItemId, ing.Count);

		ItemStack output;
		output.ItemId = m_Output.ItemId;
		output.StackSize = m_Output.Count;
		output.Quality = m_Output.Quality;
		inventory->AddItem(output);
		return true;
	}

	inline int32_t Recipe::MaxCraftable(const Container* inventory) const
	{
		if (!inventory) return 0;
		int32_t max = INT32_MAX;
		for (const auto& ing : m_Ingredients)
		{
			int32_t have = inventory->CountItem(ing.ItemId);
			max = std::min(max, have / ing.Count);
		}
		return max;
	}

	inline Shop::Shop(const std::string& ownerId)
		: m_OwnerId(ownerId) {}

	inline void Shop::AddStock(const ShopStock& stock)
	{
		m_Stock.push_back(stock);
	}

	inline void Shop::RemoveStock(const std::string& itemId)
	{
		m_Stock.erase(
			std::remove_if(m_Stock.begin(), m_Stock.end(),
				[&itemId](const ShopStock& s) { return s.ItemId == itemId; }),
			m_Stock.end()
		);
	}

	inline const ShopStock* Shop::GetStock(const std::string& itemId) const
	{
		for (const auto& s : m_Stock)
			if (s.ItemId == itemId) return &s;
		return nullptr;
	}

	inline std::vector<const ShopStock*> Shop::GetAvailableStock() const
	{
		std::vector<const ShopStock*> available;
		for (const auto& s : m_Stock)
		{
			if (s.Unlimited || s.Stock > 0)
				available.push_back(&s);
		}
		return available;
	}

	inline bool Shop::BuyItem(Container* buyerInventory, const std::string& itemId, int32_t count, int32_t playerMoney)
	{
		const ShopStock* stock = GetStock(itemId);
		if (!stock) return false;

		int32_t price = stock->Price * count;
		if (playerMoney < price) return false;

		ItemStack item;
		item.ItemId = itemId;
		item.StackSize = count;
		item.Price = stock->Price;

		if (!buyerInventory->AddItem(item)) return false;
		return true;
	}
}
