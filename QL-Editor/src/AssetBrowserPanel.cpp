#include "AssetBrowserPanel.h"
#include "Quentlam/Resource/ResourceManager.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <algorithm>

namespace Quentlam
{

static const char* SupportedTextureExts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr", ".webp" };
static const char* SupportedAudioExts[] = { ".wav", ".mp3", ".ogg", ".flac" };

bool AssetBrowserPanel::IsSupportedTexture(const std::string& ext) const
{
	for (const char* e : SupportedTextureExts)
		if (ext == e) return true;
	return false;
}

bool AssetBrowserPanel::IsSupportedAudio(const std::string& ext) const
{
	for (const char* e : SupportedAudioExts)
		if (ext == e) return true;
	return false;
}

bool AssetBrowserPanel::IsSceneFile(const std::string& path) const
{
	return path.find(".scene") != std::string::npos;
}

bool AssetBrowserPanel::IsMaterialFile(const std::string& path) const
{
	return path.find(".qlmat") != std::string::npos;
}

AssetBrowserPanel::AssetType AssetBrowserPanel::GetAssetType(const std::filesystem::path& path) const
{
	if (std::filesystem::is_directory(path))
		return AssetType::Directory;

	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (IsSupportedTexture(ext)) return AssetType::Texture;
	if (IsSupportedAudio(ext)) return AssetType::Audio;

	std::string name = path.filename().string();
	std::transform(name.begin(), name.end(), name.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (name.find(".scene") != std::string::npos) return AssetType::Scene;
	if (name.find(".qlmat") != std::string::npos) return AssetType::Material;
	if (name.find(".glsl") != std::string::npos) return AssetType::Shader;
	if (name.find(".anim") != std::string::npos) return AssetType::Animation;
	if (name.find(".obj") != std::string::npos || name.find(".fbx") != std::string::npos ||
		name.find(".gltf") != std::string::npos || name.find(".glb") != std::string::npos)
		return AssetType::Model;

	return AssetType::Unknown;
}

const char* AssetBrowserPanel::GetAssetTypeLabel(AssetType type) const
{
	switch (type)
	{
	case AssetType::Directory:   return "Folder";
	case AssetType::Texture:     return "Texture";
	case AssetType::Audio:       return "Audio";
	case AssetType::Scene:       return "Scene";
	case AssetType::Material:    return "Material";
	case AssetType::Model:       return "Model";
	case AssetType::Shader:      return "Shader";
	case AssetType::Animation:   return "Animation";
	default:                      return "Unknown";
	}
}

void AssetBrowserPanel::Refresh()
{
	m_Assets.clear();
	m_Assets.reserve(256);

	std::filesystem::path dir(m_CurrentDirectory);
	if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
		return;

	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		const auto& path = entry.path();
		std::string filename = path.filename().string();

		if (m_SearchBuffer[0] != '\0')
		{
			std::string lowerName = filename;
			std::string lowerSearch = m_SearchBuffer;
			std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (lowerName.find(lowerSearch) == std::string::npos)
				continue;
		}

		AssetEntry asset;
		asset.Path = path.string();
		asset.Name = filename;
		asset.Extension = path.extension().string();
		asset.Type = GetAssetType(path);

		if (asset.Type == AssetType::Texture)
		{
			auto it = m_ThumbnailCache.find(asset.Path);
			if (it != m_ThumbnailCache.end())
			{
				asset.Thumbnail = it->second;
			}
			else
			{
				try
				{
					auto tex = ResourceManager::Load<Texture2D>(filename, asset.Path);
					if (tex)
					{
						m_ThumbnailCache[asset.Path] = tex;
						asset.Thumbnail = tex;
					}
				}
				catch (...) { }
			}
		}

		m_Assets.push_back(std::move(asset));
	}

	std::sort(m_Assets.begin(), m_Assets.end(), [](const AssetEntry& a, const AssetEntry& b)
		{
			if (a.Type != b.Type)
			{
				if (a.Type == AssetType::Directory) return true;
				if (b.Type == AssetType::Directory) return false;
			}
			return a.Name < b.Name;
		});

	m_LastRefreshTime = ImGui::GetTime();
}

void AssetBrowserPanel::OnImGuiRender()
{
	if (!m_IsOpen) return;

	ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Asset Browser", &m_IsOpen,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	if (m_CurrentDirectory.empty())
		m_CurrentDirectory = "assets";

	DrawToolbar();
	DrawBreadcrumb();

	if (ImGui::GetTime() - m_LastRefreshTime > 2.0 || m_Assets.empty())
		Refresh();

	ImGui::Separator();

	HandleDragDrop();

	if (m_IsGridView)
		DrawGridView();
	else
		DrawListView();

	ImGui::End();
}

void AssetBrowserPanel::DrawToolbar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

