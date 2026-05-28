#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Material.h"
#include "Quentlam/Renderer/Shader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

namespace Quentlam
{
	class QUENTLAM_API MaterialEditorPanel
	{
	public:
		MaterialEditorPanel() = default;

		void OnImGuiRender();

		void Open() { m_IsOpen = true; }
		void Close() { m_IsOpen = false; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		void DrawMaterialList();
		void DrawMaterialProperties(Ref<Material> mat);
		void DrawShaderSourceEditor(Ref<Shader> shader, Ref<Material> mat);

		static bool LoadShaderSourceFromFile(const std::string& filepath,
			std::string& outVertexSrc, std::string& outFragmentSrc,
			std::string& outError);

		bool m_IsOpen = true;
		Ref<Material> m_SelectedMaterial;
		std::string m_SelectedShaderPath;
		bool m_ShaderDirty = false;
		std::string m_LastCompileError;
		bool m_HasCompileError = false;
	};
}
