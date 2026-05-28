#pragma once
#include <glm/glm.hpp>

namespace Quentlam
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_Projection(projection) {}
		virtual ~Camera() = default;

		const glm::mat4& GetProjection() const { return m_Projection; }
		virtual glm::vec3 GetPosition() const { return { 0.0f, 0.0f, 0.0f }; }
		virtual void SetViewportSize(uint32_t width, uint32_t height) {}

	protected:
		glm::mat4 m_Projection{ 1.0f };
	};
}