#include "qlpch.h"
#include "CubemapTexture.h"
#include "Quentlam/Renderer/Renderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "stb_image.h"
#include <vector>

namespace Quentlam
{

	Ref<CubemapTexture> CubemapTexture::Create(const std::string& right, const std::string& left,
		const std::string& top, const std::string& bottom,
		const std::string& front, const std::string& back)
	{
		Ref<CubemapTexture> cubemap = CreateRef<CubemapTexture>();
		glGenTextures(1, &cubemap->m_RendererID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->m_RendererID);

		std::vector<std::string> faces = { right, left, top, bottom, front, back };
		stbi_set_flip_vertically_on_load(0);

		for (uint32_t i = 0; i < 6; ++i)
		{
			int w, h, ch;
			stbi_uc* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
			if (data)
			{
				GLenum format = ch == 4 ? GL_RGBA : GL_RGB;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, w, h, 0, format, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			}
			else
			{
				QL_CORE_ERROR("CubemapTexture: Failed to load face: {0}", faces[i]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		return cubemap;
	}

	Ref<CubemapTexture> CubemapTexture::CreateHDRI(const std::string& filepath)
	{
		QL_CORE_WARN("CubemapTexture: HDRI loading not yet implemented, using procedural skybox");
		return CreateProceduralSkybox();
	}

	Ref<CubemapTexture> CubemapTexture::CreateProceduralSkybox()
	{
		Ref<CubemapTexture> cubemap = CreateRef<CubemapTexture>();
		glGenTextures(1, &cubemap->m_RendererID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->m_RendererID);

		// Generate placeholder colors for each face
		glm::vec3 faceColors[6] = {
			glm::vec3(0.2f, 0.3f, 0.8f),  // +X (right) - blue
			glm::vec3(0.8f, 0.2f, 0.2f),  // -X (left) - red
			glm::vec3(0.9f, 0.9f, 0.95f), // +Y (top) - white
			glm::vec3(0.3f, 0.5f, 0.2f),  // -Y (bottom) - green
			glm::vec3(0.9f, 0.8f, 0.2f),  // +Z (front) - yellow
			glm::vec3(0.7f, 0.2f, 0.8f),  // -Z (back) - purple
		};

		for (uint32_t i = 0; i < 6; ++i)
		{
			std::vector<stbi_uc> data(256 * 256 * 3);
			for (uint32_t j = 0; j < 256 * 256; ++j)
			{
				data[j * 3 + 0] = static_cast<stbi_uc>(faceColors[i].r * 255);
				data[j * 3 + 1] = static_cast<stbi_uc>(faceColors[i].g * 255);
				data[j * 3 + 2] = static_cast<stbi_uc>(faceColors[i].b * 255);
			}
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		return cubemap;
	}
}
