#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Shader.h"
#include "Quentlam/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace Quentlam
{
	class QUENTLAM_API Material
	{
	public:
		Material() = default;
		Material(Ref<Shader> shader);
		~Material() = default;

		void Bind() const;
		void Unbind() const;

		Ref<Shader> GetShader() const { return m_Shader; }
		void SetShader(Ref<Shader> shader) { m_Shader = shader; }

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		void SetFloat(const std::string& name, float value);
		void SetInt(const std::string& name, int value);
		void SetVec2(const std::string& name, const glm::vec2& value);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetVec4(const std::string& name, const glm::vec4& value);
		void SetMat4(const std::string& name, const glm::mat4& value);
		void SetTexture(const std::string& name, Ref<Texture2D> texture);

		template<typename T>
		T* GetUniform(const std::string& name);

		const std::unordered_map<std::string, float>& GetFloatUniforms() const { return m_FloatUniforms; }
		const std::unordered_map<std::string, int>& GetIntUniforms() const { return m_IntUniforms; }
		const std::unordered_map<std::string, glm::vec4>& GetVec4Uniforms() const { return m_Vec4Uniforms; }
		const std::unordered_map<std::string, glm::vec3>& GetVec3Uniforms() const { return m_Vec3Uniforms; }
		const std::unordered_map<std::string, glm::vec2>& GetVec2Uniforms() const { return m_Vec2Uniforms; }

		static Ref<Material> Create(Ref<Shader> shader);

	private:
		Ref<Shader> m_Shader;
		std::string m_Name;

		std::unordered_map<std::string, float> m_FloatUniforms;
		std::unordered_map<std::string, int> m_IntUniforms;
		std::unordered_map<std::string, glm::vec2> m_Vec2Uniforms;
		std::unordered_map<std::string, glm::vec3> m_Vec3Uniforms;
		std::unordered_map<std::string, glm::vec4> m_Vec4Uniforms;
		std::unordered_map<std::string, glm::mat4> m_Mat4Uniforms;
		std::unordered_map<std::string, Ref<Texture2D>> m_TextureUniforms;
		std::unordered_map<std::string, int> m_TextureSlots;
		int m_TextureSlotCount = 0;
	};

	class MaterialDefaults
	{
	public:
		static Ref<Material> GetSpriteMaterial();
		static Ref<Material> GetFlatColorMaterial();
		static const std::unordered_map<std::string, Ref<Material>>& GetAllDefaults();

	private:
		static Ref<Material> s_SpriteMaterial;
		static Ref<Material> s_FlatColorMaterial;
		static std::unordered_map<std::string, Ref<Material>> s_AllDefaults;
	};
}
