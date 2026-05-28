#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <filesystem>

namespace Quentlam
{
	class ModManifest
	{
	public:
		ModManifest() = default;

		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		const std::string& GetAuthor() const { return m_Author; }
		void SetAuthor(const std::string& author) { m_Author = author; }

		const std::string& GetVersion() const { return m_Version; }
		void SetVersion(const std::string& version) { m_Version = version; }

		const std::string& GetDescription() const { return m_Description; }
		void SetDescription(const std::string& desc) { m_Description = desc; }

		const std::string& GetEntryPoint() const { return m_EntryPoint; }
		void SetEntryPoint(const std::string& entry) { m_EntryPoint = entry; }

		const std::vector<std::string>& GetDependencies() const { return m_Dependencies; }
		void AddDependency(const std::string& dep) { m_Dependencies.push_back(dep); }

		const std::vector<std::string>& GetContentPaths() const { return m_ContentPaths; }
		void AddContentPath(const std::string& path) { m_ContentPaths.push_back(path); }

		bool IsEnabled() const { return m_Enabled; }
		void SetEnabled(bool enabled) { m_Enabled = enabled; }

		const std::string& GetDirectory() const { return m_Directory; }
		void SetDirectory(const std::string& dir) { m_Directory = dir; }

		bool IsValid() const { return !m_Id.empty() && !m_Version.empty(); }

	private:
		std::string m_Id;
		std::string m_Name;
		std::string m_Author;
		std::string m_Version = "1.0.0";
		std::string m_Description;
		std::string m_EntryPoint;
		std::string m_Directory;
		std::vector<std::string> m_Dependencies;
		std::vector<std::string> m_ContentPaths;
		bool m_Enabled = false;
	};

	class ContentPack
	{
	public:
		ContentPack() = default;
		explicit ContentPack(const std::string& id);

		const std::string& GetId() const { return m_Id; }

		void AddAssetMapping(const std::string& virtualPath, const std::string& realPath);
		const std::string& GetRealPath(const std::string& virtualPath) const;
		bool HasAsset(const std::string& virtualPath) const;

		void AddOverride(const std::string& assetId, const std::string& replacementId);
		bool HasOverride(const std::string& assetId) const;
		const std::string& GetOverride(const std::string& assetId) const;

		const std::unordered_map<std::string, std::string>& GetAllMappings() const { return m_AssetMappings; }
		const std::unordered_map<std::string, std::string>& GetAllOverrides() const { return m_Overrides; }

	private:
		std::string m_Id;
		std::unordered_map<std::string, std::string> m_AssetMappings;
		std::unordered_map<std::string, std::string> m_Overrides;
	};

	inline ContentPack::ContentPack(const std::string& id)
		: m_Id(id) {}

	inline void ContentPack::AddAssetMapping(const std::string& virtualPath, const std::string& realPath)
	{
		m_AssetMappings[virtualPath] = realPath;
	}

	inline const std::string& ContentPack::GetRealPath(const std::string& virtualPath) const
	{
		static std::string empty;
		auto it = m_AssetMappings.find(virtualPath);
		return it != m_AssetMappings.end() ? it->second : empty;
	}

	inline bool ContentPack::HasAsset(const std::string& virtualPath) const
	{
		return m_AssetMappings.find(virtualPath) != m_AssetMappings.end();
	}

	inline void ContentPack::AddOverride(const std::string& assetId, const std::string& replacementId)
	{
		m_Overrides[assetId] = replacementId;
	}

	inline bool ContentPack::HasOverride(const std::string& assetId) const
	{
		return m_Overrides.find(assetId) != m_Overrides.end();
	}

	inline const std::string& ContentPack::GetOverride(const std::string& assetId) const
	{
		static std::string empty;
		auto it = m_Overrides.find(assetId);
		return it != m_Overrides.end() ? it->second : empty;
	}

	class ScriptBridge
	{
	public:
		ScriptBridge() = default;
		virtual ~ScriptBridge() = default;

		virtual bool LoadScript(const std::string& scriptPath) = 0;
		virtual bool CallFunction(const std::string& funcName, const std::vector<std::string>& args, std::string* result = nullptr) = 0;
		virtual bool HasFunction(const std::string& funcName) const = 0;
		virtual void UnloadScript() = 0;
		virtual bool IsValid() const = 0;
	};

	class ModLoader
	{
	public:
		ModLoader() = default;
		~ModLoader();

		bool Initialize(const std::string& modsDirectory);
		void Shutdown();

		bool LoadMod(const std::string& modDirectory);
		bool UnloadMod(const std::string& modId);
		bool EnableMod(const std::string& modId);
		bool DisableMod(const std::string& modId);

		bool ReloadMod(const std::string& modId);
		bool ReloadAllMods();

