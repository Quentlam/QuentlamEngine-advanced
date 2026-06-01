#include "qlpch.h"
#include "Light2DRenderer.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{
	std::vector<Light2DComponent> Light2DRenderer::s_LightBuffer;

	void Light2DRenderer::Initialize()
	{
		QL_PROFILE_FUNCTION();
		QL_CORE_INFO("Light2DRenderer initialized");
	}

	void Light2DRenderer::Shutdown()
	{
		QL_PROFILE_FUNCTION();
		QL_CORE_INFO("Light2DRenderer shutdown");
		s_LightBuffer.clear();
	}

	void Light2DRenderer::Begin()
	{
		QL_PROFILE_FUNCTION();
	}

	void Light2DRenderer::End()
	{
		QL_PROFILE_FUNCTION();
		Flush();
	}

	void Light2DRenderer::Flush()
	{
		QL_PROFILE_FUNCTION();
	}

	void Light2DRenderer::SubmitLight()
	{
		QL_PROFILE_FUNCTION();
	}

	void Light2DRenderer::DrawShadowCaster()
	{
		QL_PROFILE_FUNCTION();
	}
}
