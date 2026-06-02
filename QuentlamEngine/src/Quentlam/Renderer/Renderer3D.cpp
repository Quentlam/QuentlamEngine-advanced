#include "qlpch.h"
#include "Renderer3D.h"

#include "VertexArray.h"
#include "Shader.h"
#include "RenderCommand.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Quentlam
{
	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		int EntityID;
		
		float AmbientStrength;
		float DiffuseStrength;
		float SpecularStrength;
		float Shininess;
	};

	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;
		static const uint32_t MaxVertices = MaxCubes * 24;
		static const uint32_t MaxIndices = MaxCubes * 36;
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> CubeVertexArray;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Shader> ModelShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 CubeVertexPositions[24];
		glm::vec3 CubeVertexNormals[24];
		glm::vec2 CubeTexCoords[24];

		Renderer3D::Statistics Stats;

		glm::vec3 LightDirection = { -0.5f, -1.0f, -0.3f };
		glm::vec3 LightColor = { 1.0f, 1.0f, 1.0f };
		float LightIntensity = 1.0f;
	};

	static Renderer3DData s_Data3D;

	void Renderer3D::Init()
	{
		QL_PROFILE_FUNCTION();

		s_Data3D.CubeVertexArray = VertexArray::Create();

		// For a cube, we need 24 vertices to have correct texture coordinates per face
		const uint32_t maxVertices = s_Data3D.MaxCubes * 24;
		s_Data3D.CubeVertexBuffer = VertexBuffer::Create(maxVertices * sizeof(CubeVertex));
		s_Data3D.CubeVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID" },
			{ ShaderDataType::Float,  "a_AmbientStrength" },
			{ ShaderDataType::Float,  "a_DiffuseStrength" },
			{ ShaderDataType::Float,  "a_SpecularStrength" },
			{ ShaderDataType::Float,  "a_Shininess" }
		});
		s_Data3D.CubeVertexArray->AddVertexBuffer(s_Data3D.CubeVertexBuffer);

		s_Data3D.CubeVertexBufferBase = new CubeVertex[maxVertices];

		uint32_t* cubeIndices = new uint32_t[s_Data3D.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data3D.MaxIndices; i += 36)
		{
			for (int face = 0; face < 6; face++)
			{
				cubeIndices[i + face * 6 + 0] = offset + 0;
				cubeIndices[i + face * 6 + 1] = offset + 1;
				cubeIndices[i + face * 6 + 2] = offset + 2;

				cubeIndices[i + face * 6 + 3] = offset + 2;
				cubeIndices[i + face * 6 + 4] = offset + 3;
				cubeIndices[i + face * 6 + 5] = offset + 0;

				offset += 4;
			}
		}

		Ref<IndexBuffer> cubeIB = IndexBuffer::Create(cubeIndices, s_Data3D.MaxIndices);
		s_Data3D.CubeVertexArray->SetIndexBuffer(cubeIB);
		delete[] cubeIndices;

		s_Data3D.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data3D.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data3D.MaxTextureSlots];
		for (int i = 0; i < s_Data3D.MaxTextureSlots; i++)
			samplers[i] = i;

		// Reusing the 2D texture shader for now, ideally 3D needs its own shader with lighting
		s_Data3D.TextureShader = Shader::Create("assets/shaders/CubeShader.glsl");
		s_Data3D.TextureShader->Bind();
		s_Data3D.TextureShader->SetIntArray("u_Textures", samplers, s_Data3D.MaxTextureSlots);

		s_Data3D.ModelShader = Shader::Create("assets/shaders/ModelShader.glsl");

		s_Data3D.TextureSlots[0] = s_Data3D.WhiteTexture;

		// Define the 24 vertices for a unit cube centered at origin
		// Front
		s_Data3D.CubeVertexPositions[0]  = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[1]  = {  0.5f, -0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[2]  = {  0.5f,  0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[3]  = { -0.5f,  0.5f,  0.5f, 1.0f };
		// Back
		s_Data3D.CubeVertexPositions[4]  = {  0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[5]  = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[6]  = { -0.5f,  0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[7]  = {  0.5f,  0.5f, -0.5f, 1.0f };
		// Left
		s_Data3D.CubeVertexPositions[8]  = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[9]  = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[10] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[11] = { -0.5f,  0.5f, -0.5f, 1.0f };
		// Right
		s_Data3D.CubeVertexPositions[12] = {  0.5f, -0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[13] = {  0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[14] = {  0.5f,  0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[15] = {  0.5f,  0.5f,  0.5f, 1.0f };
		// Top
		s_Data3D.CubeVertexPositions[16] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[17] = {  0.5f,  0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[18] = {  0.5f,  0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[19] = { -0.5f,  0.5f, -0.5f, 1.0f };
		// Bottom
		s_Data3D.CubeVertexPositions[20] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[21] = {  0.5f, -0.5f, -0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[22] = {  0.5f, -0.5f,  0.5f, 1.0f };
		s_Data3D.CubeVertexPositions[23] = { -0.5f, -0.5f,  0.5f, 1.0f };

		// Normals
		for (int i = 0; i < 4; i++) s_Data3D.CubeVertexNormals[i] = { 0.0f, 0.0f, 1.0f }; // Front
		for (int i = 4; i < 8; i++) s_Data3D.CubeVertexNormals[i] = { 0.0f, 0.0f, -1.0f }; // Back
		for (int i = 8; i < 12; i++) s_Data3D.CubeVertexNormals[i] = { -1.0f, 0.0f, 0.0f }; // Left
		for (int i = 12; i < 16; i++) s_Data3D.CubeVertexNormals[i] = { 1.0f, 0.0f, 0.0f }; // Right
		for (int i = 16; i < 20; i++) s_Data3D.CubeVertexNormals[i] = { 0.0f, 1.0f, 0.0f }; // Top
		for (int i = 20; i < 24; i++) s_Data3D.CubeVertexNormals[i] = { 0.0f, -1.0f, 0.0f }; // Bottom

		for (int i = 0; i < 6; i++)
		{
			s_Data3D.CubeTexCoords[i * 4 + 0] = { 0.0f, 0.0f };
			s_Data3D.CubeTexCoords[i * 4 + 1] = { 1.0f, 0.0f };
			s_Data3D.CubeTexCoords[i * 4 + 2] = { 1.0f, 1.0f };
			s_Data3D.CubeTexCoords[i * 4 + 3] = { 0.0f, 1.0f };
		}
	}

	void Renderer3D::Shutdown()
	{
		QL_PROFILE_FUNCTION();
		delete[] s_Data3D.CubeVertexBufferBase;
	}

	void Renderer3D::BeginScene(OrthographicCamera& camera)
	{
		QL_PROFILE_FUNCTION();
		s_Data3D.TextureShader->Bind();
		s_Data3D.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
		s_Data3D.TextureShader->SetFloat3("u_LightDirection", s_Data3D.LightDirection);
		s_Data3D.TextureShader->SetFloat3("u_LightColor", s_Data3D.LightColor);
		s_Data3D.TextureShader->SetFloat("u_LightIntensity", s_Data3D.LightIntensity);
		s_Data3D.TextureShader->SetFloat3("u_ViewPos", camera.GetPosition());
		s_Data3D.ModelShader->Bind();
		s_Data3D.ModelShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
		s_Data3D.CubeIndexCount = 0;
		s_Data3D.CubeVertexBufferPtr = s_Data3D.CubeVertexBufferBase;
		s_Data3D.TextureSlotIndex = 1;
	}

	void Renderer3D::BeginScene(const PerspectiveCamera& camera)
	{
		QL_PROFILE_FUNCTION();
		s_Data3D.TextureShader->Bind();
		s_Data3D.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
		s_Data3D.TextureShader->SetFloat3("u_LightDirection", s_Data3D.LightDirection);
		s_Data3D.TextureShader->SetFloat3("u_LightColor", s_Data3D.LightColor);
		s_Data3D.TextureShader->SetFloat("u_LightIntensity", s_Data3D.LightIntensity);
		s_Data3D.TextureShader->SetFloat3("u_ViewPos", camera.GetPosition());
		s_Data3D.ModelShader->Bind();
		s_Data3D.ModelShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
		s_Data3D.CubeIndexCount = 0;
		s_Data3D.CubeVertexBufferPtr = s_Data3D.CubeVertexBufferBase;
		s_Data3D.TextureSlotIndex = 1;
	}

	void Renderer3D::DrawModel(const glm::mat4& transform, const Model& model, const glm::vec4& color, int entityID)
	{
		QL_PROFILE_FUNCTION();

		s_Data3D.ModelShader->Bind();
		s_Data3D.ModelShader->SetMat4("u_Transform", transform);
		s_Data3D.ModelShader->SetFloat4("u_Color", color);
		s_Data3D.ModelShader->SetInt("u_EntityID", entityID);

		model.Draw();
	}

	void Renderer3D::EndScene()
	{
		QL_PROFILE_FUNCTION();

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data3D.CubeVertexBufferPtr - (uint8_t*)s_Data3D.CubeVertexBufferBase);
		s_Data3D.CubeVertexBuffer->SetData(s_Data3D.CubeVertexBufferBase, dataSize);

		s_Data3D.TextureShader->Bind();
		s_Data3D.TextureShader->SetMat4("u_Transform", glm::mat4(1.0f));

		Flush();
	}

	void Renderer3D::Flush()
	{
		if (s_Data3D.CubeIndexCount == 0)
			return;

		s_Data3D.TextureShader->Bind();

		for (uint32_t i = 0; i < s_Data3D.TextureSlotIndex; i++)
			s_Data3D.TextureSlots[i]->Bind(i);

		RenderCommand::DrawIndexed(s_Data3D.CubeVertexArray, s_Data3D.CubeIndexCount);
		s_Data3D.Stats.DrawCalls++;
	}

	void Renderer3D::FlushAndReset()
	{
		EndScene();

		s_Data3D.CubeIndexCount = 0;
		s_Data3D.CubeVertexBufferPtr = s_Data3D.CubeVertexBufferBase;
		s_Data3D.TextureSlotIndex = 1;
	}

	void Renderer3D::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), size);
		DrawCube(transform, color, entityID);
	}

	void Renderer3D::DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID, float ambient, float diffuse, float specular, float shininess)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data3D.CubeIndexCount >= s_Data3D.MaxIndices)
			FlushAndReset();

		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;
		
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		for (uint32_t i = 0; i < 24; i++)
		{
			s_Data3D.CubeVertexBufferPtr->Position = transform * s_Data3D.CubeVertexPositions[i];
			s_Data3D.CubeVertexBufferPtr->Normal = normalMatrix * s_Data3D.CubeVertexNormals[i];
			s_Data3D.CubeVertexBufferPtr->Color = color;
			s_Data3D.CubeVertexBufferPtr->TexCoord = s_Data3D.CubeTexCoords[i];
			s_Data3D.CubeVertexBufferPtr->TexIndex = textureIndex;
			s_Data3D.CubeVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data3D.CubeVertexBufferPtr->EntityID = entityID;
			s_Data3D.CubeVertexBufferPtr->AmbientStrength = ambient;
			s_Data3D.CubeVertexBufferPtr->DiffuseStrength = diffuse;
			s_Data3D.CubeVertexBufferPtr->SpecularStrength = specular;
			s_Data3D.CubeVertexBufferPtr->Shininess = shininess;
			s_Data3D.CubeVertexBufferPtr++;
		}

		s_Data3D.CubeIndexCount += 36;
		s_Data3D.Stats.CubeCount++;
	}

	void Renderer3D::DrawCube(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor, const glm::vec4& tintColor, int entityID, float ambient, float diffuse, float specular, float shininess)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data3D.CubeIndexCount >= s_Data3D.MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data3D.TextureSlotIndex; i++)
		{
			if (*s_Data3D.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data3D.TextureSlotIndex >= s_Data3D.MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data3D.TextureSlotIndex;
			s_Data3D.TextureSlots[s_Data3D.TextureSlotIndex] = texture;
			s_Data3D.TextureSlotIndex++;
		}
		
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		for (uint32_t i = 0; i < 24; i++)
		{
			s_Data3D.CubeVertexBufferPtr->Position = transform * s_Data3D.CubeVertexPositions[i];
			s_Data3D.CubeVertexBufferPtr->Normal = normalMatrix * s_Data3D.CubeVertexNormals[i];
			s_Data3D.CubeVertexBufferPtr->Color = tintColor;
			s_Data3D.CubeVertexBufferPtr->TexCoord = s_Data3D.CubeTexCoords[i];
			s_Data3D.CubeVertexBufferPtr->TexIndex = textureIndex;
			s_Data3D.CubeVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data3D.CubeVertexBufferPtr->EntityID = entityID;
			s_Data3D.CubeVertexBufferPtr->AmbientStrength = ambient;
			s_Data3D.CubeVertexBufferPtr->DiffuseStrength = diffuse;
			s_Data3D.CubeVertexBufferPtr->SpecularStrength = specular;
			s_Data3D.CubeVertexBufferPtr->Shininess = shininess;
			s_Data3D.CubeVertexBufferPtr++;
		}

		s_Data3D.CubeIndexCount += 36;
		s_Data3D.Stats.CubeCount++;
	}

	void Renderer3D::DrawCapsule(const glm::mat4& transform, const glm::vec4& color, int entityID, float ambient, float diffuse, float specular, float shininess)
	{
		QL_PROFILE_FUNCTION();

		// Smooth Capsule Implementation using Quads (compatible with existing Index Buffer)
		// A capsule consists of a cylinder body and two hemispherical caps.
		
		uint32_t segments = 16;
		uint32_t rings = 8;
		float radius = 0.5f;
		float halfHeight = 0.5f; // Total height will be 1.0 (0.5 cylinder + 0.5 + 0.5 caps? No, let's make it total height 2.0 like the cubes)
		// User's previous pillars were scale(0.5, 2.0, 0.5). 
		// A unit capsule in most engines is radius 0.5, total height 2.0 (cylinder height 1.0 + two 0.5 radius caps).
		float cylinderHeight = 1.0f;
		
		auto pushQuad = [&](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
						   const glm::vec3& n0, const glm::vec3& n1, const glm::vec3& n2, const glm::vec3& n3)
		{
			if (s_Data3D.CubeIndexCount + 6 >= s_Data3D.MaxIndices)
				FlushAndReset();

			glm::vec3 pos[4] = { p0, p1, p2, p3 };
			glm::vec3 norm[4] = { n0, n1, n2, n3 };
			glm::vec2 tex[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

			for (int i = 0; i < 4; i++)
			{
				s_Data3D.CubeVertexBufferPtr->Position = transform * glm::vec4(pos[i], 1.0f);
				s_Data3D.CubeVertexBufferPtr->Normal = normalMatrix * norm[i];
				s_Data3D.CubeVertexBufferPtr->Color = color;
				s_Data3D.CubeVertexBufferPtr->TexCoord = tex[i];
				s_Data3D.CubeVertexBufferPtr->TexIndex = 0.0f;
				s_Data3D.CubeVertexBufferPtr->TilingFactor = 1.0f;
				s_Data3D.CubeVertexBufferPtr->EntityID = entityID;
				s_Data3D.CubeVertexBufferPtr->AmbientStrength = ambient;
				s_Data3D.CubeVertexBufferPtr->DiffuseStrength = diffuse;
				s_Data3D.CubeVertexBufferPtr->SpecularStrength = specular;
				s_Data3D.CubeVertexBufferPtr->Shininess = shininess;
				s_Data3D.CubeVertexBufferPtr++;
			}
			
			// We need to skip the other 20 vertices that the index buffer expects for a "Cube" slot
			// because our index buffer is hardcoded for 24-vertex chunks.
			// To reuse the index buffer (0,1,2, 2,3,0), we must submit 4 vertices and then "waste" 20.
			// Alternatively, we can just fill 6 quads (a cube) or update the index buffer logic.
			// Easiest hack: submit 6 quads where 5 are degenerate (zero area) or just fill the 24.
			for (int i = 0; i < 20; i++)
			{
				*s_Data3D.CubeVertexBufferPtr = *(s_Data3D.CubeVertexBufferPtr - 1);
				s_Data3D.CubeVertexBufferPtr->Position = s_Data3D.CubeVertexBufferPtr[-1].Position; // degenerate
				s_Data3D.CubeVertexBufferPtr++;
			}

			s_Data3D.CubeIndexCount += 36;
			s_Data3D.Stats.CubeCount++;
		};

		// 1. Cylinder Body
		for (uint32_t i = 0; i < segments; i++)
		{
			float a0 = (float)i / segments * 2.0f * 3.14159f;
			float a1 = (float)(i + 1) / segments * 2.0f * 3.14159f;

			glm::vec3 n0(glm::cos(a0), 0, glm::sin(a0));
			glm::vec3 n1(glm::cos(a1), 0, glm::sin(a1));

			glm::vec3 p0 = n0 * radius + glm::vec3(0, -cylinderHeight * 0.5f, 0);
			glm::vec3 p1 = n1 * radius + glm::vec3(0, -cylinderHeight * 0.5f, 0);
			glm::vec3 p2 = n1 * radius + glm::vec3(0,  cylinderHeight * 0.5f, 0);
			glm::vec3 p3 = n0 * radius + glm::vec3(0,  cylinderHeight * 0.5f, 0);

			pushQuad(p0, p1, p2, p3, n0, n1, n1, n0);
		}

		// 2. Top Hemisphere
		for (uint32_t r = 0; r < rings; r++)
		{
			float phi0 = (float)r / rings * 0.5f * 3.14159f;
			float phi1 = (float)(r + 1) / rings * 0.5f * 3.14159f;

			for (uint32_t i = 0; i < segments; i++)
			{
				float theta0 = (float)i / segments * 2.0f * 3.14159f;
				float theta1 = (float)(i + 1) / segments * 2.0f * 3.14159f;

				auto getSpherePos = [&](float phi, float theta) {
					return glm::vec3(
						glm::cos(phi) * glm::cos(theta),
						glm::sin(phi),
						glm::cos(phi) * glm::sin(theta)
					);
				};

				glm::vec3 n0 = getSpherePos(phi0, theta0);
				glm::vec3 n1 = getSpherePos(phi0, theta1);
				glm::vec3 n2 = getSpherePos(phi1, theta1);
				glm::vec3 n3 = getSpherePos(phi1, theta0);

				glm::vec3 offset(0, cylinderHeight * 0.5f, 0);
				pushQuad(n0 * radius + offset, n1 * radius + offset, n2 * radius + offset, n3 * radius + offset, n0, n1, n2, n3);
			}
		}

		// 3. Bottom Hemisphere
		for (uint32_t r = 0; r < rings; r++)
		{
			float phi0 = -(float)r / rings * 0.5f * 3.14159f;
			float phi1 = -(float)(r + 1) / rings * 0.5f * 3.14159f;

			for (uint32_t i = 0; i < segments; i++)
			{
				float theta0 = (float)i / segments * 2.0f * 3.14159f;
				float theta1 = (float)(i + 1) / segments * 2.0f * 3.14159f;

				auto getSpherePos = [&](float phi, float theta) {
					return glm::vec3(
						glm::cos(phi) * glm::cos(theta),
						glm::sin(phi),
						glm::cos(phi) * glm::sin(theta)
					);
				};

				glm::vec3 n0 = getSpherePos(phi0, theta0);
				glm::vec3 n1 = getSpherePos(phi0, theta1);
				glm::vec3 n2 = getSpherePos(phi1, theta1);
				glm::vec3 n3 = getSpherePos(phi1, theta0);

				glm::vec3 offset(0, -cylinderHeight * 0.5f, 0);
				pushQuad(n3 * radius + offset, n2 * radius + offset, n1 * radius + offset, n0 * radius + offset, n3, n2, n1, n0);
			}
		}
	}

	void Renderer3D::SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
	{
		s_Data3D.LightDirection = direction;
		s_Data3D.LightColor = color;
		s_Data3D.LightIntensity = intensity;
	}

	void Renderer3D::ResetStats()
	{
		memset(&s_Data3D.Stats, 0, sizeof(Statistics));
	}

	Renderer3D::Statistics Renderer3D::GetStatistics()
	{
		return s_Data3D.Stats;
	}

	void Renderer3D::DrawCameraFrustum(const glm::vec3& position, const glm::vec3& rotation, float fov, float aspectRatio, float nearClip, float farClip, const glm::vec4& color)
	{
		// Use a fixed visual depth for the gizmo so it's always a reasonable size in the scene,
		// independent of the camera's clip planes (which can be very close like 0.1).
		// visualDepth = 50 * nearClip gives a frustum ~5 units deep for nearClip=0.1.
		float visualDepth = nearClip * 50.0f;
		float halfNearH = nearClip * tanf(glm::radians(fov * 0.5f));
		float halfNearW = halfNearH * aspectRatio;
		float halfFarH = visualDepth * tanf(glm::radians(fov * 0.5f));
		float halfFarW = halfFarH * aspectRatio;

		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);

		auto transformVec3 = [&](const glm::vec3& v) {
			return glm::vec3(translation * rot * glm::vec4(v, 0.0f));
		};

		glm::vec3 ntl = transformVec3({ -halfNearW,  halfNearH, -nearClip });
		glm::vec3 ntr = transformVec3({  halfNearW,  halfNearH, -nearClip });
		glm::vec3 nbl = transformVec3({ -halfNearW, -halfNearH, -nearClip });
		glm::vec3 nbr = transformVec3({  halfNearW, -halfNearH, -nearClip });

		glm::vec3 ftl = transformVec3({ -halfFarW,  halfFarH, -visualDepth });
		glm::vec3 ftr = transformVec3({  halfFarW,  halfFarH, -visualDepth });
		glm::vec3 fbl = transformVec3({ -halfFarW, -halfFarH, -visualDepth });
		glm::vec3 fbr = transformVec3({  halfFarW, -halfFarH, -visualDepth });

		glColor4fv(&color.x);
		glBegin(GL_LINES);
		glVertex3fv(&ntl.x); glVertex3fv(&ntr.x);
		glVertex3fv(&ntr.x); glVertex3fv(&nbr.x);
		glVertex3fv(&nbr.x); glVertex3fv(&nbl.x);
		glVertex3fv(&nbl.x); glVertex3fv(&ntl.x);

		glVertex3fv(&ftl.x); glVertex3fv(&ftr.x);
		glVertex3fv(&ftr.x); glVertex3fv(&fbr.x);
		glVertex3fv(&fbr.x); glVertex3fv(&fbl.x);
		glVertex3fv(&fbl.x); glVertex3fv(&ftl.x);

		glVertex3fv(&ntl.x); glVertex3fv(&ftl.x);
		glVertex3fv(&ntr.x); glVertex3fv(&ftr.x);
		glVertex3fv(&nbl.x); glVertex3fv(&fbl.x);
		glVertex3fv(&nbr.x); glVertex3fv(&fbr.x);
		glEnd();
	}
}
