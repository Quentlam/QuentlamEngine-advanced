#include "Quentlam/Modding/ModdingModule.h"
#include <fstream>
#include <sstream>

namespace Quentlam
{
	bool ModLoader::Initialize(const std::string& modsDirectory)
	{
		m_ModsDirectory = modsDirectory;
		if (!std::filesystem::exists(modsDirectory))
			std::filesystem::create_directories(modsDirectory);
		ScanModsDirectory();
		return true;
	}

	std::optional<ModManifest> ModLoader::LoadManifest(const std::string& modDir) const
	{
		std::filesystem::path manifestPath = std::filesystem::path(modDir) / "manifest.json";

		if (!std::filesystem::exists(manifestPath))
		{
			QL_CORE_WARN("ModLoader: No manifest.json found in {0}", modDir);
			return std::nullopt;
		}

		std::ifstream file(manifestPath);
		if (!file.is_open())
		{
			QL_CORE_ERROR("ModLoader: Failed to open manifest: {0}", manifestPath.string());
			return std::nullopt;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		return ParseManifestJson(buffer.str(), modDir);
	}

	std::optional<ModManifest> ModLoader::ParseManifestJson(const std::string& json, const std::string& modDir) const
	{
		ModManifest manifest;
		manifest.SetDirectory(modDir);

		std::istringstream stream(json);
		std::string line;

		auto trim = [](std::string& s) {
			size_t start = s.find_first_not_of(" \t\r\n");
			if (start == std::string::npos) { s.clear(); return; }
			size_t end = s.find_last_not_of(" \t\r\n");
			s = s.substr(start, end - start + 1);
		};

		auto extractString = [&](const std::string& line) -> std::string {
			size_t start = line.find('"');
			if (start == std::string::npos) return "";
			size_t end = line.find('"', start + 1);
			if (end == std::string::npos) return "";
			return line.substr(start + 1, end - start - 1);
		};

		auto extractArray = [&](const std::string& line) -> std::vector<std::string> {
			std::vector<std::string> result;
			size_t start = line.find('[');
			size_t end = line.find(']');
			if (start == std::string::npos || end == std::string::npos || end <= start) return result;

			std::string arr = line.substr(start + 1, end - start - 1);
			std::istringstream arrStream(arr);
			std::string item;
			while (std::getline(arrStream, item, ','))
			{
				trim(item);
				if (!item.empty() && item.front() == '"')
					result.push_back(extractString(item));
			}
			return result;
		};

		while (std::getline(stream, line))
		{
			trim(line);
			if (line.empty()) continue;

			if (line.find("\"id\"") != std::string::npos)
				manifest.SetId(extractString(line));
			else if (line.find("\"name\"") != std::string::npos)
				manifest.SetName(extractString(line));
			else if (line.find("\"author\"") != std::string::npos)
				manifest.SetAuthor(extractString(line));
			else if (line.find("\"version\"") != std::string::npos)
				manifest.SetVersion(extractString(line));
			else if (line.find("\"description\"") != std::string::npos)
				manifest.SetDescription(extractString(line));
			else if (line.find("\"entry\"") != std::string::npos)
				manifest.SetEntryPoint(extractString(line));
			else if (line.find("\"dependencies\"") != std::string::npos)
			{
				auto deps = extractArray(line);
				for (const auto& d : deps) manifest.AddDependency(d);
			}
			else if (line.find("\"content\"") != std::string::npos)
			{
				auto paths = extractArray(line);
				for (const auto& p : paths) manifest.AddContentPath(p);
			}
		}

		if (!manifest.IsValid())
		{
			QL_CORE_ERROR("ModLoader: manifest.json invalid (missing id or version): {0}", modDir);
			return std::nullopt;
		}

		return manifest;
	}

	bool ModLoader::ScanModsDirectory()
	{
		if (!std::filesystem::exists(m_ModsDirectory))
		{
			std::filesystem::create_directories(m_ModsDirectory);
			QL_CORE_INFO("ModLoader: Created mods directory: {0}", m_ModsDirectory);
			return true;
		}

		for (const auto& entry : std::filesystem::directory_iterator(m_ModsDirectory))
		{
			if (!entry.is_directory()) continue;

			std::string modDir = entry.path().string();
			if (LoadMod(modDir))
			{
				QL_CORE_INFO("ModLoader: Discovered mod: {0}", entry.path().filename().string());
			}
		}
		return true;
	}
}
