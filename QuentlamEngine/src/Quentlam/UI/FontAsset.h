#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace Quentlam
{

struct FontGlyph
{
	uint32_t Codepoint = 0;
	float Advance = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
};

class FontAsset
{
public:
	FontAsset() = default;
	~FontAsset() = default;

	bool LoadFromFile(const std::string& filepath, uint32_t fontSize = 24);

	const std::string& GetName() const { return m_Name; }
	float GetFontSize() const { return m_FontSize; }
	float GetLineHeight() const { return m_FontSize * 1.2f; }

	float GetStringWidth(const std::string& text) const;
	float GetStringHeight(const std::string& text) const;
	glm::vec2 MeasureString(const std::string& text) const;

	bool IsLoaded() const { return m_Loaded; }

	void SetFallbackFont(Ref<FontAsset> font) { m_FallbackFont = font; }
	Ref<FontAsset> GetFallbackFont() const { return m_FallbackFont; }

private:
	std::string m_Name;
	std::string m_Filepath;
	float m_FontSize = 24.0f;
	bool m_Loaded = false;
	Ref<FontAsset> m_FallbackFont;
};

class LocalizationSystem
{
public:
	static LocalizationSystem& Get();

	void Initialize();
	void Shutdown();

	bool LoadStringTable(const std::string& locale, const std::string& filepath);
	bool LoadStringTableFromString(const std::string& locale, const std::string& csvData);

	const std::string& GetCurrentLocale() const { return m_CurrentLocale; }
	void SetLocale(const std::string& locale);

	std::vector<std::string> GetAvailableLocales() const;

	const std::string& GetString(const std::string& key) const;
	const std::string& GetString(const std::string& key, const std::string& fallback) const;

	std::string Format(const std::string& key, const std::vector<std::string>& args) const;

	bool HasKey(const std::string& key) const;

	void ClearLocale(const std::string& locale);
	void ClearAll();

private:
	LocalizationSystem() = default;
	~LocalizationSystem() = default;

	std::string m_CurrentLocale = "en";
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_StringTables;
};

}