		const ModManifest* GetMod(const std::string& modId) const;
		const std::vector<const ModManifest*> GetAllMods() const;
		const std::vector<const ModManifest*> GetEnabledMods() const;
		const std::vector<const ModManifest*> GetDisabledMods() const;

		bool IsModLoaded(const std::string& modId) const;
		bool IsModEnabled(const std::string& modId) const;
		bool IsModLoadedAndEnabled(const std::string& modId) const;

		bool HasMod(const std::string& modId) const;
		bool HasModDependency(const std::string& modId, const std::string& dependencyId) const;
		std::vector<std::string> GetMissingDependencies(const std::string& modId) const;

		const std::string& GetModsDirectory() const { return m_ModsDirectory; }

		void SetOnModLoaded(std::function<void(const std::string& modId)> callback) { m_OnModLoaded = std::move(callback); }
		void SetOnModUnloaded(std::function<void(const std::string& modId)> callback) { m_OnModUnloaded = std::move(callback); }
		void SetOnModError(std::function<void(const std::string& modId, const std::string& error)> callback) { m_OnModError = std::move(callback); }

	private:
		bool ValidateDependencies(const ModManifest& manifest) const;
		void SortByLoadOrder();
		std::optional<ModManifest> LoadManifest(const std::string& modDir) const;
		std::optional<ModManifest> ParseManifestJson(const std::string& json, const std::string& modDir) const;
		bool ScanModsDirectory();

		std::string m_ModsDirectory;
		std::unordered_map<std::string, ModManifest> m_Mods;
		std::vector<std::string> m_LoadOrder;
		std::unordered_set<std::string> m_EnabledMods;
		std::unordered_map<std::string, std::unique_ptr<ScriptBridge>> m_ScriptBridges;

		std::function<void(const std::string&)> m_OnModLoaded;
		std::function<void(const std::string&)> m_OnModUnloaded;
		std::function<void(const std::string&, const std::string&)> m_OnModError;
	};

	class LoadOrderResolver
	{
	public:
		LoadOrderResolver() = default;

		bool Resolve(const std::vector<const ModManifest*>& mods, std::vector<std::string>* outLoadOrder) const;
		bool HasCycle(const std::vector<const ModManifest*>& mods) const;
		std::vector<std::string> GetLoadOrder(const std::vector<const ModManifest*>& mods) const;

	private:
		bool Visit(const std::string& modId, const std::vector<const ModManifest*>& mods,
			std::vector<std::string>* outOrder,
			std::unordered_set<std::string>* visiting,
			std::unordered_set<std::string>* visited) const;
	};

	inline ModLoader::~ModLoader()
	{
		Shutdown();
	}

	inline void ModLoader::Shutdown()
	{
		m_ScriptBridges.clear();
		m_Mods.clear();
		m_LoadOrder.clear();
		m_EnabledMods.clear();
	}

	inline bool ModLoader::LoadMod(const std::string& modDirectory)
	{
		auto manifestOpt = LoadManifest(modDirectory);
		if (!manifestOpt.has_value())
			return false;

		const ModManifest& manifest = *manifestOpt;
		if (HasMod(manifest.GetId()))
			return false;

		m_Mods[manifest.GetId()] = manifest;
		SortByLoadOrder();
		return true;
	}

	inline bool ModLoader::UnloadMod(const std::string& modId)
	{
		if (!HasMod(modId))
			return false;

		m_ScriptBridges.erase(modId);
		m_EnabledMods.erase(modId);
		m_Mods.erase(modId);
		SortByLoadOrder();
		if (m_OnModUnloaded)
			m_OnModUnloaded(modId);
		return true;
	}

	inline bool ModLoader::EnableMod(const std::string& modId)
	{
		if (!HasMod(modId))
			return false;

		auto missing = GetMissingDependencies(modId);
		if (!missing.empty())
		{
			if (m_OnModError)
			{
				std::string err = "Missing dependencies: ";
				for (const auto& d : missing) err += d + " ";
				m_OnModError(modId, err);
			}
			return false;
		}

		m_EnabledMods.insert(modId);
		SortByLoadOrder();
		if (m_OnModLoaded)
			m_OnModLoaded(modId);
		return true;
	}

	inline bool ModLoader::DisableMod(const std::string& modId)
	{
		if (!HasMod(modId))
			return false;

		for (const auto& [id, mod] : m_Mods)
		{
			for (const auto& dep : mod.GetDependencies())
				if (dep == modId)
					DisableMod(id);
		}

		m_EnabledMods.erase(modId);
		m_ScriptBridges.erase(modId);
		SortByLoadOrder();
		if (m_OnModUnloaded)
			m_OnModUnloaded(modId);
		return true;
	}

