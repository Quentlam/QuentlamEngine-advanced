#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Renderer/SubTexture2D.h"
#include <glm/glm.hpp>
#include <string>

namespace Quentlam
{
	class Material;
	enum class ESpriteFlip : uint8_t
	{
		None = 0,
		Horizontal = BIT(0),
		Vertical = BIT(1)
	};

	struct SpriteRendererComponent
	{
		Ref<Texture2D> Texture = nullptr;
		Ref<SubTexture2D> SubTexture = nullptr;

		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };

		glm::vec2 Size = { 1.0f, 1.0f };
		glm::vec2 Pivot = { 0.5f, 0.5f };

		float TilingFactor = 1.0f;
		float SortingOrder = 0.0f;

		bool FlipX = false;
		bool FlipY = false;

		ESpriteFlip FlipMode = ESpriteFlip::None;

		Ref<Material> Material = nullptr;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;

		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}

		SpriteRendererComponent(Ref<Texture2D> texture, const glm::vec4& color = {1,1,1,1})
			: Texture(texture), Color(color) {}

		SpriteRendererComponent(Ref<SubTexture2D> subTex, const glm::vec4& color = {1,1,1,1})
			: SubTexture(subTex), Color(color)
		{
			if (subTex)
				Texture = subTex->GetTexture();
		}

		bool HasTexture() const { return Texture != nullptr; }
		bool HasSubTexture() const { return SubTexture != nullptr; }
	};

	inline ESpriteFlip operator|(ESpriteFlip a, ESpriteFlip b)
	{
		return static_cast<ESpriteFlip>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	inline bool HasFlip(ESpriteFlip a, ESpriteFlip b)
	{
		return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
	}
}
