#pragma once

#include "Renderer.h"
#include "Camera.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Model.h"
#include "Texture.h"

namespace Quentlam
{
	class QUENTLAM_API Renderer3D : Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(OrthographicCamera& camera);
		static void BeginScene(const PerspectiveCamera& camera);
		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Flush();

		// Basic 3D primitives
		static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
		static void DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID = -1, float ambient = 0.3f, float diffuse = 0.8f, float specular = 0.5f, float shininess = 32.0f);
		static void DrawCube(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1, float ambient = 0.3f, float diffuse = 0.8f, float specular = 0.5f, float shininess = 32.0f);

		static void DrawCapsule(const glm::mat4& transform, const glm::vec4& color, int entityID = -1, float ambient = 0.3f, float diffuse = 0.8f, float specular = 0.5f, float shininess = 32.0f);

		static void DrawModel(const glm::mat4& transform, const Model& model, const glm::vec4& color = glm::vec4(1.0f), int entityID = -1);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;

			uint32_t GetTotalVertexCount() { return CubeCount * 24; } // 24 vertices per cube for proper normals
			uint32_t GetTotalIndexCount() { return CubeCount * 36; } // 36 indices per cube
		};

		static void ResetStats();
		static Statistics GetStatistics();

		static void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);

		// Gizmos (editor overlay)
		static void DrawCameraFrustum(const glm::vec3& position, const glm::vec3& rotation, float fov, float aspectRatio, float nearClip, float farClip, const glm::vec4& color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

	private:
		static void FlushAndReset();
	};
}
