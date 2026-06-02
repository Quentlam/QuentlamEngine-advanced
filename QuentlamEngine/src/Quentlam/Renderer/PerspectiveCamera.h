#pragma once

#include "qlpch.h"
#include <glm/glm.hpp>
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Camera.h"

namespace Quentlam
{
	class QUENTLAM_API PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera(float fov, float aspectRatio, float nearClip = 0.1f, float farClip = 1000.0f);

		void SetPerspective(float fov, float aspect, float nearClip, float farClip) override;
		void SetOrthographic(float left, float right, float bottom, float top);
		void SetViewportSize(uint32_t width, uint32_t height) override;

		const glm::vec3& GetPosition() const override { return m_Position; }
		void SetPosition(const glm::vec3& position) override { m_Position = position; RecalculateViewMatrix(); }

		const glm::vec3& GetRotation() const { return m_Rotation; }
		void SetRotation(const glm::vec3& rotation) override { m_Rotation = rotation; RecalculateViewMatrix(); }

		const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; }

		bool IsOrthographic() const { return m_IsOrthographic; }

		float GetFOV() const { return m_FOV; }
		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }

	private:
		void RecalculateViewMatrix();

	private:
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };

		float m_FOV = 60.0f;
		float m_AspectRatio = 1.78f;
		float m_NearClip = 0.1f;
		float m_FarClip = 1000.0f;
		bool m_IsOrthographic = false;
	};
}
