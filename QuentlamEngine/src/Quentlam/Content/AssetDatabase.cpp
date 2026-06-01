#include "qlpch.h"
#include "AssetDatabase.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <sys/stat.h>
#include <sys/stat.h>

namespace Quentlam
{

static std::string HexChar = "0123456789abcdef";

std::string AssetMetaFile::GenerateGUID()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);

	uint64_t part1 = dis(gen);
	uint64_t part2 = dis(gen);

	char guid[33];
	for (int i = 0; i < 16; ++i)
	{
		if (i < 8)
			guid[i] = HexChar[(part1 >> ((7 - i) * 4)) & 0xF];
		else
			guid[i] = HexChar[(part2 >> ((15 - i) * 4)) & 0xF];
	}
	guid[32] = '\0';
	return std::string(guid);
}

std::string AssetMetaFile::GetMetaPath(const std::string& assetPath)
{
	return assetPath + ".meta";
}

AssetMetaFile::AssetMetaFile(const std::string& guid, const std::string& assetPath)
	: m_GUID(guid), m_AssetPath(assetPath) {}

bool AssetMetaFile::SerializeToFile(const std::string& metaPath) const
{
	std::string json = SerializeToString();
	FILE* f = fopen(metaPath.c_str(), "w");
	if (!f) return false;
	fwrite(json.c_str(), 1, json.size(), f);
	fclose(f);
	return true;
}

