#pragma once
#include <glm/glm.hpp>

namespace Quentlam
{

struct RectTransformComponent
{
	glm::vec2 AnchorMin = { 0.5f, 0.5f };
	glm::vec2 AnchorMax = { 0.5f, 0.5f };
	glm::vec2 Pivot = { 0.5f, 0.5f };
	glm::vec2 AnchoredPosition = { 0.0f, 0.0f };
	glm::vec2 SizeDelta = { 100.0f, 50.0f };
	glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec2 Scale = { 1.0f, 1.0f };

	glm::vec2 GetWorldSize() const
	{
		return SizeDelta;
	}

	glm::vec3 GetWorldPosition() const
	{
		return { AnchoredPosition.x, AnchoredPosition.y, 0.0f };
	}
};

}
