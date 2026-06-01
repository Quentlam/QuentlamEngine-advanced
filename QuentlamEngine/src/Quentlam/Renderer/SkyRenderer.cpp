#include "qlpch.h"
#include "SkyRenderer.h"
#include "CubemapTexture.h"
#include "Quentlam/Renderer/Renderer.h"
#include <glad/glad.h>
#include <cmath>

namespace Quentlam
{
	SkyRenderer::SkyRenderer() = default;

	void SkyRenderer::Init()
	{
		GenerateQuadMesh();
		GenerateCubeMesh();

		m_SkyShader = Shader::Create("SkyQuad", R"(
			#version 330 core
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec2 a_UV;
			out vec2 v_UV;
			void main()
			{
				v_UV = a_UV;
				gl_Position = vec4(a_Position.xy, 0.9999, 1.0);
			}
		)", R"(
			#version 330 core
			precision highp float;
			in vec2 v_UV;
			out vec4 fragColor;
			uniform mat4 u_InverseViewProjection;
			uniform float u_TimeOfDay;
			uniform float u_WeatherIntensity;
			uniform float u_RainIntensity;
			const float PI = 3.14159265359;

			float Hash31(vec3 p)
			{
				p = fract(p * vec3(443.8975, 397.2973, 491.1871));
				p += dot(p.zxy, p.yxz + 19.19);
				return fract(p.x * p.y * p.z);
			}

			vec3 GetSunDir()
			{
				float angle = u_TimeOfDay * 2.0 * PI - PI * 0.5;
				return normalize(vec3(cos(angle), sin(angle) * 0.7 + 0.3, 0.2));
			}

			vec3 GetSkyColor(vec3 dir)
			{
				dir = normalize(dir);
				float sunH = GetSunDir().y;
				float day = clamp(sunH, 0.0, 1.0);
				float night = clamp(1.0 - sunH * 3.0, 0.0, 1.0);

				vec3 zenithDay = mix(vec3(0.18, 0.35, 0.70), vec3(0.52, 0.74, 0.95), day);
				vec3 horizonDay = mix(vec3(0.05, 0.05, 0.14), vec3(0.78, 0.88, 1.00), day);
				vec3 nightZenith = vec3(0.005, 0.005, 0.02);
				vec3 nightHorizon = vec3(0.01, 0.01, 0.04);

				vec3 sky = mix(horizonDay, zenithDay, pow(max(dir.y, 0.0), 0.45));
				sky = mix(sky, mix(nightHorizon, nightZenith, pow(max(dir.y, 0.0), 0.5)), night);

				float h = max(dir.y, 0.0);
				sky = mix(sky, mix(vec3(0.9,0.55,0.2), vec3(1.0,0.85,0.6), day), night * 0.6 * pow(1.0 - h, 3.0));
				return sky;
			}

			float GetStars(vec3 dir)
			{
				dir = normalize(dir);
				float night = clamp(1.0 - GetSunDir().y * 4.0, 0.0, 1.0);
				if (night < 0.05 || dir.y < 0.02) return 0.0;
				vec3 sp = floor(dir * 150.0);
				float s = Hash31(sp);
				s = pow(max(s - 0.94, 0.0) * 16.67, 2.0);
				return s * night * (0.6 + 0.4 * sin(u_TimeOfDay * 50.0 + s * 20.0));
			}

			float GetClouds(vec3 dir)
			{
				dir = normalize(dir);
				if (dir.y < 0.01) return 0.0;
				vec3 cp = floor(dir * 6.0 + vec3(u_TimeOfDay * 2.0, 0.0, u_TimeOfDay));
				float c = Hash31(cp + 100.0);
				c = smoothstep(0.45, 0.65, c);
				return clamp(c * clamp(1.0 - u_WeatherIntensity * 0.8, 0.0, 1.0), 0.0, 1.0);
			}

			void main()
			{
				vec4 ndc = vec4(v_UV * 2.0 - 1.0, 1.0, 1.0);
				vec4 worldPos = u_InverseViewProjection * ndc;
				vec3 dir = normalize(worldPos.xyz / worldPos.w);

				vec3 sky = GetSkyColor(dir);

				float cloudCover = GetClouds(dir);
				vec3 cloudCol = mix(vec3(0.92, 0.93, 0.97), vec3(0.45, 0.45, 0.5), u_WeatherIntensity);
				sky = mix(sky, cloudCol, cloudCover * 0.55);

				float stars = GetStars(dir);
				sky += vec3(stars);

				vec3 sunDir = GetSunDir();
				float sunDot = dot(dir, sunDir);
				float day = clamp(sunDir.y, 0.0, 1.0);
				vec3 sunCol = mix(vec3(1.0, 0.45, 0.1), vec3(1.0, 0.98, 0.92), day);

				sky += sunCol * pow(max(sunDot, 0.0), 256.0) * 3.0 * day;
				sky += sunCol * pow(max(sunDot, 0.0), 16.0) * 0.3 * day;
				sky += vec3(1.0, 1.0, 0.95) * pow(max(sunDot, 0.0), 4096.0) * 8.0 * day;

				float haze = exp(-dir.y * 8.0) * 0.15 * day;
				sky = mix(sky, vec3(0.85, 0.88, 0.95), haze);

				if (u_WeatherIntensity > 0.0)
				{
					vec3 fogCol = mix(vec3(0.7, 0.75, 0.85), vec3(0.4, 0.42, 0.5), u_WeatherIntensity);
					sky = mix(sky, fogCol, u_WeatherIntensity * 0.3);
				}

				fragColor = vec4(sky, 1.0);
			}
		)");

		// Cubemap skybox shader — samples a cube texture and applies simple atmospheric tint
		m_CubemapShader = Shader::Create("SkyCubemap", R"(
			#version 330 core
			layout(location = 0) in vec3 a_Position;
			out vec3 v_TexCoord;
			uniform mat4 u_ViewProjection;
			void main()
			{
				v_TexCoord = a_Position;
				vec4 pos = u_ViewProjection * vec4(a_Position, 1.0);
				gl_Position = pos.xyww; // Force depth to 1.0 (far plane)
			}
		)", R"(
			#version 330 core
			precision highp float;
			in vec3 v_TexCoord;
			out vec4 fragColor;
			uniform samplerCube u_Cubemap;
			uniform float u_TimeOfDay;
			uniform float u_WeatherIntensity;

			const float PI = 3.14159265359;
			vec3 GetSunDir() {
				float angle = u_TimeOfDay * 2.0 * PI - PI * 0.5;
				return normalize(vec3(cos(angle), sin(angle) * 0.7 + 0.3, 0.2));
			}

			void main()
			{
				vec3 dir = normalize(v_TexCoord);
				vec3 sky = texture(u_Cubemap, dir).rgb;

				// Atmospheric tint based on sun direction
				float day = clamp(GetSunDir().y, 0.0, 1.0);
				vec3 dayTint = mix(vec3(0.9, 0.85, 0.8), vec3(1.0, 1.0, 1.0), day);
				vec3 nightTint = mix(vec3(0.3, 0.3, 0.5), vec3(0.6, 0.6, 0.8), day);
				sky *= mix(nightTint, dayTint, day);

				// Weather fog
				if (u_WeatherIntensity > 0.0) {
					vec3 fogCol = mix(vec3(0.7, 0.75, 0.85), vec3(0.4, 0.42, 0.5), u_WeatherIntensity);
					sky = mix(sky, fogCol, u_WeatherIntensity * 0.4);
				}

				fragColor = vec4(sky, 1.0);
			}
		)");

		if (!m_SkyShader)
		{
			QL_CORE_ERROR("SkyRenderer: SkyShader failed to create! Check OpenGL logs for shader compilation errors.");
		}
		if (!m_CubemapShader)
		{
			QL_CORE_ERROR("SkyRenderer: CubemapShader failed to create! Check OpenGL logs for shader compilation errors.");
		}
	}