	inline bool ModLoader::ReloadMod(const std::string& modId)
	{
		if (!HasMod(modId))
			return false;

		const auto& manifest = *GetMod(modId);
		std::string dir = manifest.GetDirectory();
		bool wasEnabled = IsModEnabled(modId);
		UnloadMod(modId);
		return LoadMod(dir) && (!wasEnabled || EnableMod(modId));
	}

	inline bool ModLoader::ReloadAllMods()
	{
		std::vector<std::string> dirs;
		for (const auto& [id, mod] : m_Mods)
			dirs.push_back(mod.GetDirectory());
		dirs.push_back(m_ModsDirectory);

		for (const auto& dir : dirs)
			if (!ReloadMod(std::filesystem::path(dir).filename().string()))
				return false;
		return true;
	}

	inline const ModManifest* ModLoader::GetMod(const std::string& modId) const
	{
		auto it = m_Mods.find(modId);
		return it != m_Mods.end() ? &it->second : nullptr;
	}

	inline const std::vector<const ModManifest*> ModLoader::GetAllMods() const
	{
		std::vector<const ModManifest*> result;
		for (const auto& [id, mod] : m_Mods)
			result.push_back(&mod);
		return result;
	}

	inline const std::vector<const ModManifest*> ModLoader::GetEnabledMods() const
	{
		std::vector<const ModManifest*> result;
		for (const auto& id : m_LoadOrder)
			if (m_EnabledMods.find(id) != m_EnabledMods.end())
				if (auto it = m_Mods.find(id); it != m_Mods.end())
					result.push_back(&it->second);
		return result;
	}

	inline const std::vector<const ModManifest*> ModLoader::GetDisabledMods() const
	{
		std::vector<const ModManifest*> result;
		for (const auto& [id, mod] : m_Mods)
			if (m_EnabledMods.find(id) == m_EnabledMods.end())
				result.push_back(&mod);
		return result;
	}

	inline bool ModLoader::IsModLoaded(const std::string& modId) const
	{
		return m_Mods.find(modId) != m_Mods.end();
	}

	inline bool ModLoader::IsModEnabled(const std::string& modId) const
	{
		return m_EnabledMods.find(modId) != m_EnabledMods.end();
	}

	inline bool ModLoader::IsModLoadedAndEnabled(const std::string& modId) const
	{
		return IsModLoaded(modId) && IsModEnabled(modId);
	}

	inline bool ModLoader::HasMod(const std::string& modId) const
	{
		return m_Mods.find(modId) != m_Mods.end();
	}

	inline bool ModLoader::HasModDependency(const std::string& modId, const std::string& dependencyId) const
	{
		auto* mod = GetMod(modId);
		if (!mod) return false;
		for (const auto& dep : mod->GetDependencies())
			if (dep == dependencyId) return true;
		return false;
	}

	inline std::vector<std::string> ModLoader::GetMissingDependencies(const std::string& modId) const
	{
		std::vector<std::string> missing;
		auto* mod = GetMod(modId);
		if (!mod) return missing;

		for (const auto& dep : mod->GetDependencies())
			if (!IsModLoaded(dep))
				missing.push_back(dep);
		return missing;
	}

	inline bool ModLoader::ValidateDependencies(const ModManifest& manifest) const
	{
		for (const auto& dep : manifest.GetDependencies())
			if (!IsModLoaded(dep))
				return false;
		return true;
	}

	inline void ModLoader::SortByLoadOrder()
	{
		m_LoadOrder.clear();
		LoadOrderResolver resolver;
		std::vector<const ModManifest*> mods;
		for (const auto& [id, mod] : m_Mods)
			mods.push_back(&mod);
		if (resolver.Resolve(mods, &m_LoadOrder))
		{
		}
		else
		{
			for (const auto& [id, mod] : m_Mods)
				m_LoadOrder.push_back(id);
		}
	}

	inline bool LoadOrderResolver::Resolve(const std::vector<const ModManifest*>& mods, std::vector<std::string>* outLoadOrder) const
	{
		if (HasCycle(mods))
			return false;

		outLoadOrder->clear();
		std::unordered_set<std::string> visited;
		std::unordered_set<std::string> visiting;

		for (const auto* mod : mods)
		{
			if (visited.find(mod->GetId()) == visited.end())
			{
				if (!Visit(mod->GetId(), mods, outLoadOrder, &visiting, &visited))
					return false;
			}
		}
		return true;
	}

	inline bool LoadOrderResolver::HasCycle(const std::vector<const ModManifest*>& mods) const
	{
		std::unordered_set<std::string> visited;
		std::unordered_set<std::string> visiting;
		for (const auto* mod : mods)
		{
			if (visited.find(mod->GetId()) == visited.end())
				if (!Visit(mod->GetId(), mods, nullptr, &visiting, &visited))
					return true;
		}
		return false;
	}

