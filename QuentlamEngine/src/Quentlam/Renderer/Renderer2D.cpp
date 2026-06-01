#include "qlpch.h"
#include "Renderer2D.h"


#include "VertexArray.h"
#include "Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "RenderCommand.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Quentlam/Scene/Components.h"


namespace Quentlam
{

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		int EntityID;
	};


	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 10000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;// TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		
		Ref<VertexArray> TriangleVertexArray;
		Ref<VertexBuffer> TriangleVertexBuffer;

		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t TriangleIndexCount = 0;
		QuadVertex* TriangleVertexBufferBase = nullptr;
		QuadVertex* TriangleVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPosition[4];
		glm::vec4 TriangleVertexPosition[3];

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_Data;


	void Renderer2D::Init()
	{
		QL_PROFILE_FUNCTION();
		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ShaderDataType::Float3,"a_Position"},
			{ShaderDataType::Float4,"a_Color"},
			{ ShaderDataType::Float2,"a_TexCoord" },
			{ ShaderDataType::Float,"a_TexIndex" },
			{ ShaderDataType::Float,"a_TilingFactor" },
			{ ShaderDataType::Int,"a_EntityID" }
			});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i+= 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}


		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Triangles
		s_Data.TriangleVertexArray = VertexArray::Create();
		s_Data.TriangleVertexBuffer = VertexBuffer::Create(s_Data.MaxIndices * sizeof(QuadVertex));
		s_Data.TriangleVertexBuffer->SetLayout({
			{ShaderDataType::Float3,"a_Position"},
			{ShaderDataType::Float4,"a_Color"},
			{ ShaderDataType::Float2,"a_TexCoord" },
			{ ShaderDataType::Float,"a_TexIndex" },
			{ ShaderDataType::Float,"a_TilingFactor" },
			{ ShaderDataType::Int,"a_EntityID" }
			});
		s_Data.TriangleVertexArray->AddVertexBuffer(s_Data.TriangleVertexBuffer);

		s_Data.TriangleVertexBufferBase = new QuadVertex[s_Data.MaxIndices];
		uint32_t* triangleIndices = new uint32_t[s_Data.MaxIndices];
		offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 3)
		{
			triangleIndices[i + 0] = offset + 0;
			triangleIndices[i + 1] = offset + 1;
			triangleIndices[i + 2] = offset + 2;
			offset += 3;
		}

		Ref<IndexBuffer> triangleIB = IndexBuffer::Create(triangleIndices, s_Data.MaxIndices);
		s_Data.TriangleVertexArray->SetIndexBuffer(triangleIB);
		delete[] triangleIndices;

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData,sizeof(uint32_t));
		
		int32_t samplaers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
			samplaers[i] = i;

		s_Data.TextureShader = Shader::Create("assets/shaders/Texture2DShader.glsl");
		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetIntArray("u_Textures", samplaers, s_Data.MaxTextureSlots);
		s_Data.TextureShader->SetFloat("u_TilingFactor", 1.0f);

		
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPosition[0] = { -0.5f,-0.5f,0.0f,1.0f };
		s_Data.QuadVertexPosition[1] = { 0.5f, -0.5f,0.0f,1.0f };
		s_Data.QuadVertexPosition[2] = { 0.5f,  0.5f,0.0f,1.0f };
		s_Data.QuadVertexPosition[3] = { -0.5f, 0.5f,0.0f,1.0f };

		s_Data.TriangleVertexPosition[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.TriangleVertexPosition[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.TriangleVertexPosition[2] = { 0.0f, 0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::Shutdown()
	{
		QL_PROFILE_FUNCTION();
		delete[] s_Data.QuadVertexBufferBase;
		delete[] s_Data.TriangleVertexBufferBase;
	}

	void Renderer2D::BeginScene(OrthographicCamera& camera)
	{
		QL_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}
	void Renderer2D::BeginScene(const PerspectiveCamera& camera)
	{
		QL_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::BeginScene(const Camera& camera)
	{
		QL_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::EndScene()
	{
		QL_PROFILE_FUNCTION();

		uint32_t quadDataSize = static_cast<uint32_t>((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, quadDataSize);

		uint32_t triangleDataSize = static_cast<uint32_t>((uint8_t*)s_Data.TriangleVertexBufferPtr - (uint8_t*)s_Data.TriangleVertexBufferBase);
		s_Data.TriangleVertexBuffer->SetData(s_Data.TriangleVertexBufferBase, triangleDataSize);

		Flush();
	}
	void Renderer2D::Flush()
	{
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		if (s_Data.TriangleIndexCount)
		{
			RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
			s_Data.Stats.DrawCalls++;
		}
		if (s_Data.QuadIndexCount)
		{
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::FlushAndReset()
	{
		EndScene();

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}


	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		DrawQuad({ position.x,position.y,0.0f }, size, color, entityID);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size.x,size.y, 1.0f));

		DrawQuad(transform, color, entityID);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		DrawQuad({ position.x,position.y,0.0f }, size, texture, tilingFactor, tinColor, entityID);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		QL_PROFILE_FUNCTION();
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f,0.0f }, { 1.0f, 0.0f },{ 1.0f,1.0f },{ 0.0f, 1.0f} };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

		DrawQuad(transform, texture, tilingFactor, tinColor, entityID);

	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		DrawQuad({ position.x,position.y,0.0f }, size, subTexture, tilingFactor, tinColor, entityID);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		QL_PROFILE_FUNCTION();
		constexpr size_t quadVertexCount = 4;

		const glm::vec2* textureCoords = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();


		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}


		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = tinColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;

	}

	void Renderer2D::DrawTriangle(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		QL_PROFILE_FUNCTION();
		constexpr size_t triangleVertexCount = 3;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },{ 0.5f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.TriangleIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = transform * s_Data.TriangleVertexPosition[i];
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TriangleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += 3;
		s_Data.Stats.QuadCount++; // We'll count triangles in quadcount for simplicity
	}

	void Renderer2D::DrawTriangle(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		QL_PROFILE_FUNCTION();
		constexpr size_t triangleVertexCount = 3;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },{ 0.5f, 1.0f } };

		if (s_Data.TriangleIndexCount >= Renderer2DData::MaxIndices || s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			FlushAndReset();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = transform * s_Data.TriangleVertexPosition[i];
			s_Data.TriangleVertexBufferPtr->Color = tintColor;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TriangleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += 3;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		QL_PROFILE_FUNCTION();
		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;
		constexpr glm::vec2 textureCoords[] = { { 0.0f,0.0f }, { 1.0f, 0.0f },{ 1.0f,1.0f },{ 0.0f, 1.0f} };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		constexpr size_t quadVertexCount = 4;

		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;

	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices || s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			FlushAndReset();

		constexpr size_t quadVertexCount = 4;

		float textureIndex = 0.0f;

		constexpr glm::vec2 textureCoords[] = { { 0.0f,0.0f }, { 1.0f, 0.0f },{ 1.0f,1.0f },{ 0.0f, 1.0f} };

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}


		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}


		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = tinColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;

	}



	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const float rotation, const glm::vec4& color, int entityID)
	{
		DrawRotatedQuad({ position.x, position.y , 0.0f}, size, rotation, color, entityID);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const float rotation, const glm::vec4& color, int entityID)
	{

		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();


		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f,0.0f }, { 1.0f, 0.0f },{ 1.0f,1.0f },{ 0.0f, 1.0f} };

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;


