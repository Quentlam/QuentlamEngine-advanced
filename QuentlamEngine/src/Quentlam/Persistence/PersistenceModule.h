#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>

namespace Quentlam
{
	enum class ESaveStatus : uint8_t
	{
		Success = 0,
		Failure = 1,
		VersionMismatch = 2,
		Corrupted = 3,
		FileNotFound = 4,
		PermissionDenied = 5
	};

	struct SaveSlot
	{
		std::string SlotId;
		std::string SlotName;
		std::chrono::system_clock::time_point LastPlayed;
		std::chrono::system_clock::time_point Created;
		int32_t PlayTimeSeconds = 0;
		std::string ThumbnailPath;
		bool IsValid = false;
		int32_t SaveVersion = 1;
		std::string EngineVersion;
	};

	class SaveSchema;

	using SaveValue = std::variant<
		std::monostate,
		bool,
		int32_t,
		int64_t,
		float,
		double,
		std::string,
		std::vector<int32_t>,
		std::vector<float>,
		std::vector<std::string>
	>;

	class SaveObject
	{
	public:
		SaveObject() = default;
		SaveObject(const std::string& id);
		SaveObject(const std::string& id, const std::string& type);

		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }
		const std::string& GetType() const { return m_Type; }
		void SetType(const std::string& type) { m_Type = type; }

		void SetValue(const std::string& key, const SaveValue& value);
		SaveValue* GetValue(const std::string& key);
		const SaveValue* GetValue(const std::string& key) const;
		bool HasValue(const std::string& key) const;
		void RemoveValue(const std::string& key);

		template<typename T>
		T GetValueAs(const std::string& key, const T& defaultValue = T{}) const
		{
			const SaveValue* val = GetValue(key);
			if (!val) return defaultValue;

			if constexpr (std::is_same_v<T, bool>)
				if (auto* p = std::get_if<bool>(val)) return *p;
			else if constexpr (std::is_same_v<T, int32_t>)
				if (auto* p = std::get_if<int32_t>(val)) return *p;
			else if constexpr (std::is_same_v<T, float>)
				if (auto* p = std::get_if<float>(val)) return *p;
			else if constexpr (std::is_same_v<T, std::string>)
				if (auto* p = std::get_if<std::string>(val)) return *p;

			return defaultValue;
		}

		const std::unordered_map<std::string, SaveValue>& GetAllValues() const { return m_Values; }

		void Clear() { m_Values.clear(); }

