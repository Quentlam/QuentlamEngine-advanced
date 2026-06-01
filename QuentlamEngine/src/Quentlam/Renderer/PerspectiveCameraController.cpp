#include "qlpch.h"
#include "PerspectiveCameraController.h"

#include "Quentlam/Core/Input.h"
#include "Quentlam/Core/KeyCodes.h"
#include <algorithm>

namespace Quentlam
{
	PerspectiveCameraController::PerspectiveCameraController(float fov, float aspectRatio)
		: m_AspectRatio(aspectRatio), m_ZoomLevel(fov), m_Camera(m_ZoomLevel, m_AspectRatio)
	{
		m_Camera.SetPosition(m_CameraPosition);
	}

	void PerspectiveCameraController::OnUpdate(Timestep ts)
	{
		QL_PROFILE_FUNCTION();

		// Rotate
		if (Input::IsKeyPressed(Key::Left))
			m_CameraRotation.y += m_CameraRotationSpeed * ts; // Yaw Left
		else if (Input::IsKeyPressed(Key::Right))
			m_CameraRotation.y -= m_CameraRotationSpeed * ts; // Yaw Right

		if (Input::IsKeyPressed(Key::Up))
			m_CameraRotation.x += m_CameraRotationSpeed * ts; // Pitch Up
		else if (Input::IsKeyPressed(Key::Down))
			m_CameraRotation.x -= m_CameraRotationSpeed * ts; // Pitch Down

		// Clamp pitch to avoid flipping
		m_CameraRotation.x = std::clamp(m_CameraRotation.x, -89.0f, 89.0f);

		// Mouse Rotate
		glm::vec2 currentMousePos = { Input::GetMouseX(), Input::GetMouseY() };
		bool isRightMousePressed = Input::IsMouseButtonPressed(Mouse::ButtonRight);
		if (isRightMousePressed)
		{
			if (m_RightMouseWasPressed)
			{
				glm::vec2 delta = (currentMousePos - m_InitialMousePosition) * 0.1f; // Sensitivity

				m_CameraRotation.y -= delta.x * m_CameraRotationSpeed * ts; // Yaw
				m_CameraRotation.x -= delta.y * m_CameraRotationSpeed * ts; // Pitch
				
				// Clamp pitch to avoid flipping
				m_CameraRotation.x = std::clamp(m_CameraRotation.x, -89.0f, 89.0f);
			}
		}
		m_RightMouseWasPressed = isRightMousePressed;
		m_InitialMousePosition = currentMousePos;

		// Calculate Forward/Right vectors for relative movement
		float yaw = glm::radians(m_CameraRotation.y);
		float pitch = glm::radians(m_CameraRotation.x);
		glm::vec3 forward = {
			cos(yaw) * cos(pitch),
			sin(pitch),
			sin(yaw) * cos(pitch)
		};
		// The engine seems to use a specific coordinate system. Assuming standard OpenGL:
		// Actually, in Quentlam, looking down -Z might mean:
		forward = {
			-sin(yaw) * cos(pitch),
			sin(pitch),
			-cos(yaw) * cos(pitch)
		};
		forward = glm::normalize(forward);
		
		glm::vec3 upRef = glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::abs(forward.y) > 0.999f)
			upRef = glm::vec3(0.0f, 0.0f, -1.0f);
			
		glm::vec3 right = glm::normalize(glm::cross(forward, upRef));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));

		// Move
		if (Input::IsKeyPressed(Key::W))
			m_CameraPosition += forward * (m_CameraTranslationSpeed * ts);
		else if (Input::IsKeyPressed(Key::S))
			m_CameraPosition -= forward * (m_CameraTranslationSpeed * ts);

		if (Input::IsKeyPressed(Key::A))
			m_CameraPosition -= right * (m_CameraTranslationSpeed * ts);
		else if (Input::IsKeyPressed(Key::D))
			m_CameraPosition += right * (m_CameraTranslationSpeed * ts);

		// Move Up/Down
		if (Input::IsKeyPressed(Key::E))
			m_CameraPosition += glm::vec3(0.0f, 1.0f, 0.0f) * (m_CameraTranslationSpeed * ts);
		else if (Input::IsKeyPressed(Key::Q))
			m_CameraPosition -= glm::vec3(0.0f, 1.0f, 0.0f) * (m_CameraTranslationSpeed * ts);

		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetRotation(m_CameraRotation);
	}

	void PerspectiveCameraController::OnEvent(Event& e)
	{
		QL_PROFILE_FUNCTION();
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(QL_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(QL_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
	}

	bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		QL_PROFILE_FUNCTION();
		
		float yaw = glm::radians(m_CameraRotation.y);
		float pitch = glm::radians(m_CameraRotation.x);
		glm::vec3 forward = {
			-sin(yaw) * cos(pitch),
			sin(pitch),
			-cos(yaw) * cos(pitch)
		};
		forward = glm::normalize(forward);

		// Move camera position forward/backward instead of changing FOV (ZoomLevel)
		m_CameraPosition += forward * (e.GetYOffset() * 2.0f);
		m_Camera.SetPosition(m_CameraPosition);

		return false;
	}

	bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		QL_PROFILE_FUNCTION();
		OnResize((float)e.GetWidth(), (float)e.GetHeight());
		return false;
	}

	void PerspectiveCameraController::OnResize(float width, float height)
	{
		if (height <= 0.0f)
			return;

		float aspect = (width <= 0.0f) ? 1.78f : (width / height);
		m_AspectRatio = aspect;
		m_Camera.SetPerspective(m_ZoomLevel, m_AspectRatio, 0.1f, 1000.0f);
	}
}
