#include "qlpch.h"
#include "AtlasBuilderPanel.h"
#include "Quentlam/Core/Log.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <ShlObj.h>
#include <Commdlg.h>

namespace Quentlam
{

void AtlasBuilderPanel::OnImGuiRender()
{
	if (!ImGui::Begin("Atlas Builder", nullptr, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("图集设置", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int atlasW = m_Packer.GetConfig().AtlasWidth;
		int atlasH = m_Packer.GetConfig().AtlasHeight;
		int padding = m_Packer.GetConfig().Padding;

		ImGui::SetNextItemWidth(100);
		if (ImGui::InputInt("宽度", &atlasW, 64, 256))
			atlasW = std::clamp(atlasW, 64, 8192);
		ImGui::SetNextItemWidth(100);
		if (ImGui::InputInt("高度", &atlasH, 64, 256))
			atlasH = std::clamp(atlasH, 64, 8192);
		ImGui::SetNextItemWidth(100);
		if (ImGui::InputInt("间距", &padding, 1, 4))
			padding = std::clamp(padding, 0, 32);

		if (atlasW != m_Packer.GetConfig().AtlasWidth ||
			atlasH != m_Packer.GetConfig().AtlasHeight ||
			padding != m_Packer.GetConfig().Padding)
		{
			AtlasPackerConfig config;
			config.AtlasWidth = atlasW;
			config.AtlasHeight = atlasH;
			config.Padding = padding;
			m_Packer.SetConfig(config);
			m_AtlasGenerated = false;
		}
	}

	if (ImGui::CollapsingHeader("源图片", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto& entries = m_Packer.GetEntries();
		ImGui::Text("图片数量: %zu", entries.size());

		if (entries.empty())
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1), "还没有添加图片。点击[添加图片]按钮来添加源图片。");
		}

		for (size_t i = 0; i < entries.size(); ++i)
		{
			ImGui::BulletText("[%zu] %s (%dx%d) @ (%d,%d)",
				i, entries[i].Name.c_str(),
				entries[i].Width, entries[i].Height,
				entries[i].X, entries[i].Y);
		}

		ImGui::Spacing();

		if (ImGui::Button("添加图片..."))
		{
			OpenFolder();
		}

		ImGui::SameLine();

		if (ImGui::Button("清除全部"))
		{
			ClearImages();
		}
	}

	if (ImGui::CollapsingHeader("Pack & Export", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("打包图集"))
		{
			PackAtlas();
		}

		ImGui::SameLine();

		if (ImGui::Button("保存元数据..."))
		{
			SaveAtlas();
		}

		if (m_AtlasGenerated)
		{
			auto tex = m_Packer.GetAtlasTexture();
			if (tex)
			{
				ImGui::Separator();
				ImGui::Text("图集预览:");
				ImGui::Image((ImTextureID)(uintptr_t)tex->GetRendererID(),
					ImVec2(256, 256),
					ImVec2(0, 1), ImVec2(1, 0));
			}
		}
	}

	if (m_Packer.IsPacked())
	{
		if (ImGui::CollapsingHeader("精灵UV (for scripts)"))
		{
			const auto& entries = m_Packer.GetEntries();
			for (const auto& e : entries)
			{
				ImGui::BulletText("%s: uvMin(%.4f,%.4f) uvMax(%.4f,%.4f)",
					e.Name.c_str(),
					e.UVMin.x, e.UVMin.y,
					e.UVMax.x, e.UVMax.y);
			}
		}
	}

	ImGui::End();
}

void AtlasBuilderPanel::OpenFolder()
{
	if (ImGui::Button("打开文件夹..."))
	{
		char folderBuf[MAX_PATH] = { 0 };
		BROWSEINFOA bi = {};
		bi.lpszTitle = "选择包含图片的文件夹";
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl)
		{
			SHGetPathFromIDListA(pidl, folderBuf);
			CoTaskMemFree(pidl);

			std::string folder(folderBuf);
			if (!folder.empty())
			{
				WIN32_FIND_DATAA findData;
				std::string searchPath = folder + "\\*.png";
				HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

				if (hFind != INVALID_HANDLE_VALUE)
				{
					int added = 0;
					do
					{
						std::string fullPath = folder + "\\" + findData.cFileName;
						if (m_Packer.AddImage(fullPath))
							added++;
					} while (FindNextFileA(hFind, &findData));
					FindClose(hFind);
					QL_CORE_INFO("从文件夹添加了 {0} 张图片: {1}", added, folder);
					m_AtlasGenerated = false;
				}
				else
				{
					searchPath = folder + "\\*.jpg";
					hFind = FindFirstFileA(searchPath.c_str(), &findData);
					if (hFind != INVALID_HANDLE_VALUE)
					{
						int added = 0;
						do
						{
							std::string fullPath = folder + "\\" + findData.cFileName;
							if (m_Packer.AddImage(fullPath))
								added++;
						} while (FindNextFileA(hFind, &findData));
						FindClose(hFind);
						QL_CORE_INFO("从文件夹添加了 {0} 张图片: {1}", added, folder);
						m_AtlasGenerated = false;
					}
				}
			}
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("添加单张图片..."))
	{
		char fileBuf[MAX_PATH] = { 0 };
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = "图片文件\0*.png;*.jpg;*.jpeg;*.bmp\0所有文件\0*.*\0\0";
		ofn.lpstrFile = fileBuf;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
		ofn.lpstrTitle = "选择图片文件";
		if (GetOpenFileNameA(&ofn))
		{
			char* p = fileBuf;
			std::string dir(p);
			p += strlen(p) + 1;
			if (*p == '\0')
			{
				if (m_Packer.AddImage(dir))
				{
					QL_CORE_INFO("添加图片: {0}", dir);
					m_AtlasGenerated = false;
				}
			}
			else
			{
				int added = 0;
				while (*p)
				{
					std::string fullPath = dir + "\\" + std::string(p);
					if (m_Packer.AddImage(fullPath))
						added++;
					p += strlen(p) + 1;
				}
				QL_CORE_INFO("添加了 {0} 张图片", added);
				m_AtlasGenerated = false;
			}
		}
	}
}

void AtlasBuilderPanel::ClearImages()
{
	m_Packer.Clear();
	m_AtlasGenerated = false;
	QL_CORE_INFO("Atlas builder cleared");
}

void AtlasBuilderPanel::PackAtlas()
{
	if (m_Packer.GetEntries().empty())
	{
		QL_CORE_WARN("没有图片可以打包!");
		return;
	}

	if (m_Packer.Pack())
	{
		auto tex = m_Packer.GenerateAtlas();
		if (tex)
		{
			m_AtlasGenerated = true;
			QL_CORE_INFO("Atlas packed and generated successfully");
		}
	}
}

void AtlasBuilderPanel::SaveAtlas()
{
	if (!m_Packer.IsPacked())
	{
		QL_CORE_WARN("请先打包图集!");
		return;
	}

	char metaPathBuf[512] = { 0 };
	ImGui::InputText("Metadata JSON Path", metaPathBuf, sizeof(metaPathBuf));

	if (ImGui::Button("Save JSON"))
	{
		std::string path(metaPathBuf);
		if (!path.empty())
		{
			if (m_Packer.SaveMetadata(path))
			{
				QL_CORE_INFO("Atlas metadata saved to: {0}", path);
			}
		}
	}
}

}
