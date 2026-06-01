#pragma once
#include "Renderer.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Camera.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include "ParticleSystem2D.h"

namespace Quentlam
{
	class PerspectiveCamera;

	class QUENTLAM_API Renderer2D : Renderer
	{

	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(OrthographicCamera& camera);
		static void BeginScene(const PerspectiveCamera& camera);
		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Flush();


		//primitive (����)
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);

		static void DrawSprite(const glm::mat4& transform, const struct SpriteTransformComponent& src, int entityID = -1);
		static void DrawSprite(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);

		static void DrawTriangle(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawTriangle(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);



		//Rotation is in radians (��������ת������)
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const float rotation, const glm::vec4& color, int entityID = -1);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const float rotation, const glm::vec4& color, int entityID = -1);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float rotation, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float rotation, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float rotation, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float rotation, const float tilingFactor = 1.0f, const glm::vec4& tinColor = glm::vec4(1.0f), int entityID = -1);




		//Statistic
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; };
			uint32_t GetTotalIndexCount() { return QuadCount * 3; };
		};


		static void ResetStats();
		static Statistics GetStatistics();

		static void DrawParticle(const glm::mat4& transform, const struct Particle2D& particle, const Ref<Texture2D>& texture, int entityID = -1);
		static Ref<Texture2D> GetWhiteTexture();

	private:
		static void FlushAndReset();

	};

}


