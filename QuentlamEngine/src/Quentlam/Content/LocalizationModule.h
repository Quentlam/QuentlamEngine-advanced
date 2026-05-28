#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Quentlam
{
	class LocalizationModule
	{
	public:
		LocalizationModule() = default;
		static LocalizationModule& Get();

		void Initialize();
		void Shutdown();

		void SetCurrentLanguage(const std::string& languageCode);
		const std::string& GetCurrentLanguage() const { return m_CurrentLanguage; }

		void LoadLanguage(const std::string& languageCode, const std::string& filePath);
		void LoadStringTable(const std::string& languageCode, const std::unordered_map<std::string, std::string>& entries);
		void UnloadLanguage(const std::string& languageCode);

		const std::string& GetString(const std::string& key) const;
		const std::string& GetString(const std::string& key, const std::string& fallback) const;
		std::string Format(const std::string& key, const std::vector<std::string>& args) const;
		std::string Format(const std::string& key, const std::unordered_map<std::string, std::string>& args) const;

		bool HasString(const std::string& key) const;
		bool HasLanguage(const std::string& languageCode) const;

		const std::string& GetLanguageDisplayName(const std::string& languageCode) const;
		std::vector<std::string> GetAvailableLanguages() const;

		void AddFontFallback(const std::string& languageCode, const std::string& fontPath);
		const std::vector<std::string>& GetFontFallbacks(const std::string& languageCode) const;

		class StringTable
		{
		public:
			void Set(const std::string& key, const std::string& value) { m_Strings[key] = value; }
			const std::string* Get(const std::string& key) const
			{
				auto it = m_Strings.find(key);
				return it != m_Strings.end() ? &it->second : nullptr;
			}
			bool Has(const std::string& key) const { return m_Strings.find(key) != m_Strings.end(); }
			const std::unordered_map<std::string, std::string>& All() const { return m_Strings; }
			void Merge(const StringTable& other)
			{
				for (const auto& [k, v] : other.m_Strings)
					m_Strings[k] = v;
			}

		private:
			std::unordered_map<std::string, std::string> m_Strings;
		};

		const StringTable* GetLanguageTable(const std::string& languageCode) const;
		const StringTable* GetCurrentTable() const;

		std::function<void(const std::string& oldLang, const std::string& newLang)> OnLanguageChanged;

	private:
		std::string m_CurrentLanguage = "en";
		std::unordered_map<std::string, StringTable> m_Languages;
		std::unordered_map<std::string, std::string> m_LanguageDisplayNames;
		std::unordered_map<std::string, std::vector<std::string>> m_FontFallbacks;
	};

	class TextFormatter
	{
	public:
		static std::string Format(const std::string& template_, const std::vector<std::string>& args);
		static std::string Format(const std::string& template_, const std::unordered_map<std::string, std::string>& args);

		static std::string ToUpper(const std::string& str);
		static std::string ToLower(const std::string& str);
		static std::string Capitalize(const std::string& str);

		static bool MatchGender(const std::string& text, const std::string& gender);
		static std::string ApplyGender(const std::string& text, const std::string& gender);

	private:
		static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
	};

	class FontFallbackSet
	{
	public:
		FontFallbackSet() = default;

		void SetPrimary(const std::string& path);
		void AddFallback(const std::string& path);
		void Clear();

		const std::string& GetPrimary() const { return m_Primary; }
		const std::vector<std::string>& GetFallbacks() const { return m_Fallbacks; }

	private:
		std::string m_Primary;
		std::vector<std::string> m_Fallbacks;
	};

	inline LocalizationModule& LocalizationModule::Get()
	{
		static LocalizationModule instance;
		return instance;
	}

	inline void LocalizationModule::Initialize()
	{
		m_LanguageDisplayNames["en"] = "English";
		m_LanguageDisplayNames["zh"] = "中文";
		m_LanguageDisplayNames["ja"] = "日本語";
		m_LanguageDisplayNames["ko"] = "한국어";
		m_LanguageDisplayNames["es"] = "Español";
		m_LanguageDisplayNames["fr"] = "Français";
		m_LanguageDisplayNames["de"] = "Deutsch";
		m_CurrentLanguage = "en";
	}

	inline void LocalizationModule::Shutdown()
	{
		m_Languages.clear();
	}

	inline void LocalizationModule::SetCurrentLanguage(const std::string& languageCode)
	{
		if (m_CurrentLanguage == languageCode) return;
		std::string old = m_CurrentLanguage;
		if (m_Languages.find(languageCode) != m_Languages.end())
		{
			m_CurrentLanguage = languageCode;
			if (OnLanguageChanged)
				OnLanguageChanged(old, languageCode);
		}
	}

	inline void LocalizationModule::LoadLanguage(const std::string& languageCode, const std::string&)
	{
		if (m_Languages.find(languageCode) == m_Languages.end())
			m_Languages[languageCode] = StringTable{};
	}

	inline void LocalizationModule::LoadStringTable(const std::string& languageCode, const std::unordered_map<std::string, std::string>& entries)
	{
		m_Languages[languageCode].Merge(StringTable{});
		auto& table = m_Languages[languageCode];
		for (const auto& [k, v] : entries)
			table.Set(k, v);
	}

	inline void LocalizationModule::UnloadLanguage(const std::string& languageCode)
	{
		m_Languages.erase(languageCode);
	}

	inline const std::string& LocalizationModule::GetString(const std::string& key) const
	{
		static std::string empty;
		auto langIt = m_Languages.find(m_CurrentLanguage);
		if (langIt != m_Languages.end())
		{
			auto* str = langIt->second.Get(key);
			if (str) return *str;
		}
		auto enIt = m_Languages.find("en");
		if (enIt != m_Languages.end())
		{
			auto* str = enIt->second.Get(key);
			if (str) return *str;
		}
		return empty;
	}

	inline const std::string& LocalizationModule::GetString(const std::string& key, const std::string& fallback) const
	{
		auto& result = GetString(key);
		return result.empty() ? fallback : result;
	}

	inline std::string LocalizationModule::Format(const std::string& key, const std::vector<std::string>& args) const
	{
		return TextFormatter::Format(GetString(key), args);
	}

	inline std::string LocalizationModule::Format(const std::string& key, const std::unordered_map<std::string, std::string>& args) const
	{
		return TextFormatter::Format(GetString(key), args);
	}

	inline bool LocalizationModule::HasString(const std::string& key) const
	{
		auto* table = GetCurrentTable();
		return table && table->Has(key);
	}

	inline bool LocalizationModule::HasLanguage(const std::string& languageCode) const
	{
		return m_Languages.find(languageCode) != m_Languages.end();
	}

	inline const std::string& LocalizationModule::GetLanguageDisplayName(const std::string& languageCode) const
	{
		static std::string empty;
		auto it = m_LanguageDisplayNames.find(languageCode);
		return it != m_LanguageDisplayNames.end() ? it->second : empty;
	}

	inline std::vector<std::string> LocalizationModule::GetAvailableLanguages() const
	{
		std::vector<std::string> result;
		for (const auto& [code, _] : m_Languages)
			result.push_back(code);
		return result;
	}

	inline void LocalizationModule::AddFontFallback(const std::string& languageCode, const std::string& fontPath)
	{
		m_FontFallbacks[languageCode].push_back(fontPath);
	}

	inline const std::vector<std::string>& LocalizationModule::GetFontFallbacks(const std::string& languageCode) const
	{
		static std::vector<std::string> empty;
		auto it = m_FontFallbacks.find(languageCode);
		return it != m_FontFallbacks.end() ? it->second : empty;
	}

	inline const LocalizationModule::StringTable* LocalizationModule::GetLanguageTable(const std::string& languageCode) const
	{
		auto it = m_Languages.find(languageCode);
		return it != m_Languages.end() ? &it->second : nullptr;
	}

	inline const LocalizationModule::StringTable* LocalizationModule::GetCurrentTable() const
	{
		return GetLanguageTable(m_CurrentLanguage);
	}

	inline std::string TextFormatter::Format(const std::string& template_, const std::vector<std::string>& args)
	{
		std::string result = template_;
		size_t pos = 0;
		for (size_t i = 0; i < args.size(); ++i)
		{
			std::string placeholder = "{" + std::to_string(i) + "}";
			pos = result.find(placeholder, pos);
			while (pos != std::string::npos)
			{
				result.replace(pos, placeholder.length(), args[i]);
				pos = result.find(placeholder, pos + args[i].length());
			}
		}
		return result;
	}

	inline std::string TextFormatter::Format(const std::string& template_, const std::unordered_map<std::string, std::string>& args)
	{
		std::string result = template_;
		for (const auto& [key, value] : args)
		{
			std::string placeholder = "{" + key + "}";
			size_t pos = 0;
			while ((pos = result.find(placeholder, pos)) != std::string::npos)
			{
				result.replace(pos, placeholder.length(), value);
				pos += value.length();
			}
		}
		return result;
	}

	inline std::string TextFormatter::ToUpper(const std::string& str)
	{
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	inline std::string TextFormatter::ToLower(const std::string& str)
	{
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}

	inline std::string TextFormatter::Capitalize(const std::string& str)
	{
		if (str.empty()) return str;
		std::string result = str;
		result[0] = static_cast<char>(std::toupper(result[0]));
		return result;
	}

	inline bool TextFormatter::MatchGender(const std::string&, const std::string&)
	{
		return true;
	}

	inline std::string TextFormatter::ApplyGender(const std::string& text, const std::string&)
	{
		return text;
	}

	inline std::string TextFormatter::ReplaceAll(std::string str, const std::string& from, const std::string& to)
	{
		size_t pos = 0;
		while ((pos = str.find(from, pos)) != std::string::npos)
		{
			str.replace(pos, from.length(), to);
			pos += to.length();
		}
		return str;
	}
}
