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
	enum class EAssetType : uint8_t
	{
		Unknown = 0,
		Texture = 1,
		Audio = 2,
		Font = 3,
		Model = 4,
		Shader = 5,
		Data = 6,
		Scene = 7,
		Animation = 8,
		FontData = 9
	};

	struct AssetMetadata
	{
		std::string AssetId;
		std::string SourcePath;
		std::string DisplayName;
		EAssetType Type = EAssetType::Unknown;
		std::string ContentType;
		uint64_t FileSize = 0;
		std::string Hash;
		bool IsVirtual = false;
		std::string VirtualPath;
	};

	class Importer
	{
	public:
		Importer() = default;
		virtual ~Importer() = default;

		virtual bool CanImport(const std::string& extension) const = 0;
		virtual bool Import(const std::string& sourcePath, const std::string& outputPath) = 0;
		virtual EAssetType GetAssetType() const = 0;
		virtual const char* GetName() const = 0;
	};

	class TextureImporter : public Importer
	{
	public:
		bool CanImport(const std::string& extension) const override;
		bool Import(const std::string& sourcePath, const std::string& outputPath) override;
		EAssetType GetAssetType() const override { return EAssetType::Texture; }
		const char* GetName() const override { return "TextureImporter"; }

		int MaxSize = 4096;
		bool GenerateMipmaps = true;
		bool IsNormalMap = false;
	};

	class AudioImporter : public Importer
	{
	public:
		bool CanImport(const std::string& extension) const override;
		bool Import(const std::string& sourcePath, const std::string& outputPath) override;
		EAssetType GetAssetType() const override { return EAssetType::Audio; }
		const char* GetName() const override { return "AudioImporter"; }

		bool ForceMono = false;
		float Volume = 1.0f;
	};

	struct AtlasRegion
	{
		std::string Name;
		int32_t X = 0, Y = 0;
		int32_t Width = 0, Height = 0;
		float PivotX = 0.5f, PivotY = 0.5f;
		std::string FrameId;
		bool Rotated = false;
	};

	struct AtlasDefinition
	{
		std::string AtlasId;
		std::string TexturePath;
		int32_t Width = 0;
		int32_t Height = 0;
		std::vector<AtlasRegion> Regions;
	};

	class QUENTLAM_API AtlasPacker
	{
	public:
		struct Options
		{
			int32_t MaxWidth = 2048;
			int32_t MaxHeight = 2048;
			int32_t Padding = 2;
			bool AllowRotation = false;
			bool PowerOfTwo = true;
			bool Square = false;
		};

		AtlasPacker() = default;
		AtlasPacker(const Options& options);

		void AddImage(const std::string& name, int32_t width, int32_t height, const void* pixels);
		bool Pack();
		const std::vector<AtlasDefinition>& GetAtlases() const { return m_Atlases; }
		const Options& GetOptions() const { return m_Options; }

		void ExportAtlasData(const std::string& path, const std::string& atlasId) const;
		bool LoadAtlasData(const std::string& path, AtlasDefinition& outAtlas) const;

	private:
		struct ImageEntry
		{
			std::string Name;
			int32_t Width, Height;
			const void* Pixels;
			int32_t AtlasIndex = -1;
			int32_t PackedX = 0, PackedY = 0;
			bool Rotated = false;
		};

		bool PackSingleAtlas(std::vector<ImageEntry>& entries, int32_t width, int32_t height);

		Options m_Options;
		std::vector<ImageEntry> m_Entries;
		std::vector<AtlasDefinition> m_Atlases;
	};

	class ContentManifest
	{
	public:
		ContentManifest() = default;
		ContentManifest(const std::string& id);

		const std::string& GetId() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }

		void AddAsset(const AssetMetadata& asset);
		void RemoveAsset(const std::string& assetId);
		const AssetMetadata* GetAsset(const std::string& assetId) const;
		const AssetMetadata* GetAssetByPath(const std::string& sourcePath) const;
		bool HasAsset(const std::string& assetId) const;

		const std::vector<AssetMetadata>& GetAllAssets() const { return m_Assets; }
		std::vector<const AssetMetadata*> GetAssetsByType(EAssetType type) const;

		void SetVersion(int32_t version) { m_Version = version; }
		int32_t GetVersion() const { return m_Version; }

		std::string Serialize() const;
		bool Deserialize(const std::string& data);

	private:
		std::string m_Id;
		int32_t m_Version = 1;
		std::vector<AssetMetadata> m_Assets;
	};

	class HotReloadWatcher
	{
	public:
		HotReloadWatcher() = default;
		~HotReloadWatcher();

		void Start(const std::string& directory);
		void Stop();
		void SetPollInterval(float intervalSeconds);

		void SetOnFileChanged(std::function<void(const std::string& path)> callback);
		void SetOnFileAdded(std::function<void(const std::string& path)> callback);
		void SetOnFileRemoved(std::function<void(const std::string& path)> callback);

		void Update();

	private:
		void ScanDirectory(const std::string& directory);
		uint64_t GetFileHash(const std::string& path);

		bool m_Running = false;
		float m_PollInterval = 1.0f;
		float m_Accumulator = 0.0f;
		std::string m_Directory;
		std::unordered_map<std::string, uint64_t> m_FileHashes;
		std::unordered_map<std::string, std::filesystem::file_time_type> m_FileTimes;

		std::function<void(const std::string&)> m_OnFileChanged;
		std::function<void(const std::string&)> m_OnFileAdded;
		std::function<void(const std::string&)> m_OnFileRemoved;
	};

	class AssetValidator
	{
	public:
		struct ValidationIssue
		{
			std::string AssetId;
			std::string Message;
			enum class Severity { Warning, Error } Severity;
		};

		AssetValidator() = default;

		std::vector<ValidationIssue> Validate(const AssetMetadata& asset);
		std::vector<ValidationIssue> ValidateAll(const ContentManifest& manifest);
		std::vector<ValidationIssue> ValidateDirectory(const std::string& directory);

		bool HasErrors(const std::vector<ValidationIssue>& issues);
		bool HasWarnings(const std::vector<ValidationIssue>& issues);
	};

	class ContentPipelineModule
	{
	public:
		ContentPipelineModule() = default;
		static ContentPipelineModule& Get();

		void Initialize();
		void Shutdown();

		void RegisterImporter(Ref<Importer> importer);
		void UnregisterImporter(const std::string& name);
		Importer* GetImporter(const std::string& name) const;
		Importer* GetImporterForExtension(const std::string& extension) const;

		bool ImportAsset(const std::string& sourcePath, const std::string& outputPath, const std::string& importerName = "");
		bool ImportAll(const std::string& sourceDir, const std::string& outputDir);

		ContentManifest& GetManifest() { return m_Manifest; }
		const ContentManifest& GetManifest() const { return m_Manifest; }

		const std::string& GetSourceDirectory() const { return m_SourceDirectory; }
		const std::string& GetOutputDirectory() const { return m_OutputDirectory; }
		void SetSourceDirectory(const std::string& dir) { m_SourceDirectory = dir; }
		void SetOutputDirectory(const std::string& dir) { m_OutputDirectory = dir; }

		bool BuildAtlas(const std::string& atlasId, const std::vector<std::string>& imagePaths);
		bool BuildAllAtlases(const std::string& configPath);

		HotReloadWatcher& GetWatcher() { return m_Watcher; }

		void SetOnImportProgress(std::function<void(float progress, const std::string& currentFile)> callback);
		void SetOnImportComplete(std::function<void(bool success, const std::string& message)> callback);

	private:
		ContentManifest m_Manifest;
		std::string m_SourceDirectory;
		std::string m_OutputDirectory;
		std::unordered_map<std::string, Ref<Importer>> m_Importers;
		HotReloadWatcher m_Watcher;

		std::function<void(float, const std::string&)> m_OnImportProgress;
		std::function<void(bool, const std::string&)> m_OnImportComplete;
	};

	inline ContentManifest::ContentManifest(const std::string& id)
		: m_Id(id) {}

	inline void ContentManifest::AddAsset(const AssetMetadata& asset)
	{
		for (auto& existing : m_Assets)
		{
			if (existing.AssetId == asset.AssetId)
			{
				existing = asset;
				return;
			}
		}
		m_Assets.push_back(asset);
	}

	inline void ContentManifest::RemoveAsset(const std::string& assetId)
	{
		m_Assets.erase(
			std::remove_if(m_Assets.begin(), m_Assets.end(),
				[&assetId](const AssetMetadata& a) { return a.AssetId == assetId; }),
			m_Assets.end()
		);
	}

	inline const AssetMetadata* ContentManifest::GetAsset(const std::string& assetId) const
	{
		for (const auto& asset : m_Assets)
			if (asset.AssetId == assetId) return &asset;
		return nullptr;
	}

	inline const AssetMetadata* ContentManifest::GetAssetByPath(const std::string& sourcePath) const
	{
		for (const auto& asset : m_Assets)
			if (asset.SourcePath == sourcePath) return &asset;
		return nullptr;
	}

	inline bool ContentManifest::HasAsset(const std::string& assetId) const
	{
		return GetAsset(assetId) != nullptr;
	}

	inline std::vector<const AssetMetadata*> ContentManifest::GetAssetsByType(EAssetType type) const
	{
		std::vector<const AssetMetadata*> result;
		for (const auto& asset : m_Assets)
			if (asset.Type == type) result.push_back(&asset);
		return result;
	}

	inline bool TextureImporter::CanImport(const std::string& extension) const
	{
		return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga";
	}

	inline bool AudioImporter::CanImport(const std::string& extension) const
	{
		return extension == ".wav" || extension == ".ogg" || extension == ".mp3" || extension == ".flac";
	}

	inline HotReloadWatcher::~HotReloadWatcher()
	{
		Stop();
	}

	inline uint64_t HotReloadWatcher::GetFileHash(const std::string& path)
	{
		try
		{
			auto ftime = std::filesystem::last_write_time(path);
			return static_cast<uint64_t>(ftime.time_since_epoch().count());
		}
		catch (...)
		{
			return 0;
		}
	}

	inline void HotReloadWatcher::Update()
	{
		if (!m_Running || m_Directory.empty()) return;

		m_Accumulator += m_PollInterval;
		if (m_Accumulator < m_PollInterval) return;
		m_Accumulator = 0.0f;

		for (const auto& [path, oldHash] : m_FileHashes)
		{
			uint64_t newHash = GetFileHash(path);
			if (newHash != oldHash)
			{
				m_FileHashes[path] = newHash;
				if (m_OnFileChanged)
					m_OnFileChanged(path);
			}
		}
	}

	inline ContentPipelineModule& ContentPipelineModule::Get()
	{
		static ContentPipelineModule instance;
		return instance;
	}

	inline void ContentPipelineModule::Initialize()
	{
		m_Manifest = ContentManifest("main");
	}

	inline void ContentPipelineModule::Shutdown()
	{
		m_Watcher.Stop();
		m_Importers.clear();
	}

	inline void ContentPipelineModule::RegisterImporter(Ref<Importer> importer)
	{
		if (importer)
			m_Importers[importer->GetName()] = importer;
	}

	inline void ContentPipelineModule::UnregisterImporter(const std::string& name)
	{
		m_Importers.erase(name);
	}

	inline Importer* ContentPipelineModule::GetImporter(const std::string& name) const
	{
		auto it = m_Importers.find(name);
		return it != m_Importers.end() ? it->second.get() : nullptr;
	}

	inline Importer* ContentPipelineModule::GetImporterForExtension(const std::string& extension) const
	{
		for (const auto& [name, importer] : m_Importers)
			if (importer->CanImport(extension)) return importer.get();
		return nullptr;
	}
}
