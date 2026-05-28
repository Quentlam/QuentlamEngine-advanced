#include "qlpch.h"
#include "Material.h"
#include "Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Quentlam
{
	Ref<Material> MaterialDefaults::s_SpriteMaterial;
	Ref<Material> MaterialDefaults::s_FlatColorMaterial;
	std::unordered_map<std::string, Ref<Material>> MaterialDefaults::s_AllDefaults;

	Material::Material(Ref<Shader> shader)
		: m_Shader(shader)
	{
	}

	void Material::Bind() const
	{
		if (!m_Shader)
			return;

		m_Shader->Bind();

		for (auto& [name, value] : m_FloatUniforms)
			m_Shader->SetFloat(name, value);
		for (auto& [name, value] : m_IntUniforms)
			m_Shader->SetInt(name, value);
		for (auto& [name, value] : m_Vec2Uniforms)
			m_Shader->SetFloat2(name, value);
		for (auto& [name, value] : m_Vec3Uniforms)
			m_Shader->SetFloat3(name, value);
		for (auto& [name, value] : m_Vec4Uniforms)
			m_Shader->SetFloat4(name, value);
		for (auto& [name, value] : m_Mat4Uniforms)
			m_Shader->SetMat4(name, value);

		int slot = 0;
		for (auto& [name, texture] : m_TextureUniforms)
		{
			texture->Bind(slot);
			m_Shader->SetInt(name, slot);
			slot++;
		}
	}

	void Material::Unbind() const
	{
		if (m_Shader)
			m_Shader->Unbind();
	}

	void Material::SetFloat(const std::string& name, float value)
	{
		m_FloatUniforms[name] = value;
	}

	void Material::SetInt(const std::string& name, int value)
	{
		m_IntUniforms[name] = value;
	}

	void Material::SetVec2(const std::string& name, const glm::vec2& value)
	{
		m_Vec2Uniforms[name] = value;
	}

	void Material::SetVec3(const std::string& name, const glm::vec3& value)
	{
		m_Vec3Uniforms[name] = value;
	}

	void Material::SetVec4(const std::string& name, const glm::vec4& value)
	{
		m_Vec4Uniforms[name] = value;
	}

	void Material::SetMat4(const std::string& name, const glm::mat4& value)
	{
		m_Mat4Uniforms[name] = value;
	}

	void Material::SetTexture(const std::string& name, Ref<Texture2D> texture)
	{
		m_TextureUniforms[name] = texture;
	}

	Ref<Material> Material::Create(Ref<Shader> shader)
	{
		return CreateRef<Material>(shader);
	}

	Ref<Material> MaterialDefaults::GetSpriteMaterial()
	{
		if (!s_SpriteMaterial)
		{
			s_SpriteMaterial = Material::Create(Shader::Create("assets/shaders/Texture2DShader.glsl"));
			s_SpriteMaterial->SetName("DefaultSprite");
			s_SpriteMaterial->SetVec4("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			s_SpriteMaterial->SetFloat("u_TilingFactor", 1.0f);
		}
		return s_SpriteMaterial;
	}

	Ref<Material> MaterialDefaults::GetFlatColorMaterial()
	{
		if (!s_FlatColorMaterial)
		{
			s_FlatColorMaterial = Material::Create(Shader::Create("assets/shaders/FlatColorShader.glsl"));
			s_FlatColorMaterial->SetName("FlatColor");
			s_FlatColorMaterial->SetVec4("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
		return s_FlatColorMaterial;
	}

	const std::unordered_map<std::string, Ref<Material>>& MaterialDefaults::GetAllDefaults()
	{
		if (s_AllDefaults.empty())
		{
			s_AllDefaults["DefaultSprite"] = GetSpriteMaterial();
			s_AllDefaults["FlatColor"] = GetFlatColorMaterial();
		}
		return s_AllDefaults;
	}
}