	void SkyRenderer::SetSkyMode(SkyMode mode)
	{
		m_SkyMode = mode;
	}

	void SkyRenderer::SetCubemap(Ref<CubemapTexture> cubemap)
	{
		m_Cubemap = cubemap;
		m_SkyMode = SkyMode::Cubemap;
	}

	void SkyRenderer::SetTimeOfDay(float time)
	{
		m_TimeOfDay = std::clamp(time, 0.0f, 1.0f);
	}

	void SkyRenderer::SetWeatherIntensity(float w)
	{
		m_WeatherIntensity = std::clamp(w, 0.0f, 1.0f);
	}

	void SkyRenderer::SetRainIntensity(float r)
	{
		m_RainIntensity = std::clamp(r, 0.0f, 1.0f);
	}

	void SkyRenderer::GenerateQuadMesh()
	{
		float quad[] = {
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
		};
		uint32_t idx[] = { 0, 1, 2, 2, 3, 0 };
		glGenVertexArrays(1, &m_QuadVAO);
		glGenBuffers(1, &m_QuadVBO);
		glGenBuffers(1, &m_QuadIBO);
		glBindVertexArray(m_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glBindVertexArray(0);
	}

	void SkyRenderer::GenerateCubeMesh()
	{
		float cube[] = {
			// Back face
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			// Front face
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,
			// Left face
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			// Right face
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			// Bottom face
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			// Top face
			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,
		};

		glGenVertexArrays(1, &m_CubeVAO);
		glGenBuffers(1, &m_CubeVBO);
		glBindVertexArray(m_CubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindVertexArray(0);
	}

	void SkyRenderer::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPos)
	{
		if (m_SkyMode == SkyMode::Cubemap && m_Cubemap && m_CubemapShader)
		{
			// ---- Cubemap skybox rendering ----
			glDepthMask(GL_FALSE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);

			glm::mat4 rotOnlyView = glm::mat4(glm::mat3(viewMatrix));
			glm::mat4 viewProj = projectionMatrix * rotOnlyView;

			m_CubemapShader->Bind();
			m_CubemapShader->SetMat4("u_ViewProjection", viewProj);
			m_CubemapShader->SetFloat("u_TimeOfDay", m_TimeOfDay);
			m_CubemapShader->SetFloat("u_WeatherIntensity", m_WeatherIntensity);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_Cubemap->GetRendererID());
			m_CubemapShader->SetInt("u_Cubemap", 0);

			glBindVertexArray(m_CubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);

			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
		}
		else
		{
			// ---- Procedural skybox rendering ----
			if (!m_SkyShader)
			{
				QL_CORE_ERROR("SkyRenderer: m_SkyShader is null!");
				return;
			}
			if (m_QuadVAO == 0)
			{
				QL_CORE_ERROR("SkyRenderer: QuadVAO is 0!");
				return;
			}

			glDepthMask(GL_FALSE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);

			glm::mat4 viewProj = projectionMatrix * viewMatrix;
			glm::mat4 inverseVP = glm::inverse(viewProj);

			m_SkyShader->Bind();
			m_SkyShader->SetMat4("u_InverseViewProjection", inverseVP);
			m_SkyShader->SetFloat("u_TimeOfDay", m_TimeOfDay);
			m_SkyShader->SetFloat("u_WeatherIntensity", m_WeatherIntensity);
			m_SkyShader->SetFloat("u_RainIntensity", m_RainIntensity);

			glBindVertexArray(m_QuadVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
		}
	}

	void SkyRenderer::RenderOrthographic(const glm::vec3& cameraPos)
	{
		Render(glm::mat4(1.0f), glm::mat4(1.0f), cameraPos);
	}
}
