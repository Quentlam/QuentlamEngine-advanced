#include "qlpch.h"
#include "MaterialEditorPanel.h"
#include "Quentlam/Renderer/material.h"
#include "Quentlam/Renderer/Shader.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Core/Log.h"
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

namespace Quentlam
{
	bool MaterialEditorPanel::LoadShaderSourceFromFile(const std::string& filepath,
		std::string& outVertexSrc, std::string& outFragmentSrc,
		std::string& outError)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			outError = "Failed to open file: " + filepath;
			return false;
		}

		std::stringstream ss;
		ss << file.rdbuf();
		std::string fullSrc = ss.str();
		file.close();

		std::string vertSrc, fragSrc;
		std::string mode = "none";

		size_t pos = 0;
		while (pos < fullSrc.size())
		{
			size_t lineStart = fullSrc.find_first_not_of("\r\n", pos);
			if (lineStart == std::string::npos) break;

			size_t lineEnd = fullSrc.find('\n', lineStart);
			if (lineEnd == std::string::npos) lineEnd = fullSrc.size();

			std::string line = fullSrc.substr(lineStart, lineEnd - lineStart);

			size_t typePos = line.find("#type");
			if (typePos != std::string::npos)
			{
				size_t namePos = line.find_first_not_of(" \t", typePos + 5);
				std::string type = line.substr(namePos);
				size_t endNonSpace = type.find_last_not_of(" \t");
				if (endNonSpace != std::string::npos)
					type = type.substr(0, endNonSpace + 1);

				if (type == "vertex")
					mode = "vertex";
				else if (type == "fragment")
					mode = "fragment";
				else
					mode = "none";
			}
			else
			{
				if (mode == "vertex")
					vertSrc += line + "\n";
				else if (mode == "fragment")
					fragSrc += line + "\n";
			}

			pos = lineEnd;
		}

		if (vertSrc.empty() || fragSrc.empty())
		{
			outError = "Shader must contain both #type vertex and #type fragment sections";
			return false;
		}

		outVertexSrc = vertSrc;
		outFragmentSrc = fragSrc;
		return true;
	}

	void MaterialEditorPanel::OnImGuiRender()
	{
		if (!m_IsOpen)
			return;

		ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("材质编辑器 (Material Editor)", &m_IsOpen))
		{
			ImGui::End();
			return;
		}

		float panelWidth = ImGui::GetWindowWidth() * 0.25f;
		ImGui::BeginChild("##MaterialList", ImVec2(panelWidth, 0), true);
		DrawMaterialList();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("##MaterialDetails", ImVec2(0, 0), false);
		if (m_SelectedMaterial)
		{
			DrawMaterialProperties(m_SelectedMaterial);

			ImGui::Separator();
			ImGui::Text("Shader 配置");

			char pathBuf[512] = {};
			strncpy_s(pathBuf, m_SelectedShaderPath.c_str(), sizeof(pathBuf) - 1);
			ImGui::InputText("Shader 文件路径", pathBuf, sizeof(pathBuf),
				ImGuiInputTextFlags_ReadOnly);

			ImGui::SameLine();
			if (ImGui::Button("从磁盘热重载"))
			{
				if (m_SelectedShaderPath.empty())
				{
					QL_CORE_WARN("No shader file path set. Cannot hot reload.");
				}
				else
				{
					m_HasCompileError = false;
					m_ShaderDirty = false;
					std::string vertSrc, fragSrc, error;
					auto shader = m_SelectedMaterial->GetShader();
					if (LoadShaderSourceFromFile(m_SelectedShaderPath, vertSrc, fragSrc, error))
					{
						auto newShader = Shader::Create(
							shader ? (shader->GetName() + "_hot") : "HotShader",
							vertSrc, fragSrc);
						if (newShader)
						{
							m_SelectedMaterial->SetShader(newShader);
							QL_CORE_INFO("Shader hot-reloaded: {0}", m_SelectedShaderPath);
						}
						else
						{
							m_HasCompileError = true;
							m_LastCompileError = "Compilation failed after reload.";
						}
					}
					else
					{
						m_HasCompileError = true;
						m_LastCompileError = error;
					}
				}
			}

			if (m_HasCompileError)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "编译错误:");
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_LastCompileError.c_str());
			}

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
				"提示: 在外部编辑器修改 .glsl 文件后，点击\"从磁盘热重载\"重新编译 Shader");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
				"选择一个材质查看属性，或在下方创建新材质。");
		}
		ImGui::EndChild();

		ImGui::End();
	}

	void MaterialEditorPanel::DrawMaterialList()
	{
		ImGui::Text("材质列表");
		ImGui::Separator();

		auto drawMatItem = [&](const char* name, Ref<Material> mat) {
			bool isSelected = (m_SelectedMaterial == mat);
			if (ImGui::Selectable(name, isSelected))
			{
				m_SelectedMaterial = mat;
				m_SelectedShaderPath.clear();
				m_ShaderDirty = false;
				m_HasCompileError = false;
			}
		};

		drawMatItem("DefaultSprite", MaterialDefaults::GetSpriteMaterial());
		drawMatItem("FlatColor", MaterialDefaults::GetFlatColorMaterial());

		ImGui::Separator();
		ImGui::Text("创建新材质:");

		static char shaderPathBuf[256] = "assets/shaders/Texture2DShader.glsl";
		static char matNameBuf[64] = "MyMaterial";

		ImGui::InputText("Shader 路径", shaderPathBuf, sizeof(shaderPathBuf));
		ImGui::InputText("材质名称", matNameBuf, sizeof(matNameBuf));

		if (ImGui::Button("Create", ImVec2(-1, 0)))
		{
			if (strlen(shaderPathBuf) > 0)
			{
				auto shader = Shader::Create(shaderPathBuf);
				if (shader)
				{
					auto newMat = Material::Create(shader);
					newMat->SetName(strlen(matNameBuf) > 0 ? matNameBuf : "NewMaterial");
					m_SelectedMaterial = newMat;
					m_SelectedShaderPath = shaderPathBuf;
					m_ShaderDirty = false;
					m_HasCompileError = false;
					QL_CORE_INFO("Created material: {0} with shader: {1}", newMat->GetName(), shaderPathBuf);
				}
				else
				{
					QL_CORE_ERROR("Failed to create shader from: {0}", shaderPathBuf);
					m_HasCompileError = true;
					m_LastCompileError = "Shader compilation failed. Check the shader file path and syntax.";
				}
			}
		}
	}

	void MaterialEditorPanel::DrawMaterialProperties(Ref<Material> mat)
	{
		ImGui::Text("材质: %s", mat->GetName().c_str());
		auto shader = mat->GetShader();
		ImGui::Text("Shader: %s", shader ? shader->GetName().c_str() : "None");
		ImGui::Spacing();

		if (m_HasCompileError)
		{
			ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Error:");
			ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%s", m_LastCompileError.c_str());
			ImGui::Spacing();
		}

		ImGui::Separator();
		ImGui::Text("内置属性");

		glm::vec4 color(1.0f);
		auto it = mat->GetVec4Uniforms().find("u_Color");
		if (it != mat->GetVec4Uniforms().end())
			color = it->second;
		if (ImGui::ColorEdit4("Color (u_Color)", glm::value_ptr(color)))
			mat->SetVec4("u_Color", color);

		float tiling = 1.0f;
		auto itTiling = mat->GetFloatUniforms().find("u_TilingFactor");
		if (itTiling != mat->GetFloatUniforms().end())
			tiling = itTiling->second;
		if (ImGui::DragFloat("Tiling (u_TilingFactor)", &tiling, 0.1f, 0.1f, 100.0f))
			mat->SetFloat("u_TilingFactor", tiling);
	}

	void MaterialEditorPanel::DrawShaderSourceEditor(Ref<Shader> shader, Ref<Material> mat)
	{
		if (!shader)
		{
			ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "No shader assigned.");
			return;
		}

		ImGui::Text("Shader: %s", shader->GetName().c_str());

		if (ImGui::Button("从磁盘热重载 (Hot Reload)"))
		{
			if (m_SelectedShaderPath.empty())
			{
				QL_CORE_WARN("No shader file path set. Cannot hot reload.");
			}
			else
			{
				m_HasCompileError = false;
				m_ShaderDirty = false;
				std::string vertSrc, fragSrc, error;
				if (LoadShaderSourceFromFile(m_SelectedShaderPath, vertSrc, fragSrc, error))
				{
					auto newShader = Shader::Create(shader->GetName() + "_hot", vertSrc, fragSrc);
					if (newShader)
					{
						mat->SetShader(newShader);
						QL_CORE_INFO("Shader hot-reloaded: {0}", m_SelectedShaderPath);
					}
					else
					{
						m_HasCompileError = true;
						m_LastCompileError = "Compilation failed after reload.";
					}
				}
				else
				{
					m_HasCompileError = true;
					m_LastCompileError = error;
				}
			}
		}

		if (m_HasCompileError)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "编译错误:");
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_LastCompileError.c_str());
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
			"提示: 在外部编辑器修改 .glsl 文件后，点击\"从磁盘热重载\"重新编译 Shader");
	}
}
