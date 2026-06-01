#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Quentlam
{

class AssetMetaFile
{
public:
	AssetMetaFile() = default;
	AssetMetaFile(const std::string& guid, const std::string& assetPath);

	const std::string& GetGUID() const { return m_GUID; }
	void SetGUID(const std::string& guid) { m_GUID = guid; }

	const std::string& GetAssetPath() const { return m_AssetPath; }
	void SetAssetPath(const std::string& path) { m_AssetPath = path; }

	const std::string& GetAssetType() const { return m_AssetType; }
	void SetAssetType(const std::string& type) { m_AssetType = type; }

	uint64_t GetPID() const { return m_PID; }
	void SetPID(uint64_t pid) { m_PID = pid; }

	uint64_t GetInstanceID() const { return m_InstanceID; }
	void SetInstanceID(uint64_t id) { m_InstanceID = id; }

	float GetImportTime() const { return m_ImportTime; }
	void SetImportTime(float t) { m_ImportTime = t; }

	float GetLastWriteTime() const { return m_LastWriteTime; }
	void SetLastWriteTime(float t) { m_LastWriteTime = t; }

	bool SerializeToFile(const std::string& metaPath) const;
	bool DeserializeFromFile(const std::string& metaPath);
	std::string SerializeToString() const;
	bool DeserializeFromString(const std::string& json);

	static std::string GenerateGUID();
	static std::string GetMetaPath(const std::string& assetPath);

private:
	std::string m_GUID;
	std::string m_AssetPath;
	std::string m_AssetType;
	uint64_t m_PID = 0;
	uint64_t m_InstanceID = 0;
	float m_ImportTime = 0.0f;
	float m_LastWriteTime = 0.0f;
};

class AssetDatabase
{
public:
	static AssetDatabase& Get();

	bool Initialize(const std::string& projectPath);
	void Shutdown();

	const std::string& GetProjectPath() const { return m_ProjectPath; }

	bool RegisterAsset(const std::string& guid, const std::string& path, const std::string& type);
	bool UnregisterAsset(const std::string& guid);
	bool UnregisterByPath(const std::string& path);

	bool HasAsset(const std::string& guid) const;
	bool HasAssetByPath(const std::string& path) const;

	const std::string* GetAssetPath(const std::string& guid) const;
	const std::string* GetAssetGUID(const std::string& path) const;
	const std::string* GetAssetType(const std::string& guid) const;

	bool Refresh();
	bool RefreshAsset(const std::string& path);
	bool RefreshDirectory(const std::string& dirPath);

	bool CreateMetaForAsset(const std::string& assetPath);
	bool DeleteMetaForAsset(const std::string& assetPath);
	Ref<AssetMetaFile> GetMeta(const std::string& guid) const;
	Ref<AssetMetaFile> GetMetaByPath(const std::string& path) const;

	bool RenameAsset(const std::string& oldPath, const std::string& newPath);
	bool MoveAsset(const std::string& oldPath, const std::string& newPath);
	bool DeleteAsset(const std::string& path);

	const std::unordered_map<std::string, Ref<AssetMetaFile>>& GetAllAssets() const { return m_Assets; }

	std::vector<std::string> GetAssetsByType(const std::string& type) const;

private:
	AssetDatabase() = default;
	~AssetDatabase() = default;

	std::string m_ProjectPath;
	std::unordered_map<std::string, Ref<AssetMetaFile>> m_Assets;
	std::unordered_map<std::string, std::string> m_PathToGUID;
};

}