bool AssetMetaFile::DeserializeFromFile(const std::string& metaPath)
{
	FILE* f = fopen(metaPath.c_str(), "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::string json(len + 1, '\0');
	fread(json.data(), 1, len, f);
	fclose(f);
	json.resize(len);
	return DeserializeFromString(json);
}

std::string AssetMetaFile::SerializeToString() const
{
	std::ostringstream ss;
	ss << "{\n";
	ss << "  \"guid\": \"" << m_GUID << "\",\n";
	ss << "  \"asset_path\": \"" << m_AssetPath << "\",\n";
	ss << "  \"asset_type\": \"" << m_AssetType << "\",\n";
	ss << "  \"pid\": " << m_PID << ",\n";
	ss << "  \"instance_id\": " << m_InstanceID << ",\n";
	ss << "  \"import_time\": " << std::fixed << std::setprecision(6) << m_ImportTime << "\n";
	ss << "}\n";
	return ss.str();
}

bool AssetMetaFile::DeserializeFromString(const std::string& json)
{
	size_t p = 0;

	auto SkipWhite = [&json, &p]() {
		while (p < json.size() && std::isspace((unsigned char)json[p])) p++;
	};

	auto ReadString = [&json, &p, &SkipWhite]() -> std::string {
		SkipWhite();
		if (p < json.size() && (json[p] == '"' || json[p] == '\'')) {
			char quote = json[p++];
			size_t start = p;
			while (p < json.size() && json[p] != quote) {
				if (json[p] == '\\') p++;
				p++;
			}
			std::string result = json.substr(start, p - start);
			if (p < json.size()) p++;
			return result;
		}
		return "";
	};

	auto ReadNumber = [&json, &p, &SkipWhite]() -> double {
		SkipWhite();
		size_t start = p;
		if (p < json.size() && (json[p] == '-' || json[p] == '+')) p++;
		while (p < json.size() && (std::isdigit((unsigned char)json[p]) || json[p] == '.' || json[p] == 'e' || json[p] == 'E' || json[p] == '-' || json[p] == '+')) p++;
		return std::stod(json.substr(start, p - start));
	};

	auto Expect = [&json, &p](char c) -> bool {
		while (p < json.size() && std::isspace((unsigned char)json[p])) p++;
		if (p < json.size() && json[p] == c) { p++; return true; }
		return false;
	};

	SkipWhite();
	if (p < json.size() && json[p] == '{') p++;
	else return false;

	while (true)
	{
		SkipWhite();
		if (p >= json.size()) break;
		if (json[p] == '}') { p++; break; }
		if (json[p] == ',') { p++; continue; }

		std::string key = ReadString();
		Expect(':');

		if (key == "guid") m_GUID = ReadString();
		else if (key == "asset_path") m_AssetPath = ReadString();
		else if (key == "asset_type") m_AssetType = ReadString();
		else if (key == "pid") m_PID = (uint64_t)ReadNumber();
		else if (key == "instance_id") m_InstanceID = (uint64_t)ReadNumber();
		else if (key == "import_time") m_ImportTime = (float)ReadNumber();
		else {
			SkipWhite();
			while (p < json.size() && json[p] != ',' && json[p] != '}') p++;
		}
	}

	return true;
}

AssetDatabase& AssetDatabase::Get()
{
	static AssetDatabase instance;
	return instance;
}

bool AssetDatabase::Initialize(const std::string& projectPath)
{
	m_ProjectPath = projectPath;
	m_Assets.clear();
	m_PathToGUID.clear();
	return Refresh();
}

void AssetDatabase::Shutdown()
{
	m_Assets.clear();
	m_PathToGUID.clear();
}

bool AssetDatabase::RegisterAsset(const std::string& guid, const std::string& path, const std::string& type)
{
	if (m_Assets.find(guid) != m_Assets.end())
		return false;

	auto meta = CreateRef<AssetMetaFile>(guid, path);
	meta->SetAssetType(type);

	m_Assets[guid] = meta;
	m_PathToGUID[path] = guid;

	return true;
}

bool AssetDatabase::UnregisterAsset(const std::string& guid)
{
	auto it = m_Assets.find(guid);
	if (it == m_Assets.end())
		return false;

	const auto& path = it->second->GetAssetPath();
	m_PathToGUID.erase(path);
	m_Assets.erase(it);
	return true;
}

bool AssetDatabase::UnregisterByPath(const std::string& path)
{
	auto git = m_PathToGUID.find(path);
	if (git == m_PathToGUID.end())
		return false;

	m_Assets.erase(git->second);
	m_PathToGUID.erase(git);
	return true;
}

bool AssetDatabase::HasAsset(const std::string& guid) const
{
	return m_Assets.find(guid) != m_Assets.end();
}

bool AssetDatabase::HasAssetByPath(const std::string& path) const
{
	return m_PathToGUID.find(path) != m_PathToGUID.end();
}

const std::string* AssetDatabase::GetAssetPath(const std::string& guid) const
{
	auto it = m_Assets.find(guid);
	if (it == m_Assets.end())
		return nullptr;
	return &it->second->GetAssetPath();
}

const std::string* AssetDatabase::GetAssetGUID(const std::string& path) const
{
	auto it = m_PathToGUID.find(path);
	if (it == m_PathToGUID.end())
		return nullptr;
	return &it->second;
}

const std::string* AssetDatabase::GetAssetType(const std::string& guid) const
{
	auto it = m_Assets.find(guid);
	if (it == m_Assets.end())
		return nullptr;
	return &it->second->GetAssetType();
}

bool AssetDatabase::Refresh()
{
	m_Assets.clear();
	m_PathToGUID.clear();
	return RefreshDirectory(m_ProjectPath);
}

bool AssetDatabase::RefreshAsset(const std::string& path)
{
	std::string metaPath = AssetMetaFile::GetMetaPath(path);
	auto meta = CreateRef<AssetMetaFile>();
	if (meta->DeserializeFromFile(metaPath))
	{
		RegisterAsset(meta->GetGUID(), meta->GetAssetPath(), meta->GetAssetType());
		return true;
	}
	return false;
}

bool AssetDatabase::RefreshDirectory(const std::string& dirPath)
{
	return true;
}

bool AssetDatabase::CreateMetaForAsset(const std::string& assetPath)
{
	std::string metaPath = AssetMetaFile::GetMetaPath(assetPath);
	std::string guid = AssetMetaFile::GenerateGUID();

	auto meta = CreateRef<AssetMetaFile>(guid, assetPath);

	std::string ext = std::filesystem::path(assetPath).extension().string();
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
		meta->SetAssetType("Texture2D");
	else if (ext == ".mat")
		meta->SetAssetType("Material");
	else if (ext == ".anim")
		meta->SetAssetType("AnimationClip");
	else if (ext == ".controller")
		meta->SetAssetType("AnimatorController");
	else if (ext == ".scene")
		meta->SetAssetType("Scene");
	else if (ext == ".lua")
		meta->SetAssetType("LuaScript");
	else if (ext == ".glsl" || ext == ".vert" || ext == ".frag")
		meta->SetAssetType("Shader");
	else if (ext == ".prefab")
		meta->SetAssetType("Prefab");
	else if (ext == ".navmesh")
		meta->SetAssetType("NavMesh");
	else
		meta->SetAssetType("Unknown");

	bool result = meta->SerializeToFile(metaPath);
	if (result)
		RegisterAsset(guid, assetPath, meta->GetAssetType());
	return result;
}

bool AssetDatabase::DeleteMetaForAsset(const std::string& assetPath)
{
	std::string metaPath = AssetMetaFile::GetMetaPath(assetPath);
	UnregisterByPath(assetPath);
	remove(metaPath.c_str());
	return true;
}

Ref<AssetMetaFile> AssetDatabase::GetMeta(const std::string& guid) const
{
	auto it = m_Assets.find(guid);
	if (it != m_Assets.end())
		return it->second;
	return nullptr;
}

Ref<AssetMetaFile> AssetDatabase::GetMetaByPath(const std::string& path) const
{
	auto git = m_PathToGUID.find(path);
	if (git != m_PathToGUID.end())
		return GetMeta(git->second);
	return nullptr;
}

bool AssetDatabase::RenameAsset(const std::string& oldPath, const std::string& newPath)
{
	auto git = m_PathToGUID.find(oldPath);
	if (git == m_PathToGUID.end())
	{
		if (!std::filesystem::exists(newPath))
			return false;
		return CreateMetaForAsset(newPath);
	}

	std::string guid = git->second;
	auto meta = m_Assets[guid];

	std::string oldMetaPath = AssetMetaFile::GetMetaPath(oldPath);
	std::string newMetaPath = AssetMetaFile::GetMetaPath(newPath);

	m_PathToGUID.erase(oldPath);
	m_PathToGUID[newPath] = guid;

	meta->SetAssetPath(newPath);
	meta->SerializeToFile(newMetaPath);
	remove(oldMetaPath.c_str());

	return true;
}

bool AssetDatabase::MoveAsset(const std::string& oldPath, const std::string& newPath)
{
	return RenameAsset(oldPath, newPath);
}

bool AssetDatabase::DeleteAsset(const std::string& path)
{
	DeleteMetaForAsset(path);
	return true;
}

std::vector<std::string> AssetDatabase::GetAssetsByType(const std::string& type) const
{
	std::vector<std::string> result;
	for (const auto& [guid, meta] : m_Assets)
	{
		if (meta->GetAssetType() == type)
			result.push_back(meta->GetAssetPath());
	}
	return result;
}

}
