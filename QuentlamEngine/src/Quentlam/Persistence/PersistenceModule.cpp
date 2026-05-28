#include "qlpch.h"
#include "Quentlam/Persistence/PersistenceModule.h"
#include "Quentlam/Core/Log.h"
#include <fstream>
#include <filesystem>

namespace Quentlam
{

__declspec(noinline)
ESaveStatus SaveManager::SaveToFile(const std::string& filepath, const SaveSchema& schema)
{
	if (m_IsSaving)
		return ESaveStatus::Failure;

	m_IsSaving = true;

	std::string data = schema.Serialize();
	if (StateSerializer::WriteToFile(filepath, data))
	{
		m_IsSaving = false;
		QL_CORE_INFO("SaveManager: Saved to '{0}'", filepath);
		return ESaveStatus::Success;
	}

	m_IsSaving = false;
	QL_CORE_ERROR("SaveManager: Failed to save to '{0}'", filepath);
	return ESaveStatus::Failure;
}

__declspec(noinline)
ESaveStatus SaveManager::LoadFromFile(const std::string& filepath, SaveSchema& schema)
{
	if (m_IsLoading)
		return ESaveStatus::Failure;

	m_IsLoading = true;

	auto dataOpt = StateSerializer::ReadFromFile(filepath);
	if (!dataOpt)
	{
		m_IsLoading = false;
		QL_CORE_ERROR("SaveManager: Failed to read '{0}'", filepath);
		return ESaveStatus::FileNotFound;
	}

	if (schema.Deserialize(*dataOpt))
	{
		m_IsLoading = false;
		QL_CORE_INFO("SaveManager: Loaded from '{0}'", filepath);
		return ESaveStatus::Success;
	}

	m_IsLoading = false;
	QL_CORE_ERROR("SaveManager: Failed to deserialize '{0}'", filepath);
	return ESaveStatus::Corrupted;
}

bool SaveManager::SlotExists(const std::string& slotId) const
{
	return std::filesystem::exists(GetSlotFilePath(slotId));
}

bool SaveManager::DeleteSlot(const std::string& slotId)
{
	std::string filepath = GetSlotFilePath(slotId);
	std::string infopath = GetSlotInfoPath(slotId);
	bool ok = true;
	if (std::filesystem::exists(filepath))
		ok = std::filesystem::remove(filepath);
	if (std::filesystem::exists(infopath))
		ok &= std::filesystem::remove(infopath);
	return ok;
}

std::optional<SaveSlot> SaveManager::GetSlotInfo(const std::string& slotId) const
{
	std::string infopath = GetSlotInfoPath(slotId);
	if (!std::filesystem::exists(infopath))
		return std::nullopt;
	return ReadSlotInfo(infopath);
}

std::vector<SaveSlot> SaveManager::GetAllSlots() const
{
	std::vector<SaveSlot> slots;
	if (!std::filesystem::exists(m_SaveDirectory))
		return slots;

	for (const auto& entry : std::filesystem::directory_iterator(m_SaveDirectory))
	{
		if (entry.path().extension() == ".info")
		{
			slots.push_back(ReadSlotInfo(entry.path().string()));
		}
	}
	return slots;
}

void SaveManager::SetSaveDirectory(const std::string& directory)
{
	m_SaveDirectory = directory;
	if (!std::filesystem::exists(directory))
		std::filesystem::create_directories(directory);
}

void SaveManager::RegisterMigration(const MigrationStep& migration)
{
	m_Migrations.push_back(migration);
}

void SaveManager::UnregisterMigration(int32_t fromVersion, int32_t toVersion)
{
	m_Migrations.erase(
		std::remove_if(m_Migrations.begin(), m_Migrations.end(),
			[fromVersion, toVersion](const MigrationStep& m) {
				return m.GetFromVersion() == fromVersion && m.GetToVersion() == toVersion;
			}),
		m_Migrations.end()
	);
}

void SaveManager::ClearMigrations()
{
	m_Migrations.clear();
}

ESaveStatus SaveManager::ValidateSaveFile(const std::string& filepath, int32_t expectedVersion) const
{
	if (!std::filesystem::exists(filepath))
		return ESaveStatus::FileNotFound;

	auto dataOpt = StateSerializer::ReadFromFile(filepath);
	if (!dataOpt)
		return ESaveStatus::Corrupted;

	SaveSchema temp(expectedVersion, "");
	if (temp.Deserialize(*dataOpt))
		return ESaveStatus::Success;
	return ESaveStatus::Corrupted;
}

SaveSlot SaveManager::ReadSlotInfo(const std::string& filepath) const
{
	SaveSlot slot;
	auto dataOpt = StateSerializer::ReadFromFile(filepath);
	if (!dataOpt)
		return slot;

	SaveSchema schema(1, "SlotInfo");
	if (schema.Deserialize(*dataOpt))
	{
		slot.IsValid = true;
		slot.SlotId = std::filesystem::path(filepath).stem().string();

		auto* nameVal = schema.GetMetadata("name");
		if (nameVal)
		{
			auto* str = std::get_if<std::string>(nameVal);
			if (str) slot.SlotName = *str;
		}

		auto* versionVal = schema.GetMetadata("version");
		if (versionVal)
		{
			auto* v = std::get_if<int32_t>(versionVal);
			if (v) slot.SaveVersion = *v;
		}
	}
	return slot;
}

void SaveManager::WriteSlotInfo(const std::string& filepath, const SaveSlot& slot) const
{
	SaveSchema schema(1, "SlotInfo");
	schema.SetMetadata("name", slot.SlotName);
	schema.SetMetadata("version", slot.SaveVersion);
	schema.SetMetadata("engine_version", slot.EngineVersion);

	std::string data = schema.Serialize();
	StateSerializer::WriteToFile(filepath, data);
}

ESaveStatus SaveManager::MigrateIfNeeded(SaveSchema& schema)
{
	for (const auto& migration : m_Migrations)
	{
		if (migration.GetFromVersion() == schema.GetVersion())
		{
			migration.Execute(&schema);
		}
	}
	return ESaveStatus::Success;
}

// --- SaveSchema implementations ---

std::string SaveSchema::Serialize() const
{
	return StateSerializer::SchemaToJson(*this);
}

bool SaveSchema::Deserialize(const std::string& data)
{
	auto schema = StateSerializer::JsonToSchema(data);
	if (schema.GetVersion() != m_Version)
	{
		QL_CORE_ERROR("SaveSchema: Version mismatch during deserialization");
		return false;
	}
	m_Name = schema.GetName();
	m_Sections = schema.GetAllSections();
	return true;
}

// --- StateSerializer implementations ---

std::string StateSerializer::ObjectToJson(const SaveObject& obj)
{
	std::stringstream ss;
	ss << "{";
	ss << "\"id\":\"" << obj.GetId() << "\",";
	ss << "\"type\":\"" << obj.GetType() << "\",";
	ss << "\"values\":{";
	bool first = true;
	for (const auto& [key, val] : obj.GetAllValues())
	{
		if (!first) ss << ",";
		first = false;
		ss << "\"" << key << "\":";
		if (auto* b = std::get_if<bool>(&val)) ss << (*b ? "true" : "false");
		else if (auto* i = std::get_if<int32_t>(&val)) ss << *i;
		else if (auto* f = std::get_if<float>(&val)) ss << *f;
		else if (auto* s = std::get_if<std::string>(&val)) ss << "\"" << *s << "\"";
		else ss << "null";
	}
	ss << "}";
	ss << "}";
	return ss.str();
}

SaveObject StateSerializer::JsonToObject(const std::string& json)
{
	SaveObject obj;
	return obj;
}

std::string StateSerializer::SectionToJson(const SaveSection& section)
{
	std::stringstream ss;
	ss << "{";
	ss << "\"name\":\"" << section.GetName() << "\",";
	ss << "\"objects\":[";
	bool first = true;
	for (const auto& obj : section.GetAllObjects())
	{
		if (!first) ss << ",";
		first = false;
		ss << ObjectToJson(obj);
	}
	ss << "]";
	ss << "}";
	return ss.str();
}

SaveSection StateSerializer::JsonToSection(const std::string& json)
{
	SaveSection section;
	return section;
}

std::string StateSerializer::SchemaToJson(const SaveSchema& schema)
{
	std::stringstream ss;
	ss << "{";
	ss << "\"version\":" << schema.GetVersion() << ",";
	ss << "\"name\":\"" << schema.GetName() << "\",";
	ss << "\"sections\":[";
	bool first = true;
	for (const auto& section : schema.GetAllSections())
	{
		if (!first) ss << ",";
		first = false;
		ss << SectionToJson(section);
	}
	ss << "]";
	ss << "}";
	return ss.str();
}

SaveSchema StateSerializer::JsonToSchema(const std::string& json)
{
	SaveSchema schema;
	schema.Deserialize(json);
	return schema;
}

bool StateSerializer::WriteToFile(const std::string& filepath, const std::string& data)
{
	std::ofstream file(filepath);
	if (!file.is_open())
		return false;
	file << data;
	file.close();
	return true;
}

std::optional<std::string> StateSerializer::ReadFromFile(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
		return std::nullopt;
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

std::string StateSerializer::IndentJson(const std::string& json, int spaces)
{
	std::string result;
	int indent = 0;
	bool inString = false;
	for (size_t i = 0; i < json.size(); ++i)
	{
		char c = json[i];
		if (c == '"' && (i == 0 || json[i - 1] != '\\'))
			inString = !inString;
		if (!inString)
		{
			if (c == '{' || c == '[')
			{
				result += c;
				result += '\n';
				indent += spaces;
				for (int j = 0; j < indent; ++j) result += ' ';
			}
			else if (c == '}' || c == ']')
			{
				result += '\n';
				indent -= spaces;
				for (int j = 0; j < indent; ++j) result += ' ';
				result += c;
			}
			else if (c == ',')
			{
				result += c;
				result += '\n';
				for (int j = 0; j < indent; ++j) result += ' ';
			}
			else if (c == ':')
			{
				result += ": ";
			}
			else if (!std::isspace(c))
			{
				result += c;
			}
		}
		else
		{
			result += c;
		}
	}
	return result;
}

// --- SaveValidator implementations ---

SaveValidator::ValidationResult SaveValidator::ValidateSchema(const SaveSchema& schema)
{
	ValidationResult result;
	result.IsValid = true;
	return result;
}

SaveValidator::ValidationResult SaveValidator::ValidateSlot(const std::string& slotId)
{
	ValidationResult result;
	result.IsValid = true;
	return result;
}

SaveValidator::ValidationResult SaveValidator::ValidateFile(const std::string& filepath)
{
	ValidationResult result;
	result.IsValid = true;
	return result;
}

bool SaveValidator::CheckIntegrity(const std::string& data)
{
	return !data.empty();
}

std::string SaveValidator::ComputeChecksum(const std::string& data)
{
	size_t hash = 0;
	for (char c : data)
		hash = hash * 31 + c;
	std::stringstream ss;
	ss << std::hex << hash;
	return ss.str();
}

}
