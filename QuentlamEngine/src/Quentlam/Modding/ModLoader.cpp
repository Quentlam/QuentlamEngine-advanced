#include "qlpch.h"
#include "Quentlam/Core/Log.h"
#include <string>
#include <vector>
#include <unordered_set>

namespace Quentlam
{

class ModManifest {};

class ModLoader
{
public:
	static ModLoader& Get();

	bool Initialize(const std::string& modsDirectory);
	inline void Shutdown();

	bool LoadMod(const std::string& modDirectory);
	bool UnloadMod(const std::string& modId);
	bool EnableMod(const std::string& modId);
	bool DisableMod(const std::string& modId);

	bool ReloadMod(const std::string& modId);
	bool ReloadAllMods();

	const ModManifest* GetMod(const std::string& modId) const;
	const std::vector<const ModManifest*>& GetAllMods() const;
	const std::vector<const ModManifest*>& GetEnabledMods() const;
	const std::vector<const ModManifest*>& GetDisabledMods() const;

	bool IsModLoaded(const std::string& modId) const;
	bool IsModEnabled(const std::string& modId) const;
	bool IsModLoadedAndEnabled(const std::string& modId) const;

	bool HasMod(const std::string& modId) const;
	bool HasModDependency(const std::string& modId, const std::string& dependencyId) const;
	std::vector<std::string> GetMissingDependencies(const std::string& modId) const;

	const std::string& GetModsDirectory() const { return m_ModsDirectory; }
	const std::string& ValidateDependencies(const ModManifest& manifest) const;
	void SortByLoadOrder();

private:
	ModLoader() = default;
	~ModLoader() = default;

	std::string m_ModsDirectory;
	std::unordered_set<std::string> m_Mods;
	std::unordered_set<std::string> m_EnabledMods;
	std::unordered_set<std::string> m_DisabledMods;
	std::vector<const ModManifest*> m_ModList;
};

ModLoader& ModLoader::Get()
{
	static ModLoader instance;
	return instance;
}

bool ModLoader::Initialize(const std::string& modsDirectory)
{
	m_ModsDirectory = modsDirectory;
	QL_CORE_INFO("ModLoader initialized with directory: {}", modsDirectory);
	return true;
}

void ModLoader::Shutdown()
{
	m_Mods.clear();
	m_EnabledMods.clear();
	m_DisabledMods.clear();
	m_ModList.clear();
	QL_CORE_INFO("ModLoader shut down.");
}

bool ModLoader::LoadMod(const std::string& modDirectory)
{
	QL_CORE_INFO("Loading mod from: {}", modDirectory);
	return true;
}

bool ModLoader::UnloadMod(const std::string& modId)
{
	QL_CORE_INFO("Unloading mod: {}", modId);
	return true;
}

bool ModLoader::EnableMod(const std::string& modId)
{
	m_EnabledMods.insert(modId);
	return true;
}

bool ModLoader::DisableMod(const std::string& modId)
{
	m_DisabledMods.insert(modId);
	return true;
}

bool ModLoader::ReloadMod(const std::string& modId)
{
	QL_CORE_INFO("Reloading mod: {}", modId);
	return true;
}

bool ModLoader::ReloadAllMods()
{
	QL_CORE_INFO("Reloading all mods.");
	return true;
}

const ModManifest* ModLoader::GetMod(const std::string& modId) const
{
	return nullptr;
}

const std::vector<const ModManifest*>& ModLoader::GetAllMods() const
{
	return m_ModList;
}

const std::vector<const ModManifest*>& ModLoader::GetEnabledMods() const
{
	return m_ModList;
}

const std::vector<const ModManifest*>& ModLoader::GetDisabledMods() const
{
	return m_ModList;
}

bool ModLoader::IsModLoaded(const std::string& modId) const
{
	return m_Mods.find(modId) != m_Mods.end();
}

bool ModLoader::IsModEnabled(const std::string& modId) const
{
	return m_EnabledMods.find(modId) != m_EnabledMods.end();
}

bool ModLoader::IsModLoadedAndEnabled(const std::string& modId) const
{
	return IsModLoaded(modId) && IsModEnabled(modId);
}

bool ModLoader::HasMod(const std::string& modId) const
{
	return IsModLoaded(modId);
}

bool ModLoader::HasModDependency(const std::string& modId, const std::string& dependencyId) const
{
	return false;
}

std::vector<std::string> ModLoader::GetMissingDependencies(const std::string& modId) const
{
	return {};
}

const std::string& ModLoader::ValidateDependencies(const ModManifest& manifest) const
{
	static std::string empty;
	return empty;
}

void ModLoader::SortByLoadOrder()
{
}

}