	inline std::vector<std::string> LoadOrderResolver::GetLoadOrder(const std::vector<const ModManifest*>& mods) const
	{
		std::vector<std::string> order;
		Resolve(mods, &order);
		return order;
	}

	inline bool LoadOrderResolver::Visit(const std::string& modId, const std::vector<const ModManifest*>& mods,
		std::vector<std::string>* outOrder,
		std::unordered_set<std::string>* visiting,
		std::unordered_set<std::string>* visited) const
	{
		if (visiting->find(modId) != visiting->end())
			return false;
		if (visited->find(modId) != visited->end())
			return true;

		visiting->insert(modId);

		for (const auto* mod : mods)
		{
			if (mod->GetId() == modId)
			{
				for (const auto& dep : mod->GetDependencies())
				{
					if (!Visit(dep, mods, outOrder, visiting, visited))
						return false;
				}
				break;
			}
		}

		visiting->erase(modId);
		visited->insert(modId);
		if (outOrder)
			outOrder->push_back(modId);
		return true;
	}

	class ModdingModule
	{
	public:
		ModdingModule() = default;
		static ModdingModule& Get();

		bool Initialize(const std::string& modsDirectory);
		void Shutdown();

		ModLoader& GetModLoader() { return m_ModLoader; }
		const ModLoader& GetModLoader() const { return m_ModLoader; }

		ContentPack* CreateContentPack(const std::string& id);
		void RemoveContentPack(const std::string& id);
		ContentPack* GetContentPack(const std::string& id);
		const std::vector<ContentPack>& GetAllContentPacks() const { return m_ContentPacks; }

		bool ResolveAsset(const std::string& assetId, std::string* outResolvedId) const;
		bool ResolveAssetPath(const std::string& virtualPath, std::string* outRealPath) const;

		void SetOnContentPackLoaded(std::function<void(const std::string& packId)> callback) { m_OnContentPackLoaded = std::move(callback); }

	private:
		ModLoader m_ModLoader;
		std::vector<ContentPack> m_ContentPacks;
		std::unordered_map<std::string, size_t> m_ContentPackIndex;

		std::function<void(const std::string&)> m_OnContentPackLoaded;
	};

	inline ModdingModule& ModdingModule::Get()
	{
		static ModdingModule instance;
		return instance;
	}

	inline bool ModdingModule::Initialize(const std::string& modsDirectory)
	{
		return m_ModLoader.Initialize(modsDirectory);
	}

	inline void ModdingModule::Shutdown()
	{
		m_ModLoader.Shutdown();
		m_ContentPacks.clear();
		m_ContentPackIndex.clear();
	}

	inline ContentPack* ModdingModule::CreateContentPack(const std::string& id)
	{
		if (m_ContentPackIndex.find(id) != m_ContentPackIndex.end())
			return nullptr;

		m_ContentPacks.push_back(ContentPack(id));
		m_ContentPackIndex[id] = m_ContentPacks.size() - 1;
		if (m_OnContentPackLoaded)
			m_OnContentPackLoaded(id);
		return &m_ContentPacks.back();
	}

	inline void ModdingModule::RemoveContentPack(const std::string& id)
	{
		auto it = m_ContentPackIndex.find(id);
		if (it == m_ContentPackIndex.end())
			return;

		size_t index = it->second;
		m_ContentPacks.erase(m_ContentPacks.begin() + index);
		m_ContentPackIndex.erase(it);

		for (size_t i = index; i < m_ContentPacks.size(); ++i)
			m_ContentPackIndex[m_ContentPacks[i].GetId()] = i;
	}

	inline ContentPack* ModdingModule::GetContentPack(const std::string& id)
	{
		auto it = m_ContentPackIndex.find(id);
		return it != m_ContentPackIndex.end() ? &m_ContentPacks[it->second] : nullptr;
	}

	inline bool ModdingModule::ResolveAsset(const std::string& assetId, std::string* outResolvedId) const
	{
		std::string resolved = assetId;

		for (const auto& pack : m_ContentPacks)
		{
			if (pack.HasOverride(assetId))
			{
				resolved = pack.GetOverride(assetId);
				break;
			}
		}

		if (outResolvedId)
			*outResolvedId = resolved;
		return resolved != assetId;
	}

	inline bool ModdingModule::ResolveAssetPath(const std::string& virtualPath, std::string* outRealPath) const
	{
		std::string resolved;

		for (const auto& pack : m_ContentPacks)
		{
			if (pack.HasAsset(virtualPath))
			{
				resolved = pack.GetRealPath(virtualPath);
				break;
			}
		}

		if (resolved.empty())
			resolved = virtualPath;

		if (outRealPath)
			*outRealPath = resolved;
		return !resolved.empty();
	}
}
