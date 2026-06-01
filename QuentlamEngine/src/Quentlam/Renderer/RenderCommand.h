#pragma once

#include "RendererAPI.h"
#include "Quentlam/Core/Base.h"
#include "Quentlam/Debug/Instrumentor.h"

namespace Quentlam
{
	class QUENTLAM_API RenderCommand
	{
	public:
		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}
		
		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x,y,width,height);
		}

		inline static void SetStencilTest(bool enable)
		{
			s_RendererAPI->SetStencilTest(enable);
		}
		inline static void SetStencilFunc(uint32_t func, int ref, uint32_t mask)
		{
			s_RendererAPI->SetStencilFunc(func, ref, mask);
		}
		inline static void SetStencilOp(uint32_t fail, uint32_t zfail, uint32_t zpass)
		{
			s_RendererAPI->SetStencilOp(fail, zfail, zpass);
		}
		inline static void SetStencilMask(uint32_t mask)
		{
			s_RendererAPI->SetStencilMask(mask);
		}
		inline static void SetDepthTest(bool enable)
		{
			s_RendererAPI->SetDepthTest(enable);
		}




		
		
		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}
		inline static void Init()
		{
			QL_PROFILE_FUNCTION();

			s_RendererAPI->Init();
		}




		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray,uint32_t count = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, count);
		}

	private:
		static RendererAPI* s_RendererAPI;

	};


}