	if (ImGui::Button("Refresh"))
		Refresh();

	ImGui::SameLine();

	if (ImGui::Button(m_IsGridView ? "[=]" : "[+]"))
		m_IsGridView = !m_IsGridView;

	ImGui::SameLine();
	ImGui::Text("Search:");
	ImGui::SameLine();
	ImGui::PushItemWidth(200.0f);
	ImGui::InputText("##Search", m_SearchBuffer, sizeof(m_SearchBuffer));
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::Text("Thumb Size:");
	ImGui::SameLine();
	ImGui::PushItemWidth(80.0f);
	ImGui::SliderInt("##ThumbSize", &m_ThumbnailSize, 32, 128, "%d", ImGuiSliderFlags_None);
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();
}

void AssetBrowserPanel::DrawBreadcrumb()
{
	std::filesystem::path root("assets");
	std::filesystem::path current(m_CurrentDirectory);
	std::vector<std::filesystem::path> parts;

	for (auto parent = current; parent != current.root_path(); parent = parent.parent_path())
	{
		parts.push_back(parent);
		if (parent == root || parts.size() > 6)
			break;
	}
	std::reverse(parts.begin(), parts.end());

	ImGui::AlignTextToFramePadding();
	for (size_t i = 0; i < parts.size(); ++i)
	{
		std::string name = parts[i].filename().string();
		if (name.empty())
			name = parts[i].string();

		if (ImGui::Button(name.c_str()))
		{
			m_CurrentDirectory = parts[i].string();
			Refresh();
		}

		if (i < parts.size() - 1)
		{
			ImGui::SameLine();
			ImGui::Text(" > ");
			ImGui::SameLine();
		}
	}
}

void AssetBrowserPanel::DrawGridView()
{
	int columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (m_ThumbnailSize + 20.0f));
	if (columnCount < 1) columnCount = 1;

	ImGui::Columns(columnCount, "assetgrid", false);
	ImGui::SetColumnWidth(0, m_ThumbnailSize + 20.0f);

	for (const auto& asset : m_Assets)
	{
		if (asset.Type == AssetType::Directory)
		{
			ImGui::ImageButton(
				reinterpret_cast<void*>(static_cast<intptr_t>(0)),
				ImVec2(static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)),
				ImVec2(0, 0), ImVec2(1, 1),
				0,
				ImVec4(0.2f, 0.2f, 0.3f, 1.0f),
				ImVec4(0.4f, 0.7f, 1.0f, 1.0f)
			);
			if (ImGui::IsItemClicked())
			{
				m_CurrentDirectory = asset.Path;
				Refresh();
			}
		}
		else if (asset.Thumbnail)
		{
			ImGui::ImageButton(
				reinterpret_cast<void*>(static_cast<intptr_t>(asset.Thumbnail->GetRendererID())),
				ImVec2(static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)),
				ImVec2(0, 1), ImVec2(1, 0),
				0,
				ImVec4(0.1f, 0.1f, 0.15f, 1.0f),
				ImVec4(1, 1, 1, 1)
			);
			if (ImGui::IsItemClicked())
			{
				m_SelectedAssetPath = asset.Path;
			}

			if (m_SelectedAssetPath == asset.Path)
			{
				ImGui::GetWindowDrawList()->AddRect(
					ImGui::GetItemRectMin(),
					ImGui::GetItemRectMax(),
					ImGui::GetColorU32(ImGuiCol_ButtonHovered)
				);
			}
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
			ImGui::Button(asset.Name.c_str(),
				ImVec2(static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)));
			ImGui::PopStyleColor(2);

			if (ImGui::IsItemClicked())
			{
				m_SelectedAssetPath = asset.Path;
			}
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s\n%s\nType: %s",
				asset.Name.c_str(), asset.Path.c_str(), GetAssetTypeLabel(asset.Type));
		}

		if (!m_SelectedAssetPath.empty() && m_SelectedAssetPath == asset.Path)
		{
			ImGui::GetWindowDrawList()->AddRect(
				ImGui::GetItemRectMin(),
				ImGui::GetItemRectMax(),
				ImGui::GetColorU32(ImGuiCol_ButtonHovered)
			);
		}

		ImGui::TextWrapped("%s", asset.Name.c_str());
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
}

