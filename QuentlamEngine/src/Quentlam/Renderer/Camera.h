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
		const glm::mat4& GetProjectionMatrix() const { return m_Projection; }
		virtual const glm::vec3& GetPosition() const { static const glm::vec3 zero{0,0,0}; return zero; }
		virtual void SetPosition(const glm::vec3& pos) {}
		virtual void SetViewportSize(uint32_t width, uint32_t height) {}
		virtual void SetOrthographic(float left, float right, float bottom, float top) {}
		virtual void SetPerspective(float fov, float aspect, float nearClip, float farClip) {}
		virtual const glm::mat4& GetViewMatrix() const { static const glm::mat4 identity(1.0f); return identity; }
		virtual void SetRotation(const glm::vec3& rot) {}
		virtual const glm::mat4& GetViewProjectionMatrix() const { return m_Projection; }

	protected:
		glm::mat4 m_Projection{ 1.0f };
	};
}