	private:
		std::string m_Id;
		std::string m_Type;
		std::unordered_map<std::string, SaveValue> m_Values;
	};

	class SaveSection
	{
	public:
		SaveSection() = default;
		SaveSection(const std::string& name);

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		void AddObject(const SaveObject& obj);
		SaveObject* GetObject(const std::string& objectId);
		const SaveObject* GetObject(const std::string& objectId) const;
		void RemoveObject(const std::string& objectId);
		bool HasObject(const std::string& objectId) const;

		const std::vector<SaveObject>& GetAllObjects() const { return m_Objects; }
		std::vector<SaveObject>& GetAllObjects() { return m_Objects; }

		void Clear() { m_Objects.clear(); }

	private:
		std::string m_Name;
		std::vector<SaveObject> m_Objects;
	};

	class QUENTLAM_API SaveSchema
	{
	public:
		SaveSchema() = default;
		SaveSchema(int32_t version, const std::string& name);

		int32_t GetVersion() const { return m_Version; }
		void SetVersion(int32_t version) { m_Version = version; }
		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		void AddSection(const SaveSection& section);
		SaveSection* GetSection(const std::string& sectionName);
		const SaveSection* GetSection(const std::string& sectionName) const;
		void RemoveSection(const std::string& sectionName);

		const std::vector<SaveSection>& GetAllSections() const { return m_Sections; }

		void SetMetadata(const std::string& key, const SaveValue& value);
		SaveValue* GetMetadata(const std::string& key);
		const SaveValue* GetMetadata(const std::string& key) const;

		void Clear();

		std::string Serialize() const;
		bool Deserialize(const std::string& data);

	protected:
		virtual void OnBeforeSerialize() {}
		virtual void OnAfterDeserialize(int32_t oldVersion) {}

	private:
		int32_t m_Version = 1;
		std::string m_Name;
		std::vector<SaveSection> m_Sections;
		std::unordered_map<std::string, SaveValue> m_Metadata;
	};

	class MigrationStep
	{
	public:
		MigrationStep() = default;
		MigrationStep(int32_t fromVersion, int32_t toVersion);

		int32_t GetFromVersion() const { return m_FromVersion; }
		int32_t GetToVersion() const { return m_ToVersion; }

		void SetMigrateFunc(std::function<void(SaveSchema*)> func);
		void Execute(SaveSchema* schema) const;

	private:
		int32_t m_FromVersion = 0;
		int32_t m_ToVersion = 0;
		std::function<void(SaveSchema*)> m_MigrateFunc;
	};

	class QUENTLAM_API SaveManager
	{
	public:
		static SaveManager& Get();

		ESaveStatus SaveToFile(const std::string& filepath, const SaveSchema& schema);
		ESaveStatus LoadFromFile(const std::string& filepath, SaveSchema& schema);

		bool SlotExists(const std::string& slotId) const;
		bool DeleteSlot(const std::string& slotId);
		std::optional<SaveSlot> GetSlotInfo(const std::string& slotId) const;
		std::vector<SaveSlot> GetAllSlots() const;

		bool IsSaving() const { return m_IsSaving; }
		bool IsLoading() const { return m_IsLoading; }

		void SetSaveDirectory(const std::string& directory);
		const std::string& GetSaveDirectory() const { return m_SaveDirectory; }

		std::string GetSlotFilePath(const std::string& slotId) const;

		void RegisterMigration(const MigrationStep& migration);
		void UnregisterMigration(int32_t fromVersion, int32_t toVersion);
		void ClearMigrations();

		ESaveStatus ValidateSaveFile(const std::string& filepath, int32_t expectedVersion) const;

		std::function<void(float progress)> OnSaveProgress;
		std::function<void(float progress)> OnLoadProgress;
		std::function<void(const std::string& error)> OnSaveError;
		std::function<void(const std::string& error)> OnLoadError;

	private:
		SaveManager() = default;

		std::string GetSlotInfoPath(const std::string& slotId) const;

		SaveSlot ReadSlotInfo(const std::string& filepath) const;
		void WriteSlotInfo(const std::string& filepath, const SaveSlot& slot) const;

		ESaveStatus MigrateIfNeeded(SaveSchema& schema);

		std::string m_SaveDirectory = "saves";
		bool m_IsSaving = false;
		bool m_IsLoading = false;
		std::vector<MigrationStep> m_Migrations;
	};

	class QUENTLAM_API StateSerializer
	{
	public:
		StateSerializer() = default;

		static std::string ObjectToJson(const SaveObject& obj);
		static SaveObject JsonToObject(const std::string& json);

		static std::string SectionToJson(const SaveSection& section);
		static SaveSection JsonToSection(const std::string& json);

		static std::string SchemaToJson(const SaveSchema& schema);
		static SaveSchema JsonToSchema(const std::string& json);

		static bool WriteToFile(const std::string& filepath, const std::string& data);
		static std::optional<std::string> ReadFromFile(const std::string& filepath);

	private:
		static std::string IndentJson(const std::string& json, int spaces = 2);
	};

	class QUENTLAM_API SaveValidator
	{
	public:
		struct ValidationResult
		{
			bool IsValid = true;
			std::vector<std::string> Errors;
			std::vector<std::string> Warnings;
			int32_t DetectedVersion = 0;
		};

		static ValidationResult ValidateSchema(const SaveSchema& schema);
		static ValidationResult ValidateSlot(const std::string& slotId);
		static ValidationResult ValidateFile(const std::string& filepath);

		static bool CheckIntegrity(const std::string& data);
		static std::string ComputeChecksum(const std::string& data);
	};

	inline SaveObject::SaveObject(const std::string& id)
		: m_Id(id) {}

	inline SaveObject::SaveObject(const std::string& id, const std::string& type)
		: m_Id(id), m_Type(type) {}

	inline void SaveObject::SetValue(const std::string& key, const SaveValue& value)
	{
		m_Values[key] = value;
	}

	inline SaveValue* SaveObject::GetValue(const std::string& key)
	{
		auto it = m_Values.find(key);
		return it != m_Values.end() ? &it->second : nullptr;
	}

	inline const SaveValue* SaveObject::GetValue(const std::string& key) const
	{
		auto it = m_Values.find(key);
		return it != m_Values.end() ? &it->second : nullptr;
	}

	inline bool SaveObject::HasValue(const std::string& key) const
	{
		return m_Values.find(key) != m_Values.end();
	}

	inline void SaveObject::RemoveValue(const std::string& key)
	{
		m_Values.erase(key);
	}

	inline SaveSection::SaveSection(const std::string& name)
		: m_Name(name) {}

	inline void SaveSection::AddObject(const SaveObject& obj)
	{
		m_Objects.push_back(obj);
	}

	inline SaveObject* SaveSection::GetObject(const std::string& objectId)
	{
		for (auto& obj : m_Objects)
		{
			if (obj.GetId() == objectId)
				return &obj;
		}
		return nullptr;
	}

	inline const SaveObject* SaveSection::GetObject(const std::string& objectId) const
	{
		for (const auto& obj : m_Objects)
		{
			if (obj.GetId() == objectId)
				return &obj;
		}
		return nullptr;
	}

	inline void SaveSection::RemoveObject(const std::string& objectId)
	{
		m_Objects.erase(
			std::remove_if(m_Objects.begin(), m_Objects.end(),
				[&objectId](const SaveObject& obj) { return obj.GetId() == objectId; }),
			m_Objects.end()
		);
	}

	inline bool SaveSection::HasObject(const std::string& objectId) const
	{
		return GetObject(objectId) != nullptr;
	}

	inline SaveSchema::SaveSchema(int32_t version, const std::string& name)
		: m_Version(version), m_Name(name) {}

	inline void SaveSchema::AddSection(const SaveSection& section)
	{
		m_Sections.push_back(section);
	}

	inline SaveSection* SaveSchema::GetSection(const std::string& sectionName)
	{
		for (auto& section : m_Sections)
		{
			if (section.GetName() == sectionName)
				return &section;
		}
		return nullptr;
	}

	inline const SaveSection* SaveSchema::GetSection(const std::string& sectionName) const
	{
		for (const auto& section : m_Sections)
		{
			if (section.GetName() == sectionName)
				return &section;
		}
		return nullptr;
	}

	inline void SaveSchema::RemoveSection(const std::string& sectionName)
	{
		m_Sections.erase(
			std::remove_if(m_Sections.begin(), m_Sections.end(),
				[&sectionName](const SaveSection& s) { return s.GetName() == sectionName; }),
			m_Sections.end()
		);
	}

	inline void SaveSchema::SetMetadata(const std::string& key, const SaveValue& value)
	{
		m_Metadata[key] = value;
	}

	inline SaveValue* SaveSchema::GetMetadata(const std::string& key)
	{
		auto it = m_Metadata.find(key);
		return it != m_Metadata.end() ? &it->second : nullptr;
	}

	inline const SaveValue* SaveSchema::GetMetadata(const std::string& key) const
	{
		auto it = m_Metadata.find(key);
		return it != m_Metadata.end() ? &it->second : nullptr;
	}

	inline void SaveSchema::Clear()
	{
		m_Sections.clear();
		m_Metadata.clear();
	}

	inline MigrationStep::MigrationStep(int32_t fromVersion, int32_t toVersion)
		: m_FromVersion(fromVersion), m_ToVersion(toVersion) {}

	inline void MigrationStep::SetMigrateFunc(std::function<void(SaveSchema*)> func)
	{
		m_MigrateFunc = std::move(func);
	}

	inline void MigrationStep::Execute(SaveSchema* schema) const
	{
		if (m_MigrateFunc && schema)
			m_MigrateFunc(schema);
	}

	inline SaveManager& SaveManager::Get()
	{
		static SaveManager instance;
		return instance;
	}

	inline std::string SaveManager::GetSlotFilePath(const std::string& slotId) const
	{
		return m_SaveDirectory + "/" + slotId + ".sav";
	}

	inline std::string SaveManager::GetSlotInfoPath(const std::string& slotId) const
	{
		return m_SaveDirectory + "/" + slotId + ".info";
	}
}
