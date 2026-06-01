#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <stb_image.h>

namespace Quentlam
{

struct AtlasEntry
{
	std::string Name;
	std::string SourcePath;
	int32_t Width = 0;
	int32_t Height = 0;
	glm::vec2 UVMin = { 0.0f, 0.0f };
	glm::vec2 UVMax = { 1.0f, 1.0f };
	int32_t X = 0;
	int32_t Y = 0;
};

struct AtlasPackerConfig
{
	int32_t AtlasWidth = 1024;
	int32_t AtlasHeight = 1024;
	int32_t Padding = 2;
	bool AllowRotation = false;
};

struct RectNode
{
	int32_t X = 0, Y = 0;
	int32_t Width = 0, Height = 0;
	bool Used = false;
	RectNode* Right = nullptr;
	RectNode* Down = nullptr;
};

class SpriteAtlasPacker
{
public:
	SpriteAtlasPacker() = default;
	explicit SpriteAtlasPacker(const AtlasPackerConfig& config);
	~SpriteAtlasPacker();

	bool AddImage(const std::string& path);
	bool AddImage(const std::string& name, const std::string& path);
	bool Pack();
	bool SaveAtlas(const std::string& outputPath);
	Ref<Texture2D> GenerateAtlas();
	bool SaveMetadata(const std::string& outputPath);

	Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
	const std::vector<AtlasEntry>& GetEntries() const { return m_Entries; }
	const AtlasPackerConfig& GetConfig() const { return m_Config; }
	bool IsPacked() const { return m_Packed; }
	int32_t GetUsedWidth() const { return m_UsedWidth; }
	int32_t GetUsedHeight() const { return m_UsedHeight; }

	void Clear();
	void SetConfig(const AtlasPackerConfig& config);

private:
	bool InsertNode(RectNode* node, int32_t width, int32_t height);
	RectNode* FindBestFit(RectNode* node, int32_t width, int32_t height);

	AtlasPackerConfig m_Config;
	std::vector<AtlasEntry> m_Entries;
	RectNode* m_Root = nullptr;
	Ref<Texture2D> m_AtlasTexture;
	bool m_Packed = false;
	int32_t m_UsedWidth = 0;
	int32_t m_UsedHeight = 0;
};

inline SpriteAtlasPacker::SpriteAtlasPacker(const AtlasPackerConfig& config)
	: m_Config(config)
{
	m_Root = new RectNode{ 0, 0, m_Config.AtlasWidth, m_Config.AtlasHeight, false, nullptr, nullptr };
}

inline SpriteAtlasPacker::~SpriteAtlasPacker()
{
	delete m_Root;
}

inline void SpriteAtlasPacker::Clear()
{
	m_Entries.clear();
	delete m_Root;
	m_Root = new RectNode{ 0, 0, m_Config.AtlasWidth, m_Config.AtlasHeight, false, nullptr, nullptr };
	m_AtlasTexture = nullptr;
	m_Packed = false;
	m_UsedWidth = 0;
	m_UsedHeight = 0;
}

inline void SpriteAtlasPacker::SetConfig(const AtlasPackerConfig& config)
{
	m_Config = config;
	Clear();
}

inline bool SpriteAtlasPacker::AddImage(const std::string& path)
{
	std::filesystem::path p(path);
	return AddImage(p.stem().string(), path);
}

inline bool SpriteAtlasPacker::AddImage(const std::string& name, const std::string& path)
{
	stbi_set_flip_vertically_on_load(0);
	int w = 0, h = 0, comp = 0;
	if (!stbi_info(path.c_str(), &w, &h, &comp))
		return false;

	AtlasEntry entry;
	entry.Name = name;
	entry.SourcePath = path;
	entry.Width = w;
	entry.Height = h;
	m_Entries.push_back(entry);
	return true;
}

inline RectNode* SpriteAtlasPacker::FindBestFit(RectNode* node, int32_t width, int32_t height)
{
	if (!node || node->Used)
		return nullptr;

	if (node->Width >= width && node->Height >= height)
	{
		int32_t rightW = node->Width - width;
		int32_t downH = node->Height - height;

		if (rightW >= m_Config.Padding && node->Right)
		{
			auto* rightFit = FindBestFit(node->Right, width, height);
			if (rightFit)
				return rightFit;
		}

		if (downH >= m_Config.Padding && node->Down)
		{
			auto* downFit = FindBestFit(node->Down, width, height);
			if (downFit)
				return downFit;
		}

		return node;
	}

	auto* rightBest = node->Right ? FindBestFit(node->Right, width, height) : nullptr;
	auto* downBest = node->Down ? FindBestFit(node->Down, width, height) : nullptr;

	if (rightBest && downBest)
	{
		int32_t rightArea = rightBest->Width * rightBest->Height;
		int32_t downArea = downBest->Width * downBest->Height;
		return (rightArea <= downArea) ? rightBest : downBest;
	}

	return rightBest ? rightBest : downBest;
}

inline bool SpriteAtlasPacker::InsertNode(RectNode* node, int32_t width, int32_t height)
{
	if (!node || node->Used)
		return false;

	if (node->Width < width || node->Height < height)
		return false;

	if (node->Width == width && node->Height == height)
	{
		node->Used = true;
		return true;
	}

	int32_t dw = node->Width - width;
	int32_t dh = node->Height - height;

	if (!node->Right && dw >= m_Config.Padding)
	{
		node->Right = new RectNode{ node->X + width, node->Y, dw, node->Height, false, nullptr, nullptr };
	}
	if (!node->Down && dh >= m_Config.Padding)
	{
		node->Down = new RectNode{ node->X, node->Y + height, width, dh, false, nullptr, nullptr };
	}

	if (node->Right && node->Right->Width >= width && node->Right->Height >= height)
	{
		if (InsertNode(node->Right, width, height))
			return true;
	}
	if (node->Down && node->Down->Width >= width && node->Down->Height >= height)
	{
		if (InsertNode(node->Down, width, height))
			return true;
	}

	return false;
}

}
