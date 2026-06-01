#include "qlpch.h"
#include <filesystem>
#include <Quentlam.h>
//----------Entry Point --------------------------
#include <Quentlam/Core/EntryPoint.h>



#include "Platform/OpenGL/OpenGLShader.h"



#include "imgui/imgui.h"


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "EditorLayer.h"

namespace Quentlam
{

	class ExampleLayer : public Layer {
	public:
		ExampleLayer()
			: m_CameraController(1280.0f / 720.0f), Layer("Example")
		{
			m_VertexArray = VertexArray::Create();
			float vertices[3 * 7] = {
		   -0.5f,-0.5f,0.0f,  0.8f,0.2f,0.8f,1.0f,
			0.5f,-0.5f,0.0f,  0.2f,0.3f,0.8f,1.0f,
			0.0f, 0.5f,0.0f,  0.8f,0.8f,0.2f,1.0f
			};

			Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
			vertexBuffer->SetLayout({
				{ShaderDataType::Float3,"a_Position"},
				{ShaderDataType::Float4,"a_Color"}
				});
			m_VertexArray->AddVertexBuffer(vertexBuffer);


			std::vector<uint32_t> indices =
			{
			  0,1,2
			};
			Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
			m_VertexArray->SetIndexBuffer(indexBuffer);


			m_BlueVertexArray = VertexArray::Create();

			float BlueVertices[5 * 4] =
			{
				-0.5f, 0.5f,0.0f,0.0f,1.0f,
				-0.5f,-0.5f,0.0f,0.0f,0.0f,
				 0.5f,-0.5f,0.0f,1.0f,0.0f,
				 0.5f, 0.5f,0.0f,1.0f,1.0f
			};


			Ref<VertexBuffer> BlueVB = VertexBuffer::Create(BlueVertices, sizeof(BlueVertices));
			BufferLayout BlueLayout = {
				{ ShaderDataType::Float3,"b_Position" },
				{ ShaderDataType::Float2,"TexCoord" }
			};
			BlueVB->SetLayout(BlueLayout);


			std::vector<uint32_t> BlueIndices = { 0,1,2,2,3,0 };
			Ref<IndexBuffer> BlueIB = IndexBuffer::Create(BlueIndices.data(), (uint32_t)BlueIndices.size());


			m_BlueVertexArray->SetIndexBuffer(BlueIB);
			m_BlueVertexArray->AddVertexBuffer(BlueVB);


			m_Shader = m_ShaderLibrary.Load("Colorful Square shader", "assets/shaders/ColorChangeShader.glsl");
			m_BlueShader = m_ShaderLibrary.Load("Blue Square shader", "assets/shaders/BlueShader.glsl");
			m_Texture2DShader = m_ShaderLibrary.Load("Texture shader", "assets/shaders/Texture.glsl");



			m_Texture2D_child = Texture2D::Create("assets/texture/child.jpg");
			m_Texture2D_merlin = Texture2D::Create("assets/texture/Merlin.png");



			std::dynamic_pointer_cast<OpenGLShader>(m_Texture2DShader)->Bind();
			std::dynamic_pointer_cast<OpenGLShader>(m_Texture2DShader)->UploadUniformInt("u_Texture", 0);
		}

		void OnUpdate(Timestep ts) override
		{
			m_CameraController.OnUpdate(ts);
			RenderCommand::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
			RenderCommand::Clear();


			std::dynamic_pointer_cast<OpenGLShader>(m_BlueShader)->Bind();
			std::dynamic_pointer_cast<OpenGLShader>(m_BlueShader)->UploadUniformFloat4("u_Color", m_square_Color);


			if (Input::IsKeyPressed(Key::J))
			{
				m_SquarePosition.x -= m_SquareSpeed * ts;
			}
			if (Input::IsKeyPressed(Key::L))
			{
				m_SquarePosition.x += m_SquareSpeed * ts;
			}
			if (Input::IsKeyPressed(Key::K))
			{
				m_SquarePosition.y -= m_SquareSpeed * ts;
			}
			if (Input::IsKeyPressed(Key::I))
			{
				m_SquarePosition.y += m_SquareSpeed * ts;
			}

			Renderer::BeginScene(m_CameraController.GetCamera());
			m_CameraController.GetCamera();
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

			glm::mat4 transfrom = glm::translate(glm::mat4(1.0f), m_SquarePosition);




			Renderer::Submit(m_Shader, m_VertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(m_trangle_scale)));

			Renderer::Submit(m_Shader, m_BlueVertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(m_Square_scale)) * transfrom);

