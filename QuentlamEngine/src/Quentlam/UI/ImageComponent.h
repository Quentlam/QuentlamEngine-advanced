#pragma once
#include <glm/glm.hpp>
#include <string>

namespace Quentlam
{

enum class EImageType
{
	Simple,
	Filled,
	Sliced,
	Tiled
};

struct ImageComponent
{
	std::string SpritePath;
	glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	EImageType ImageType = EImageType::Simple;
	float FillAmount = 1.0f;
	float FillCenter = 1.0f;
	bool RaycastTarget = true;
	bool PreserveAspect = false;
};

}
