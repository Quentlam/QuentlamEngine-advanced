#pragma once
#include <glm/glm.hpp>

namespace Quentlam
{
	enum class ELightType
	{
		Point,
		Directional,
		Spot
	};

	struct Light2DComponent
	{
		ELightType LightType = ELightType::Point;
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		float Range = 10.0f;
		float Angle = 30.0f;
		bool Enabled = true;
		float FadeDistance = 1.0f;
	};

	struct ShadowCaster2DComponent
	{
		bool CastShadows = true;
		float ShadowOpacity = 0.5f;
		int32_t ShadowLayer = 0;
	};
}
