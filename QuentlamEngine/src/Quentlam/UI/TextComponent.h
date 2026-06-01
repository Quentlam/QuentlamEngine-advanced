#pragma once
#include <glm/glm.hpp>
#include <string>

namespace Quentlam
{

enum class ETextAnchor
{
	UpperLeft, UpperCenter, UpperRight,
	MiddleLeft, MiddleCenter, MiddleRight,
	LowerLeft, LowerCenter, LowerRight
};

enum class ETextOverflow
{
	Clamp,
	Overflow,
	ResizeFreely,
	ResizeHeight
};

struct TextComponent
{
	std::string Text = "New Text";
	std::string FontPath;
	glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float FontSize = 24.0f;
	ETextAnchor Anchor = ETextAnchor::MiddleCenter;
	bool RichText = false;
	bool AlignByGeometry = false;
	bool ResizeBestFit = false;
	int32_t ResizeBestFitMinSize = 10;
	int32_t ResizeBestFitMaxSize = 40;
	float LineSpacing = 0.0f;
	ETextOverflow Overflow = ETextOverflow::Overflow;
};

}
