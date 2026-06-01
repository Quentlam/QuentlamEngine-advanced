#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Shader.h"
#include <glm/glm.hpp>

namespace Quentlam
{
	class CubemapTexture;

	enum class SkyMode
	{
		Procedural,
		Cubemap
	};

	class SkyRenderer
	{
	public:
		SkyRenderer();

		void Init();
		void SetTimeOfDay(float time);
		void SetWeatherIntensity(float w);
		void SetRainIntensity(float r);

		void SetSkyMode(SkyMode mode);
		void SetCubemap(Ref<CubemapTexture> cubemap);

		void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPos);
		void RenderOrthographic(const glm::vec3& cameraPos);

	private:
		void GenerateQuadMesh();
		void GenerateCubeMesh();

		Ref<Shader> m_SkyShader;
		Ref<Shader> m_CubemapShader;
		uint32_t m_QuadVAO = 0;
		uint32_t m_QuadVBO = 0;
		uint32_t m_QuadIBO = 0;
		uint32_t m_CubeVAO = 0;
		uint32_t m_CubeVBO = 0;

		float m_TimeOfDay = 0.4f;
		float m_WeatherIntensity = 0.0f;
		float m_RainIntensity = 0.0f;

		SkyMode m_SkyMode = SkyMode::Procedural;
		Ref<CubemapTexture> m_Cubemap;
	};
}
