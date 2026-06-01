#pragma once
#include <glm/glm.hpp>

namespace Quentlam
{

enum class ECanvasRenderMode
{
	ScreenSpaceOverlay,
	ScreenSpaceCamera,
	WorldSpace
};

struct CanvasComponent
{
	ECanvasRenderMode RenderMode = ECanvasRenderMode::ScreenSpaceOverlay;
	int32_t SortOrder = 0;
};

}