#if OLD_PATH
		QL_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
		s_Data.TextureShader->SetMat4("u_Transform", transform);
		s_Data.TextureShader->SetFloat4("u_Color", color);


		s_Data.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadVertexArray);

#endif
	}


	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float rotation, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		DrawRotatedQuad({ position.x, position.y , 0.0f }, size, texture, rotation, tilingFactor, tinColor, entityID);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const float rotation, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices || s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			FlushAndReset();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f,0.0f }, { 1.0f, 0.0f },{ 1.0f,1.0f },{ 0.0f, 1.0f} };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));



		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = tinColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
#if OLD_PATH
		QL_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
		s_Data.TextureShader->SetMat4("u_Transform", transform);
		s_Data.TextureShader->SetFloat4("u_Color", tinColor);
		s_Data.TextureShader->SetFloat("u_TilingFactor", tilingFactor);


		texture->Bind();
		s_Data.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadVertexArray);
#endif

	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float rotation, const float tilingFactor, const glm::vec4& tinColor, int entityID)
	{
		DrawRotatedQuad({ position.x, position.y , 0.0f }, size, subTexture, rotation, tilingFactor, tinColor, entityID);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const float rotation, const float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices || s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			FlushAndReset();

		constexpr size_t quadVertexCount = 4;

		const Ref<Texture2D> texture = subTexture->GetTexture();
		const glm::vec2* textureCoords = subTexture->GetTexCoords();


		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));



		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;

	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, const SpriteTransformComponent& src, int entityID)
	{
		DrawQuad(transform, src.Color, entityID);
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		DrawQuad(transform, texture, tilingFactor, tintColor, entityID);
	}

	void Renderer2D::DrawParticle(const glm::mat4& transform, const Particle2D& particle, const Ref<Texture2D>& texture, int entityID)
	{
		QL_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices || s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			FlushAndReset();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f } };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		float lifeRatio = particle.MaxLife > 0.0f ? particle.Life / particle.MaxLife : 1.0f;
		glm::vec4 color = glm::mix(particle.Color, particle.Color, lifeRatio);

		for (uint32_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPosition[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_Data.Stats;
	}

	Ref<Texture2D> Renderer2D::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

}

