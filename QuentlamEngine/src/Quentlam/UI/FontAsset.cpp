#include "qlpch.h"
#include "FontAsset.h"
#include <filesystem>

namespace Quentlam
{

bool FontAsset::LoadFromFile(const std::string& filepath, uint32_t fontSize)
{
	if (!std::filesystem::exists(filepath))
	{
		QL_CORE_WARN("FontAsset: File not found: {0}", filepath);
		return false;
	}

	m_Filepath = filepath;
	m_Name = std::filesystem::path(filepath).filename().string();
	m_FontSize = static_cast<float>(fontSize);
	m_Loaded = true;

	QL_CORE_INFO("FontAsset: Loaded '{0}' (size: {1}px)", m_Name, fontSize);
	return true;
}

float FontAsset::GetStringWidth(const std::string& text) const
{
	return static_cast<float>(text.size()) * m_FontSize * 0.6f;
}

float FontAsset::GetStringHeight(const std::string& text) const
{
	return GetLineHeight();
}

glm::vec2 FontAsset::MeasureString(const std::string& text) const
{
	return { GetStringWidth(text), GetStringHeight(text) };
}

LocalizationSystem& LocalizationSystem::Get()
{
	static LocalizationSystem instance;
	return instance;
}

void LocalizationSystem::Initialize() {}

void LocalizationSystem::Shutdown()
{
	ClearAll();
}

bool LocalizationSystem::LoadStringTable(const std::string& locale, const std::string& filepath)
{
	FILE* f = fopen(filepath.c_str(), "rb");
	if (!f)
	{
		QL_CORE_ERROR("LocalizationSystem: Cannot open file {0}", filepath);
		return false;
	}

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::string data(len + 1, '\0');
	fread(data.data(), 1, len, f);
	fclose(f);
	data.resize(len);

	return LoadStringTableFromString(locale, data);
}

bool LocalizationSystem::LoadStringTableFromString(const std::string& locale, const std::string& csvData)
{
	std::istringstream stream(csvData);
	std::string line;
	std::vector<std::string> headers;

	if (std::getline(stream, line))
	{
		std::istringstream hl(line);
		std::string cell;
		while (std::getline(hl, cell, ','))
		{
			if (!cell.empty() && cell.back() == '\r')
				cell.pop_back();
			headers.push_back(cell);
		}
	}

	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		std::istringstream ll(line);
		std::string key;
		std::string value;

		if (!std::getline(ll, key, ','))
			continue;
		if (!key.empty() && key.back() == '\r')
			key.pop_back();

		auto TrimQuotes = [](std::string& s) {
			if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
			{
				s = s.substr(1, s.size() - 2);
				size_t pos = 0;
				while ((pos = s.find("\"\"", pos)) != std::string::npos)
				{
					s.replace(pos, 2, "\"");
					pos++;
				}
			}
		};

		TrimQuotes(key);

		int localeIdx = -1;
		for (size_t h = 0; h < headers.size(); ++h)
		{
			std::string hcopy = headers[h];
			TrimQuotes(hcopy);
			if (hcopy == locale)
			{
				localeIdx = static_cast<int>(h);
				break;
			}
		}

		if (localeIdx < 0)
			continue;

		size_t col = 0;
		while (std::getline(ll, value, ','))
		{
			if (!value.empty() && value.back() == '\r')
				value.pop_back();

			if (static_cast<int>(col) == localeIdx)
			{
				TrimQuotes(value);
				m_StringTables[locale][key] = value;
				break;
			}
			col++;
		}
	}

	return true;
}

void LocalizationSystem::SetLocale(const std::string& locale)
{
	if (m_StringTables.find(locale) != m_StringTables.end())
	{
		m_CurrentLocale = locale;
		QL_CORE_INFO("LocalizationSystem: Locale set to '{0}'", locale);
	}
	else
	{
		QL_CORE_WARN("LocalizationSystem: Locale '{0}' not found", locale);
	}
}

std::vector<std::string> LocalizationSystem::GetAvailableLocales() const
{
	std::vector<std::string> result;
	for (const auto& [locale, _] : m_StringTables)
		result.push_back(locale);
	return result;
}

const std::string& LocalizationSystem::GetString(const std::string& key) const
{
	static std::string empty;
	auto lit = m_StringTables.find(m_CurrentLocale);
	if (lit == m_StringTables.end())
		return empty;
	auto it = lit->second.find(key);
	if (it != lit->second.end())
		return it->second;
	return empty;
}

const std::string& LocalizationSystem::GetString(const std::string& key, const std::string& fallback) const
{
	auto lit = m_StringTables.find(m_CurrentLocale);
	if (lit == m_StringTables.end())
		return fallback;
	auto it = lit->second.find(key);
	if (it != lit->second.end())
		return it->second;
	return fallback;
}

std::string LocalizationSystem::Format(const std::string& key, const std::vector<std::string>& args) const
{
	std::string fmt = GetString(key);
	if (fmt.empty())
		fmt = key;

	size_t argIdx = 0;
	size_t pos = 0;
	std::string result;

	while (pos < fmt.size() && argIdx < args.size())
	{
		size_t bracePos = fmt.find('{', pos);
		if (bracePos == std::string::npos)
		{
			result += fmt.substr(pos);
			break;
		}

		result += fmt.substr(pos, bracePos - pos);

		size_t closePos = fmt.find('}', bracePos);
		if (closePos == std::string::npos)
		{
			result += fmt.substr(bracePos);
			break;
		}

		result += args[argIdx++];
		pos = closePos + 1;
	}

	if (pos < fmt.size())
		result += fmt.substr(pos);

	return result;
}

bool LocalizationSystem::HasKey(const std::string& key) const
{
	auto lit = m_StringTables.find(m_CurrentLocale);
	if (lit == m_StringTables.end())
		return false;
	return lit->second.find(key) != lit->second.end();
}

void LocalizationSystem::ClearLocale(const std::string& locale)
{
	m_StringTables.erase(locale);
}

void LocalizationSystem::ClearAll()
{
	m_StringTables.clear();
}

}
