#include "qlpch.h"

#include "Input.h"
#include "Application.h"
#include "../Renderer/Renderer.h"
#include "Quentlam/Resource/ResourceManager.h"
#include "Quentlam/Modding/ModdingModule.h"

#include <glfw/glfw3.h>

namespace Quentlam
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;


	Application::Application(const std::string& name, uint32_t width, uint32_t height)
	{
		QL_PROFILE_FUNCTION();
		QL_CORE_ASSERT(!s_Instance, "Application is Already Exist!");
		s_Instance = this;

		m_Window = Window::Create(WindowProps(name, width, height));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		ResourceManager::Init();
		Renderer::Init();

		std::filesystem::path exePath = std::filesystem::current_path() / "mods";
		ModdingModule::Get().Initialize(exePath.string());
		QL_CORE_INFO("ModdingModule initialized with mods directory: {0}", exePath.string());




		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

	}
	Application::~Application()
	{
		ResourceManager::Shutdown();
	}
	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}
	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}


	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)//
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));//分发器分发WindowCloseEvent事件--判断窗口是否关闭
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));//分发器分发WindowResizeEvent的事件--判断窗口是大小有变化

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
			break;
		}
	}

	void Application::Run()
	{ 
		QL_PROFILE_FUNCTION();

		// Target frame rate logic
		const float targetFPS = 120.0f;
		const float targetFrameTime = 1.0f / targetFPS;

		while (m_Running)
		{
			QL_PROFILE_SCOPE("RunLoop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;

			// If the time elapsed is less than our target frame time, wait
			if (timestep < targetFrameTime)
			{
				continue; // Or use std::this_thread::sleep_for for better CPU usage if desired
			}

			m_LastFrameTime = time;

			ResourceManager::Update();

			if (!m_Minimized)
			{
				{
					QL_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)layer->OnUpdate(timestep);
				}
				m_ImGuiLayer->Begin();
				{
					QL_PROFILE_SCOPE("LayerStack OnImGuiRender");

					for (Layer* layer : m_LayerStack)layer->OnImGuiLayer();
				}
				m_ImGuiLayer->End();
			}





			m_Window->OnUpdate();
		};
	}


	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		ModdingModule::Get().Shutdown();
		QL_CORE_INFO("ModdingModule shutdown complete.");
		m_Running = false;
		return true;
	}


	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		QL_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());


		m_Minimized = false;
		return false;
	}
}