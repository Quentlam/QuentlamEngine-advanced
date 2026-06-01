#pragma once
#include "Light2D.h"
#include <vector>

namespace Quentlam
{
	class QUENTLAM_API Light2DRenderer
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Begin();
		static void End();
		static void Flush();
		static void SubmitLight();
		static void DrawShadowCaster();

	private:
		static std::vector<Light2DComponent> s_LightBuffer;
	};
}
