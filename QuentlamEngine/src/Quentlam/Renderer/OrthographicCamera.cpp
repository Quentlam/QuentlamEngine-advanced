#include "qlpch.h"
#include "OrthographicCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>


namespace Quentlam
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		:m_ProjectionMatrix(glm::ortho(left,right,bottom,top,-1.0f,1.0f)),m_ViewMatrix(1.0f)
	{
		QL_PROFILE_FUNCTION();

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		QL_PROFILE_FUNCTION();

		float width = right - left;
		float height = top - bottom;
		if (width <= 0.0f || height <= 0.0f) return;

		m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}



	void OrthographicCamera::RecalculateViewMatrix()
	{
		QL_PROFILE_FUNCTION();

		glm::mat4 transform = 
		glm::translate(glm::mat4(1.0f), m_Position) 
			*
		glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));

		m_ViewMatrix = glm::inverse(transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	}
}

