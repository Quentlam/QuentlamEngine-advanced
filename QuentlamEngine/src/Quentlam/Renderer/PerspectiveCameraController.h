#pragma once

#include "Quentlam/Renderer/PerspectiveCamera.h"
#include "Quentlam/Core/Timestep.h"
#include "Quentlam/Events/ApplicationEvent.h"
#include "Quentlam/Events/MouseEvent.h"

namespace Quentlam
{
	class QUENTLAM_API PerspectiveCameraController
	{
	public:
		PerspectiveCameraController(float fov, float aspectRatio);
		
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		void OnResize(float width, float height);

		PerspectiveCamera& GetCamera() { return m_Camera; }
		const PerspectiveCamera& GetCamera() const { return m_Camera; }

		float GetZoomLevel() const { return m_ZoomLevel; }
		void SetZoomLevel(float level) { m_ZoomLevel = level; m_Camera.SetPerspective(level, m_AspectRatio, 0.1f, 1000.0f); }
		glm::vec3 GetPosition() const { return m_CameraPosition; }
		glm::vec3 GetRotation() const { return m_CameraRotation; }
		void SetPosition(const glm::vec3& pos) { m_CameraPosition = pos; m_Camera.SetPosition(pos); }
		void SetRotation(const glm::vec3& rot) { m_CameraRotation = rot; }
	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
	private:	
		float m_AspectRatio;
		float m_ZoomLevel;
		PerspectiveCamera m_Camera;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 5.0f }; // Step back so we can see 0,0,0
		glm::vec3 m_CameraRotation = { 0.0f, 0.0f, 0.0f }; // Pitch, Yaw, Roll

		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 90.0f;
		
		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };
		bool m_RightMouseWasPressed = false;
	};
}
