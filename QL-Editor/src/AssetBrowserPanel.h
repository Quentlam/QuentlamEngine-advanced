#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Texture.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

namespace Quentlam
{
	class QUENTLAM_API AssetBrowserPanel
	{
	public:
		AssetBrowserPanel() = default;

		void OnImGuiRender();
		void Refresh();

		bool IsOpen() const { return m_IsOpen; }
		void SetOpen(bool open) { m_IsOpen = open; }

		const std::string& GetSelectedAssetPath() const { return m_SelectedAssetPath; }
		void ClearSelection() { m_SelectedAssetPath.clear(); }

	private:
		void DrawToolbar();
		void DrawGridView();
		void DrawListView();
		void DrawBreadcrumb();
		void HandleDragDrop();

		bool IsSupportedTexture(const std::string& ext) const;
		bool IsSupportedAudio(const std::string& ext) const;
		bool IsSceneFile(const std::string& path) const;
		bool IsMaterialFile(const std::string& path) const;

		enum class AssetType
		{
			Directory,
			Texture,
			Audio,
			Scene,
			Material,
			Model,
			Shader,
			Animation,
			Unknown
		};

		struct AssetEntry
		{
			std::string Path;
			std::string Name;
			std::string Extension;
			AssetType Type = AssetType::Unknown;
			Ref<Texture2D> Thumbnail;
			bool IsFavorite = false;
		};

		AssetType GetAssetType(const std::filesystem::path& path) const;
		const char* GetAssetTypeLabel(AssetType type) const;

		bool m_IsOpen = true;
		bool m_IsGridView = true;
		std::string m_CurrentDirectory;
		std::vector<AssetEntry> m_Assets;
		std::string m_SelectedAssetPath;
		char m_SearchBuffer[128] = { 0 };
		char m_FilterBuffer[128] = { 0 };
		int m_ThumbnailSize = 64;
		double m_LastRefreshTime = 0.0;
		std::vector<std::string> m_Favorites;
		std::unordered_map<std::string, Ref<Texture2D>> m_ThumbnailCache;
	};
}
