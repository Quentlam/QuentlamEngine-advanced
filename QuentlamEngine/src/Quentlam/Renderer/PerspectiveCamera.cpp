#include "qlpch.h"
#include "PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Quentlam
{
	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
	{
		SetPerspective(fov, aspectRatio, nearClip, farClip);
	}

	void PerspectiveCamera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip)
	{
		QL_PROFILE_FUNCTION();
		if (std::isnan(aspectRatio) || std::isinf(aspectRatio) || aspectRatio <= 0.0f) return;
		if (std::isnan(fov) || std::isinf(fov) || fov <= 0.0f) fov = 60.0f;

		m_NearClip = nearClip;
		m_FarClip = farClip;

		float n = glm::max(m_NearClip, 0.01f);
		float f = glm::max(m_FarClip, n + 1.0f);
		m_FOV = fov;
		m_AspectRatio = aspectRatio;
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, n, f);
		m_ViewProjectionMatrix = m_Projection * m_ViewMatrix;
	}

	void PerspectiveCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0) return;
		m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
		if (m_AspectRatio <= 0.0f || std::isnan(m_AspectRatio)) return;
		if (m_FOV <= 0.0f || std::isnan(m_FOV)) m_FOV = 60.0f;
		float n = glm::max(m_NearClip, 0.01f);
		float f = glm::max(m_FarClip, n + 1.0f);
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, n, f);
		m_ViewProjectionMatrix = m_Projection * m_ViewMatrix;
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		QL_PROFILE_FUNCTION();

		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
							 glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
							 glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * rotation;
		m_ViewMatrix = glm::inverse(transform);
		m_ViewProjectionMatrix = m_Projection * m_ViewMatrix;
	}
}
