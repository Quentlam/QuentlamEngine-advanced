#include "qlpch.h"
#include "SpriteAtlasPacker.h"
#include <fstream>
#include <stb_image.h>

namespace Quentlam
{

bool SpriteAtlasPacker::Pack()
{
	if (m_Entries.empty())
		return false;

	m_Packed = false;
	delete m_Root;
	m_Root = new RectNode{ 0, 0, m_Config.AtlasWidth, m_Config.AtlasHeight, false, nullptr, nullptr };

	std::sort(m_Entries.begin(), m_Entries.end(),
		[](const AtlasEntry& a, const AtlasEntry& b) {
			return a.Width * a.Height > b.Width * b.Height;
		});

	m_UsedWidth = 0;
	m_UsedHeight = 0;

	for (auto& entry : m_Entries)
	{
		int32_t paddedW = entry.Width + m_Config.Padding * 2;
		int32_t paddedH = entry.Height + m_Config.Padding * 2;

		RectNode* fit = FindBestFit(m_Root, paddedW, paddedH);
		if (!fit)
		{
			QL_CORE_WARN("Atlas packing failed: not enough space for '{0}' ({1}x{2})",
				entry.Name, entry.Width, entry.Height);
			continue;
		}

		fit->Used = true;
		entry.X = fit->X + m_Config.Padding;
		entry.Y = fit->Y + m_Config.Padding;

		m_UsedWidth = std::max(m_UsedWidth, entry.X + entry.Width);
		m_UsedHeight = std::max(m_UsedHeight, entry.Y + entry.Height);

		float invAtlasW = 1.0f / static_cast<float>(m_Config.AtlasWidth);
		float invAtlasH = 1.0f / static_cast<float>(m_Config.AtlasHeight);
		entry.UVMin = {
			static_cast<float>(entry.X) * invAtlasW,
			static_cast<float>(entry.Y) * invAtlasH
		};
		entry.UVMax = {
			static_cast<float>(entry.X + entry.Width) * invAtlasW,
			static_cast<float>(entry.Y + entry.Height) * invAtlasH
		};

		int32_t dw = fit->Width - paddedW;
		int32_t dh = fit->Height - paddedH;
		if (dw >= m_Config.Padding && dh >= m_Config.Padding)
		{
			fit->Right = new RectNode{ fit->X + paddedW, fit->Y, dw, paddedH, false, nullptr, nullptr };
			fit->Down = new RectNode{ fit->X, fit->Y + paddedH, paddedW, dh, false, nullptr, nullptr };
		}
		else if (dw >= m_Config.Padding)
		{
			fit->Right = new RectNode{ fit->X + paddedW, fit->Y, dw, fit->Height, false, nullptr, nullptr };
		}
		else if (dh >= m_Config.Padding)
		{
			fit->Down = new RectNode{ fit->X, fit->Y + paddedH, fit->Width, dh, false, nullptr, nullptr };
		}
	}

	m_Packed = true;
	QL_CORE_INFO("Atlas packed: {0} sprites, {1}x{2}", m_Entries.size(), m_UsedWidth, m_UsedHeight);
	return true;
}

Ref<Texture2D> SpriteAtlasPacker::GenerateAtlas()
{
	if (!m_Packed || m_Entries.empty())
		return nullptr;

	m_AtlasTexture = Texture2D::Create(m_Config.AtlasWidth, m_Config.AtlasHeight);
	std::vector<uint32_t> pixels(static_cast<size_t>(m_Config.AtlasWidth) * static_cast<size_t>(m_Config.AtlasHeight), 0x00000000);

	for (const auto& entry : m_Entries)
	{
		int w = entry.Width;
		int h = entry.Height;

		stbi_set_flip_vertically_on_load(1);
		int comp = 0;
		uint8_t* data = stbi_load(entry.SourcePath.c_str(), &w, &h, &comp, STBI_rgb_alpha);
		if (!data)
		{
			QL_CORE_WARN("Failed to load image for atlas: {0}", entry.SourcePath);
			continue;
		}

		for (int py = 0; py < h && py + entry.Y < m_Config.AtlasHeight; ++py)
		{
			for (int px = 0; px < w && px + entry.X < m_Config.AtlasWidth; ++px)
			{
				int srcIdx = (py * w + px) * 4;
				int dstIdx = ((entry.Y + py) * m_Config.AtlasWidth + (entry.X + px));
				pixels[dstIdx] = (static_cast<uint32_t>(data[srcIdx + 3]) << 24) |
					(static_cast<uint32_t>(data[srcIdx + 0]) << 16) |
					(static_cast<uint32_t>(data[srcIdx + 1]) << 8) |
					static_cast<uint32_t>(data[srcIdx + 2]);
			}
		}

		stbi_image_free(data);
	}

	m_AtlasTexture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size() * sizeof(uint32_t)));
	QL_CORE_INFO("Atlas texture generated: {0}x{1}", m_Config.AtlasWidth, m_Config.AtlasHeight);
	return m_AtlasTexture;
}

bool SpriteAtlasPacker::SaveMetadata(const std::string& outputPath)
{
	if (!m_Packed)
		return false;

	std::ofstream ofs(outputPath);
	if (!ofs.is_open())
		return false;

	ofs << "{\n";
	ofs << "  \"atlas\": {\n";
	ofs << "    \"width\": " << m_Config.AtlasWidth << ",\n";
	ofs << "    \"height\": " << m_Config.AtlasHeight << ",\n";
	ofs << "    \"usedWidth\": " << m_UsedWidth << ",\n";
	ofs << "    \"usedHeight\": " << m_UsedHeight << "\n";
	ofs << "  },\n";
	ofs << "  \"sprites\": [\n";

	for (size_t i = 0; i < m_Entries.size(); ++i)
	{
		const auto& e = m_Entries[i];
		ofs << "    {\n";
		ofs << "      \"name\": \"" << e.Name << "\",\n";
		ofs << "      \"x\": " << e.X << ",\n";
		ofs << "      \"y\": " << e.Y << ",\n";
		ofs << "      \"width\": " << e.Width << ",\n";
		ofs << "      \"height\": " << e.Height << ",\n";
		ofs << "      \"uvMin\": [" << e.UVMin.x << ", " << e.UVMin.y << "],\n";
		ofs << "      \"uvMax\": [" << e.UVMax.x << ", " << e.UVMax.y << "]\n";
		ofs << "    }" << (i + 1 < m_Entries.size() ? "," : "") << "\n";
	}

	ofs << "  ]\n";
	ofs << "}\n";
	ofs.close();

	QL_CORE_INFO("Atlas metadata saved to: {0}", outputPath);
	return true;
}

}
