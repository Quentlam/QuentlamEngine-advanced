#pragma once

#include "qlpch.h"
#include <glm/glm.hpp>
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Camera.h"

namespace Quentlam
{
	class QUENTLAM_API OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top);
		void SetProjection(float left, float right, float bottom, float top);
		void SetOrthographic(float left, float right, float bottom, float top) { SetProjection(left, right, bottom, top); }


		const glm::vec3& GetPosition()const { return m_Position; }
		float GetRotation()const { return m_Rotation; }
		void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
		void SetViewportSize(uint32_t width, uint32_t height) { SetProjection(-(float)width / 2.0f, (float)width / 2.0f, -(float)height / 2.0f, (float)height / 2.0f); }


		const glm::mat4& GetViewMatrix()const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix()const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix()const { return m_ViewProjectionMatrix; }

	private:
		void RecalculateViewMatrix();
	private:
		glm::mat4 m_ViewProjectionMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		float m_Rotation = 0.0f;
	};
}