			Renderer::Submit(m_Shader, m_BlueVertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(m_Square2_scale)));



			for (int i = 0; i < 20; i++)
			{
				for (int j = 0; j < 20; j++)
				{
					glm::vec3 pos(i * 0.11f, j * 0.11f, 0.0f);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
					Renderer::Submit(m_BlueShader, m_BlueVertexArray, transform);
				}
			}

			m_Texture2D_child->Bind();
			Renderer::Submit(m_Texture2DShader, m_BlueVertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(m_child_scale)));


			m_Texture2D_merlin->Bind();
			Renderer::Submit(m_Texture2DShader, m_BlueVertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(m_merlin_scale)));


			Renderer::EndScene();
		}

		void OnEvent(Event& event) override
		{
			m_CameraController.OnEvent(event);

			if (event.GetEventType() == EventType::WindowResize)
			{
				auto& re = (WindowResizeEvent&)event;

				float zoom = (float)re.GetWidth() / 1280.0f;
				m_CameraController.SetZoomLevel(zoom);
			}
		}

		void OnImGuiLayer() override
		{
			ImGui::Begin("Test");
			ImGui::Text("Just for Test");
			ImGui::ColorEdit4("square color", glm::value_ptr(m_square_Color));
			ImGui::SliderFloat("trangle scale slider", &m_trangle_scale, 0.1f, 2.0f);
			ImGui::SliderFloat("square scale slider", &m_Square_scale, 0.1f, 2.0f);
			ImGui::SliderFloat("square2 scale slider", &m_Square2_scale, 0.1f, 2.0f);
			ImGui::SliderFloat("child scale slider", &m_child_scale, 0.1f, 2.0f);
			ImGui::SliderFloat("merlin scale slider", &m_merlin_scale, 0.1f, 2.0f);
			ImGui::End();
		}


	private:
		ShaderLibrary					m_ShaderLibrary;
		Ref<Shader>			m_Shader;
		Ref<Shader>			m_BlueShader;
		Ref<Shader>			m_Texture2DShader;

		Ref<VertexArray>	m_VertexArray;
		Ref<VertexArray>	m_BlueVertexArray;
		Ref<Texture2D>		m_Texture2D_child;
		Ref<Texture2D>		m_Texture2D_merlin;

		OrthographicCameraController  m_CameraController;

		glm::vec3 m_SquarePosition = { 0.0f ,0.0f,0.0f };
		glm::vec4 m_square_Color{ 0.8f, 0.2f, 0.3f, 1.0f };
		glm::vec4 m_SecondLineColor{ 0.3f, 0.2f, 0.8f, 1.0f };


		float m_trangle_scale = 1.0f;
		float m_Square_scale = 1.0f;
		float m_Square2_scale = 1.0f;
		float m_child_scale = 1.0f;
		float m_merlin_scale = 1.0f;
		float m_SquareSpeed = 5.0f;
		float m_CameraMoveSpeed = 5.0f;
		float m_CameraRotation = 0.0f;
		float m_CameraRotationSpeed = 180.0f;


	};

	class QLEditor :public Application
	{
	public:
		QLEditor()
			:Application("QLEditor")
		{
			// ---- Resolve working directory ----
			// Climb from the current directory upward until we find a folder
			// that contains the "assets" directory. This correctly handles:
			//   - Launch from VS (CWD = QL-Editor\bin\Debug-windows-x86_64\QL-Editor\)
			//   - Launch from solution root (CWD = E:\QuentlamEngine\QuentlamEngine\)
			//   - Launch from any subdirectory
		try
		{
			std::filesystem::path currentPath = std::filesystem::current_path();
			std::filesystem::path targetPath = currentPath;

			while (!targetPath.empty())
			{
				// Prioritize Sandbox/assets as the project root (game content lives in Sandbox/)
				// Fall back to QL-Editor/assets only if Sandbox/assets doesn't exist at that level
				if (std::filesystem::exists(targetPath / "Sandbox" / "assets"))
				{
					targetPath = targetPath / "Sandbox";
					break;
				}
				if (std::filesystem::exists(targetPath / "assets"))
				{
					break;
				}

				std::filesystem::path parent = targetPath.parent_path();
				if (parent == targetPath || parent.empty())
					break;
				targetPath = parent;
			}

			if (targetPath != currentPath && !targetPath.empty())
				{
					std::filesystem::current_path(targetPath);
					QL_CORE_INFO("Working directory resolved to: {0}", targetPath.string());
				}
				else
				{
					QL_CORE_INFO("Working directory unchanged: {0}", std::filesystem::current_path().string());
				}
		}
			catch (const std::exception& e)
			{
				QL_CORE_ERROR("Failed to resolve working directory: {0}", e.what());
			}

			PushLayer(new EditorLayer());

		}
		~QLEditor()
		{

		}
	};

	Application* CreateApplication()
	{
		return new QLEditor();
	}

}