void AssetBrowserPanel::DrawListView()
{
	ImGui::Columns(4, "assetlist", true);
	ImGui::SetColumnWidth(0, 200.0f);
	ImGui::SetColumnWidth(1, 80.0f);
	ImGui::SetColumnWidth(2, 60.0f);
	ImGui::SetColumnWidth(3, 80.0f);

	ImGui::Text("Name"); ImGui::NextColumn();
	ImGui::Text("Type"); ImGui::NextColumn();
	ImGui::Text("Size"); ImGui::NextColumn();
	ImGui::Text("Modified"); ImGui::NextColumn();
	ImGui::Separator();

	for (const auto& asset : m_Assets)
	{
		bool selected = (m_SelectedAssetPath == asset.Path);

		if (asset.Type == AssetType::Directory)
		{
			ImGui::Selectable(asset.Name.c_str(), &selected);
			if (ImGui::IsItemClicked())
			{
				m_SelectedAssetPath = asset.Path;
				m_CurrentDirectory = asset.Path;
				Refresh();
			}
		}
		else
		{
			ImGui::Selectable(asset.Name.c_str(), &selected);
			if (ImGui::IsItemClicked())
			{
				m_SelectedAssetPath = asset.Path;
			}
		}
		ImGui::NextColumn();
		ImGui::Text("%s", GetAssetTypeLabel(asset.Type));
		ImGui::NextColumn();

		if (asset.Type != AssetType::Directory)
		{
			try
			{
				auto size = std::filesystem::file_size(asset.Path);
				if (size < 1024)
					ImGui::Text("%llu B", size);
				else if (size < 1024 * 1024)
					ImGui::Text("%.1f KB", size / 1024.0f);
				else
					ImGui::Text("%.1f MB", size / (1024.0f * 1024.0f));
			}
			catch (...) { ImGui::Text("-"); }
		}
		else
		{
			ImGui::Text("-");
		}
		ImGui::NextColumn();

		try
		{
			auto time = std::filesystem::last_write_time(asset.Path);
			auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
			std::time_t ctime = std::chrono::system_clock::to_time_t(sctp);
			char buf[32];
			tm tmBuf;
			localtime_s(&tmBuf, &ctime);
			strftime(buf, sizeof(buf), "%Y-%m-%d", &tmBuf);
			ImGui::Text("%s", buf);
		}
		catch (...) { ImGui::Text("-"); }
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
}

void AssetBrowserPanel::HandleDragDrop()
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			const char* path = static_cast<const char*>(payload->Data);
			if (path && std::filesystem::is_directory(path))
			{
				m_CurrentDirectory = path;
				Refresh();
			}
		}
		ImGui::EndDragDropTarget();
	}
}

} // namespace Quentlam
