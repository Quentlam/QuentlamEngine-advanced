#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>

namespace Quentlam
{
	class QUENTLAM_API CubemapTexture
	{
	public:
		static Ref<CubemapTexture> Create(const std::string& right, const std::string& left,
			const std::string& top, const std::string& bottom,
			const std::string& front, const std::string& back);

		static Ref<CubemapTexture> CreateHDRI(const std::string& filepath);

		static Ref<CubemapTexture> CreateProceduralSkybox();

		uint32_t GetRendererID() const { return m_RendererID; }

	private:
		uint32_t m_RendererID = 0;
	};
}
