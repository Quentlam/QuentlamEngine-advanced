#include <windef.h>
#include <WinBase.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <Quentlam.h>
#include "EditorLayer.h"
#include "EditorToolbarLayout.h"
#include "Quentlam/Events/ApplicationEvent.h"
#include "Quentlam/Physics/Physics3D.h"
#include "Quentlam/Debug/CrashReporter.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include "Quentlam/Scene/SceneManager.h"
#include "Quentlam/Scene/SpriteRendererComponent.h"
#include "Quentlam/Scene/SpriteAnimationComponent.h"
#include "Quentlam/Renderer/Material.h"
#include "Quentlam/Gameplay/AnimationModule.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <ImGuizmo.h>

// OpenGL Stencil definitions
#define GL_KEEP                           0x1E00
#define GL_REPLACE                        0x1E01
#define GL_ALWAYS                         0x0207
#define GL_NOTEQUAL                       0x0205

#include "Quentlam/Renderer/VertexArray.h"
#include "Glad/include/glad/glad.h"
#include <chrono>
#include <excpt.h>
#include <filesystem>

namespace Quentlam
{
	namespace
	{
		constexpr float kToolbarTransitionDurationSeconds = 0.2f;
		constexpr ImU32 kToolbarPanelBackground = IM_COL32(45, 48, 54, 255);
		constexpr ImU32 kToolbarPanelBorder = IM_COL32(78, 82, 90, 255);

		struct ToolbarButtonTheme
		{
			ImVec4 Normal = ImVec4(0.19f, 0.21f, 0.24f, 1.0f);
			ImVec4 Hovered = ImVec4(0.26f, 0.28f, 0.31f, 1.0f);
			ImVec4 Active = ImVec4(0.31f, 0.34f, 0.38f, 1.0f);
			ImVec4 Toggled = ImVec4(0.24f, 0.31f, 0.42f, 1.0f);
			ImVec4 Border = ImVec4(0.35f, 0.38f, 0.44f, 1.0f);
			ImVec4 Tint = ImVec4(0.92f, 0.94f, 0.97f, 1.0f);
		};

		bool HasToolbarIcon(const Ref<Texture2D>& icon)
		{
			return icon && icon->GetRendererID() != 0;
		}

		void DrawToolbarGroupBackground(const ImVec2& min, const ImVec2& max, float rounding)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(min, max, kToolbarPanelBackground, rounding);
			drawList->AddRect(min, max, kToolbarPanelBorder, rounding);
		}

#if defined(_MSC_VER)
		int EditorExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
		{
			// If it's a C++ exception (0xE06D7363), let it pass to C++ catch blocks
			if (code == 0xE06D7363)
			{
				return EXCEPTION_CONTINUE_SEARCH;
			}

			QL_CORE_ERROR("Structured exception (0x{0:X}) caught in Editor. Generating crash dump...", code);
			CrashReporter::HandleCrash(ep);
			
			// Recover from the crash to prevent the editor from terminating!
			return EXCEPTION_EXECUTE_HANDLER;
		}
#endif

		bool ExecuteSceneRuntimeStart(Scene* scene, bool& started)
		{
			started = false;
#if defined(_MSC_VER)
			__try
			{
				started = scene && scene->OnRuntimeStart();
				return true;
			}
			__except (EditorExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
			{
				QL_CORE_ERROR("Play request triggered a structured exception ({0}) while starting runtime.", static_cast<unsigned int>(GetExceptionCode()));
				return false;
			}
#else
			started = scene && scene->OnRuntimeStart();
			return true;
#endif
		}

		void ExecuteSceneRuntimeStop(Scene* scene)
		{
#if defined(_MSC_VER)
			__try
			{
				if (scene)
					scene->OnRuntimeStop();
			}
			__except (EditorExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
			{
				QL_CORE_ERROR("Stop request triggered a structured exception ({0}) while leaving runtime.", static_cast<unsigned int>(GetExceptionCode()));
			}
#else
			if (scene)
				scene->OnRuntimeStop();
#endif
		}

		bool DrawToolbarActionButton(const char* id, const char* fallbackLabel, const Ref<Texture2D>& icon, const ImVec2& size, float iconInset, bool toggled = false)
		{
			const ToolbarButtonTheme theme;
			ImGui::PushID(id);
			bool clicked = false;
			if (HasToolbarIcon(icon))
			{
				clicked = ImGui::InvisibleButton("##ToolbarAction", size);
				const bool hovered = ImGui::IsItemHovered();
				const bool held = ImGui::IsItemActive();
				const ImVec2 min = ImGui::GetItemRectMin();
				const ImVec2 max = ImGui::GetItemRectMax();
				const ImVec4 background = held ? theme.Active : (hovered ? (toggled ? theme.Active : theme.Hovered) : (toggled ? theme.Toggled : theme.Normal));
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(min, max, ImGui::GetColorU32(background), 4.0f);
				drawList->AddRect(min, max, ImGui::GetColorU32(theme.Border), 4.0f);
				const ImVec2 imageMin(min.x + iconInset, min.y + iconInset);
				const ImVec2 imageMax(max.x - iconInset, max.y - iconInset);
				drawList->AddImage((ImTextureID)(intptr_t)icon->GetRendererID(), imageMin, imageMax, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(theme.Tint));
			}
			else
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Border, theme.Border);
				ImGui::PushStyleColor(ImGuiCol_Button, toggled ? theme.Toggled : theme.Normal);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, toggled ? theme.Active : theme.Hovered);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme.Active);
				clicked = ImGui::Button(fallbackLabel, size);
				ImGui::PopStyleColor(4);
				ImGui::PopStyleVar(2);
			}
			ImGui::PopID();
			return clicked;
		}
	}

	static void SetUEStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.38f, 0.38f, 0.45f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.23f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.ScrollbarRounding = 0.0f;
		style.TabRounding = 0.0f;
	}

	EditorLayer::EditorLayer()
		:Layer("EditorLayer"),
		m_SceneCamera(35.0f, 1280.0f / 720.0f),
		m_GameCamera(60.0f, 1280.0f / 720.0f),
		m_AnimatorEditorPanel(AnimatorEditorPanel::Get()),
		m_NavMeshEditorPanel(NavMeshEditorPanel::Get())
	{
		m_UIHierarchyPanel.SetUIGameModule(&UIGameModule::Get());
		m_AnimationEditorPanel.SetScene(m_ActiveScene.get());
		m_AtlasBuilderPanel.SetScene(m_ActiveScene.get());
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_IsSceneTransitioning || m_SceneState != SceneState::Edit)
			return;

		if (!m_ActiveScene)
		{
			QL_CORE_ERROR("Play request ignored: active scene is null.");
			return;
		}

		m_LastPlayFailure.clear();
		std::string validationFailure;
		if (!m_ActiveScene->ValidateRuntimeState(&validationFailure))
		{
			m_LastPlayFailure = validationFailure;
			QL_CORE_ERROR("Play request blocked by runtime validation: {0}", validationFailure);
			QL_CORE_WARN("Play request was safely rejected before entering runtime mode.");
			return;
		}

		m_IsSceneTransitioning = true;
		bool started = false;
		try
		{
			if (!ExecuteSceneRuntimeStart(m_ActiveScene.get(), started))
				m_LastPlayFailure = "Structured exception raised while starting runtime scene.";
		}
		catch (const std::exception& e)
		{
			m_LastPlayFailure = e.what();
			QL_CORE_ERROR("Play request failed with exception: {0}", e.what());
		}
		catch (...)
		{
			m_LastPlayFailure = "Unknown exception raised while starting runtime scene.";
			QL_CORE_ERROR("Play request failed with unknown exception.");
		}

		if (started)
		{
			m_ActiveScene->OnRuntimeStart();
			m_SceneState = SceneState::Play;
			m_bIsPaused = false;
			m_ToolbarTransitionProgress = 0.0f;
			Application::Get().GetImGuiLayer()->SetGameModeActive(true);
			m_CommandStack.Clear();
		}
		else
		{
			m_SceneState = SceneState::Edit;
			m_bIsPaused = false;
			if (m_LastPlayFailure.empty())
				m_LastPlayFailure = "Runtime start returned false without an explicit error.";
			QL_CORE_WARN("Play request was safely rejected before entering runtime mode.");
		}

		m_IsSceneTransitioning = false;
	}

	void EditorLayer::OnSceneStop()
	{
		if (m_IsSceneTransitioning || m_SceneState == SceneState::Edit)
			return;

		if (!m_ActiveScene)
		{
			m_SceneState = SceneState::Edit;
			m_bIsPaused = false;
			return;
		}

		m_IsSceneTransitioning = true;
		try
		{
			ExecuteSceneRuntimeStop(m_ActiveScene.get());
		}
		catch (const std::exception& e)
		{
			QL_CORE_ERROR("Stop request failed with exception: {0}", e.what());
		}
		catch (...)
		{
			QL_CORE_ERROR("Stop request failed with unknown exception.");
		}

		m_SceneState = SceneState::Edit;
		m_bIsPaused = false;
		m_ToolbarTransitionProgress = 0.0f;
		m_IsSceneTransitioning = false;
		Application::Get().GetImGuiLayer()->SetGameModeActive(false);
	}

	void EditorLayer::OnScenePause()
	{
		if (m_IsSceneTransitioning || m_SceneState == SceneState::Edit)
			return;

		m_SceneState = SceneState::Pause;
		m_bIsPaused = true;
		m_ToolbarTransitionProgress = 0.0f;
	}

	void EditorLayer::OnSceneStep()
	{
		if (m_IsSceneTransitioning || m_SceneState == SceneState::Edit)
			return;

		if (m_SceneState == SceneState::Pause && m_ActiveScene)
		{
			m_ActiveScene->OnStepFrame();
		}
	}

	void EditorLayer::ResumeScenePlay()
	{
		if (m_IsSceneTransitioning || m_SceneState != SceneState::Pause)
			return;

		m_SceneState = SceneState::Play;
		m_bIsPaused = false;
		m_ToolbarTransitionProgress = 0.0f;
	}

	void EditorLayer::OnAttach()
	{
		SetUEStyle();
		m_SceneCamera.SetZoomLevel(8.0f);

		// Configure ImGuizmo Style (UE-like)
		ImGuizmo::Style& style = ImGuizmo::GetStyle();
		style.TranslationLineThickness = 4.0f;
		style.TranslationLineArrowSize = 8.0f;
		style.RotationLineThickness = 4.0f;
		style.RotationOuterLineThickness = 3.0f;
		style.ScaleLineThickness = 4.0f;
		style.ScaleLineCircleSize = 8.0f;
		style.HatchedAxisLineThickness = 6.0f;
		style.CenterCircleSize = 6.0f;

		m_Texture2D = Texture2D::Create("assets/texture/background.png");



		m_DirectoryIcon = Texture2D::Create("assets/icons/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("assets/icons/FileIcon.png");
		m_IconFBX = Texture2D::Create("assets/icons/Icon_FBX.png");
		m_IconPNG = Texture2D::Create("assets/icons/Icon_PNG.png");
		m_IconWAV = Texture2D::Create("assets/icons/Icon_WAV.png");
		m_IconUASSET = Texture2D::Create("assets/icons/Icon_UASSET.png");
		m_IconUMAP = Texture2D::Create("assets/icons/Icon_UMAP.png");

		m_IconPlay = Texture2D::Create("assets/icons/PlayButton.png");
		m_IconPause = Texture2D::Create("assets/icons/PauseButton.png");
		m_IconStop = Texture2D::Create("assets/icons/StopButton.png");
		m_IconAdd = Texture2D::Create("assets/icons/AddButton.png");
		m_IconStep = Texture2D::Create("assets/icons/StepButton.png");

		m_OutlineShader = Shader::Create("assets/shaders/OutlineShader.glsl");
		glCreateVertexArrays(1, &m_EmptyVAO);
		m_SkyRenderer.Init();

		FrameBufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;

		m_Framebuffer = FrameBuffer::Create(fbSpec);
		m_GameFramebuffer = FrameBuffer::Create(fbSpec);

		Entity sceneCam, gameCam;
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->CreateDefaultScene(&sceneCam, &gameCam);
		m_SceneCameraEntity = sceneCam;
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		// Find the default cube entity created by the scene
		auto cubeView = m_ActiveScene->GetRegistry().view<TagComponent, CubeRendererComponent>();
		for (auto entity : cubeView)
		{
			auto& tag = cubeView.get<TagComponent>(entity);
			if (tag.Tag == "Default Cube")
			{
				m_CubeEntity = Entity(entity, m_ActiveScene.get());
				break;
			}
		}

		// Initialize TileMap for editor
		m_EditorTileMap.SetName("EditorTileMap");
		m_TileMapEditor.SetTileMap(&m_EditorTileMap);
	}

	void EditorLayer::OnDetach()
	{
		QL_PROFILE_FUNCTION();

		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		Physics3D::Shutdown();

		if (m_EmptyVAO)
			glDeleteVertexArrays(1, &m_EmptyVAO);
	}

	void EditorLayer::OnEvent(Event& event)
	{
		if (m_Is2DCamera)
			m_SceneCamera.OnEvent(event);
		else
			m_SceneCamera.OnEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(QL_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.GetRepeatCount() > 0)
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		if (control && !shift && e.GetKeyCode() == Key::S)
		{
			if (m_SceneState == SceneState::Edit)
			{
				if (!m_ScenePath.empty())
				{
					SceneManager::Get().SaveScene(m_ScenePath);
					QL_CORE_INFO("Scene saved: {0}", m_ScenePath);
					m_ShowSaveNotification = true;
					m_SaveNotificationTimer = 2.0f;
				}
				else
				{
					QL_CORE_WARN("No scene path set. Use File > Save As.");
					m_ShowSaveNotification = true;
					m_SaveNotificationTimer = 3.0f;
				}
			}
			else
			{
				QL_CORE_WARN("Cannot save scene while playing. Please stop the scene first.");
				m_ShowSaveNotification = true;
				m_SaveNotificationTimer = 3.0f;
			}
		}

		if (control && !shift && e.GetKeyCode() == Key::O)
		{
			if (m_SceneState == SceneState::Edit)
			{
				QL_CORE_INFO("Scene open shortcut pressed. Use File > Open to load a scene.");
				m_ShowSaveNotification = true;
				m_SaveNotificationTimer = 2.0f;
			}
		}

		if (control && !shift && e.GetKeyCode() == Key::Z)
		{
			m_CommandStack.Undo();
			return true;
		}

		if (control && !shift && e.GetKeyCode() == Key::Y)
		{
			m_CommandStack.Redo();
			return true;
		}

		if (control && !shift && e.GetKeyCode() == Key::C)
		{
			if (m_SelectedEntity && m_SceneState == SceneState::Edit)
			{
				m_CopiedEntity = static_cast<entt::entity>(m_SelectedEntity);
				QL_CORE_INFO("Entity copied: {0}", m_SelectedEntity.GetComponent<TagComponent>().Tag);
			}
			return true;
		}

		if (control && !shift && e.GetKeyCode() == Key::V)
		{
			if (m_CopiedEntity != entt::null && m_SceneState == SceneState::Edit)
			{
				auto& reg = m_ActiveScene->GetRegistry();
				auto src = m_CopiedEntity;
				if (!reg.valid(src))
					return true;

				auto& srcTag = reg.get<TagComponent>(src).Tag;
				Entity newEntity;
				auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), srcTag + "_copy", &newEntity);
				m_CommandStack.Execute(cmd);

				auto dest = static_cast<entt::entity>(newEntity);
				if (!reg.valid(dest))
					return true;

				if (reg.all_of<TransformComponent>(src))
				{
					auto& srcTc = reg.get<TransformComponent>(src);
					if (reg.all_of<TransformComponent>(dest))
						reg.get<TransformComponent>(dest) = srcTc;
					else
						reg.emplace<TransformComponent>(dest, srcTc);
				}
				if (reg.all_of<SpriteRendererComponent>(src))
				{
					auto& srcSc = reg.get<SpriteRendererComponent>(src);
					if (reg.all_of<SpriteRendererComponent>(dest))
						reg.get<SpriteRendererComponent>(dest) = srcSc;
					else
						reg.emplace<SpriteRendererComponent>(dest, srcSc);
				}
				if (reg.all_of<SpriteTransformComponent>(src))
				{
					auto& srcStc = reg.get<SpriteTransformComponent>(src);
					if (reg.all_of<SpriteTransformComponent>(dest))
						reg.get<SpriteTransformComponent>(dest) = srcStc;
					else
						reg.emplace<SpriteTransformComponent>(dest, srcStc);
				}
				if (reg.all_of<TriangleRendererComponent>(src))
				{
					auto& srcTrc = reg.get<TriangleRendererComponent>(src);
					if (reg.all_of<TriangleRendererComponent>(dest))
						reg.get<TriangleRendererComponent>(dest) = srcTrc;
					else
						reg.emplace<TriangleRendererComponent>(dest, srcTrc);
				}
				if (reg.all_of<CubeRendererComponent>(src))
				{
					auto& srcCrc = reg.get<CubeRendererComponent>(src);
					if (reg.all_of<CubeRendererComponent>(dest))
						reg.get<CubeRendererComponent>(dest) = srcCrc;
					else
						reg.emplace<CubeRendererComponent>(dest, srcCrc);
				}
				if (reg.all_of<PrimitiveRendererComponent>(src))
				{
					auto& srcPrc = reg.get<PrimitiveRendererComponent>(src);
					if (reg.all_of<PrimitiveRendererComponent>(dest))
						reg.get<PrimitiveRendererComponent>(dest) = srcPrc;
					else
						reg.emplace<PrimitiveRendererComponent>(dest, srcPrc);
				}

				if (reg.all_of<TransformComponent>(dest))
				{
					auto& tc = reg.get<TransformComponent>(dest);
					tc.Transform[3][0] += 0.5f;
					tc.Transform[3][1] += 0.5f;
				}
				m_SelectedEntity = newEntity;
			}
			return true;
		}

		switch (e.GetKeyCode())
		{
			case Key::Delete:
			{
				if (m_SelectedEntity && m_SceneState == SceneState::Edit)
				{
					auto cmd = std::make_shared<DeleteEntityCommand>(
						m_ActiveScene.get(),
						static_cast<entt::entity>(m_SelectedEntity));
					m_CommandStack.Execute(cmd);
					m_SelectedEntity = {};
				}
				break;
			}
			case Key::A:
			{
				if (control && shift)
				{
					m_ShowQuickAddPanel = !m_ShowQuickAddPanel;
				}
				break;
			}
			// Gizmos
			case Key::Q:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = -1;
				break;
			case Key::W:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case Key::E:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			case Key::R:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::SCALE;
				break;
			case Key::F:
			{
				if (!m_Is2DCamera)
				{
					if (m_SelectedEntity && m_SelectedEntity.HasComponent<TransformComponent>())
					{
						auto& tc = m_SelectedEntity.GetComponent<TransformComponent>().Transform;
						m_SceneCamera.SetPosition({ tc[3].x, tc[3].y, 0.0f });
						m_SceneCamera.SetZoomLevel(5.0f);
					}
					else
					{
						m_SceneCamera.SetPosition({ 0.0f, 0.0f, 0.0f });
						m_SceneCamera.SetZoomLevel(12.0f);
					}
					// Update projection matrix
					m_SceneCamera.OnResize(m_ViewportSize.x, m_ViewportSize.y);
				}
				break;
			}
		}

		return false;
	}

	Camera* EditorLayer::GetSceneCameraForRendering(bool is3D)
	{
		if (is3D)
			return &m_SceneCamera.GetCamera();
		return &m_SceneCamera.GetCamera();
	}

	void EditorLayer::RenderSceneContent3D(bool is3D, const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj)
	{
		if (!m_ActiveScene) return;

		// Sky
		if (m_ShowSkybox)
		{
			if (is3D)
				m_SkyRenderer.Render(view, proj, camPos);
			else
				m_SkyRenderer.RenderOrthographic(camPos);
		}

		// Default cube
		if (m_CubeEntity)
		{
			auto& tc = m_CubeEntity.GetComponent<TransformComponent>();
			Renderer3D::DrawCube(tc.Transform, { 0.8f, 0.2f, 0.3f, 1.0f }, (int)(uint32_t)m_CubeEntity);
		}

		// All cubes from registry
		auto cubeView = m_ActiveScene->GetRegistry().view<TransformComponent, CubeRendererComponent>();
		cubeView.each([](auto entityID, auto& tc, auto& cube)
		{
			Renderer3D::DrawCube(tc.Transform, cube.Color, (int)(uint32_t)entityID, cube.AmbientStrength, cube.DiffuseStrength, cube.SpecularStrength, cube.Shininess);
		});

		// Primitives
		auto primView = m_ActiveScene->GetRegistry().view<TransformComponent, PrimitiveRendererComponent>();
		primView.each([is3D](auto entityID, TransformComponent& tc, PrimitiveRendererComponent& prim)
		{
			if (is3D)
				Renderer3D::DrawCube(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
			else
			{
				if (prim.Type == PrimitiveRendererComponent::PrimitiveType::Capsule)
					Renderer3D::DrawCapsule(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
				else
					Renderer3D::DrawCube(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
			}
		});

		// 3D colliders
		if (m_ShowPhysicsColliders)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(2.0f);
			auto bc3dView = m_ActiveScene->GetRegistry().view<TransformComponent, BoxCollider3DComponent>();
			for (auto entity : bc3dView)
			{
				auto [tc, bc3d] = bc3dView.get<TransformComponent, BoxCollider3DComponent>(entity);
				if (!bc3d.ShowCollider) continue;
				glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
				glm::vec3 position = tc.Transform[3];
				glm::vec3 offset = bc3d.Offset;
				glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
				rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
				rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), bc3d.HalfExtent * scale * 2.0f);
				Renderer3D::DrawCube(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// Model
		if (m_Model && m_ModelEntity)
		{
			auto& tc = m_ModelEntity.GetComponent<TransformComponent>();
			Renderer3D::DrawModel(tc.Transform, *m_Model, glm::vec4(1.0f), (int)(uint32_t)m_ModelEntity);
		}
	}

	void EditorLayer::RenderSceneToFramebuffer(Ref<FrameBuffer> fb, PerspectiveCamera& camera, const glm::vec2& size, bool is3D)
	{
		if (!m_ActiveScene) return;
		fb->Bind();
		fb->ClearAttachment(1, -1);
		RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
		RenderCommand::Clear();
		RenderCommand::SetDepthTest(true);

		// 3D rendering
		Renderer3D::BeginScene(camera);
		RenderSceneContent3D(is3D, camera.GetPosition(), camera.GetViewMatrix(), camera.GetProjectionMatrix());

		// 2D rendering
		Renderer3D::EndScene();
		Renderer2D::BeginScene(camera);

		// Sprites
		auto spriteView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteTransformComponent>();
		for (auto entity : spriteView)
		{
			auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
			if (sprite.Texture)
				Renderer2D::DrawQuad(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			else
				Renderer2D::DrawQuad(worldTransform, sprite.Color, (int)(uint32_t)entity);
		}
		auto srView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
		for (auto entity : srView)
		{
			auto [transform, src] = srView.get<TransformComponent, SpriteRendererComponent>(entity);
			Ref<Texture2D> texToDraw = src.SubTexture ? src.SubTexture->GetTexture() : src.Texture;
			auto* animComp = m_ActiveScene->GetRegistry().try_get<SpriteAnimationComponent>(entity);
			if (animComp && animComp->AtlasBinding)
			{
				auto subTex = animComp->GetCurrentSubTexture();
				if (subTex) texToDraw = subTex->GetTexture();
			}
			glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
			if (texToDraw)
				Renderer2D::DrawQuad(worldTransform, texToDraw, src.TilingFactor, src.Color, (int)(uint32_t)entity);
			else
				Renderer2D::DrawQuad(worldTransform, src.Color, (int)(uint32_t)entity);
		}

		// 2D colliders
		if (m_ShowPhysicsColliders && !is3D)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(2.0f);
			auto bc2dView = m_ActiveScene->GetRegistry().view<TransformComponent, BoxCollider2DComponent>();
			for (auto entity : bc2dView)
			{
				auto [tc, bc2d] = bc2dView.get<TransformComponent, BoxCollider2DComponent>(entity);
				if (!bc2d.ShowCollider) continue;
				glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
				glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
				glm::vec3 offset = glm::vec3(bc2d.Offset.x, bc2d.Offset.y, 0.0f);
				glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
				rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
				rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(bc2d.Size.x * scale.x, bc2d.Size.y * scale.y, 1.0f));
				Renderer2D::DrawQuad(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
			}
			auto triView = m_ActiveScene->GetRegistry().view<TransformComponent, TriangleCollider2DComponent>();
			for (auto entity : triView)
			{
				auto [tc, tc2d] = triView.get<TransformComponent, TriangleCollider2DComponent>(entity);
				if (!tc2d.ShowCollider) continue;
				glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
				glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
				glm::vec3 offset = glm::vec3(tc2d.Offset.x, tc2d.Offset.y, 0.0f);
				glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
				rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
				rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(tc2d.Size.x * scale.x, tc2d.Size.y * scale.y, 1.0f));
				Renderer2D::DrawTriangle(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		Renderer2D::EndScene();
		fb->UnBind();
	}

	void EditorLayer::RenderSceneToFramebuffer(Ref<FrameBuffer> fb, OrthographicCamera& camera, const glm::vec2& size, bool is3D)
	{
		if (!m_ActiveScene) return;
		fb->Bind();
		fb->ClearAttachment(1, -1);
		RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
		RenderCommand::Clear();
		RenderCommand::SetDepthTest(true);

		// 3D rendering
		Renderer3D::BeginScene(camera);
		RenderSceneContent3D(is3D, camera.GetPosition(), camera.GetViewMatrix(), camera.GetProjectionMatrix());

		// 2D rendering
		Renderer3D::EndScene();
		Renderer2D::BeginScene(camera);

		// Sprites
		auto spriteView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteTransformComponent>();
		for (auto entity : spriteView)
		{
			auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
			glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
			if (sprite.Texture)
				Renderer2D::DrawQuad(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
			else
				Renderer2D::DrawQuad(worldTransform, sprite.Color, (int)(uint32_t)entity);
		}
		auto srView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
		for (auto entity : srView)
		{
			auto [transform, src] = srView.get<TransformComponent, SpriteRendererComponent>(entity);
			Ref<Texture2D> texToDraw = src.SubTexture ? src.SubTexture->GetTexture() : src.Texture;
			auto* animComp = m_ActiveScene->GetRegistry().try_get<SpriteAnimationComponent>(entity);
			if (animComp && animComp->AtlasBinding)
			{
				auto subTex = animComp->GetCurrentSubTexture();
				if (subTex) texToDraw = subTex->GetTexture();
			}
			glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
			if (texToDraw)
				Renderer2D::DrawQuad(worldTransform, texToDraw, src.TilingFactor, src.Color, (int)(uint32_t)entity);
			else
				Renderer2D::DrawQuad(worldTransform, src.Color, (int)(uint32_t)entity);
		}

		// 2D colliders
		if (m_ShowPhysicsColliders && !is3D)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(2.0f);
			auto bc2dView = m_ActiveScene->GetRegistry().view<TransformComponent, BoxCollider2DComponent>();
			for (auto entity : bc2dView)
			{
				auto [tc, bc2d] = bc2dView.get<TransformComponent, BoxCollider2DComponent>(entity);
				if (!bc2d.ShowCollider) continue;
				glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
				glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
				glm::vec3 offset = glm::vec3(bc2d.Offset.x, bc2d.Offset.y, 0.0f);
				glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
				rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
				rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(bc2d.Size.x * scale.x, bc2d.Size.y * scale.y, 1.0f));
				Renderer2D::DrawQuad(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
			}
			auto triView = m_ActiveScene->GetRegistry().view<TransformComponent, TriangleCollider2DComponent>();
			for (auto entity : triView)
			{
				auto [tc, tc2d] = triView.get<TransformComponent, TriangleCollider2DComponent>(entity);
				if (!tc2d.ShowCollider) continue;
				glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
				glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
				glm::vec3 offset = glm::vec3(tc2d.Offset.x, tc2d.Offset.y, 0.0f);
				glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
				rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
				rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(tc2d.Size.x * scale.x, tc2d.Size.y * scale.y, 1.0f));
				Renderer2D::DrawTriangle(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		Renderer2D::EndScene();
		fb->UnBind();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		if (m_ShowSaveNotification)
		{
			m_SaveNotificationTimer -= ts;
			if (m_SaveNotificationTimer <= 0.0f)
				m_ShowSaveNotification = false;
		}

		// Always update camera - events are already blocked by BlockEvents when not focused
		if (m_Is2DCamera)
			m_SceneCamera.OnUpdate(ts);
		else
			m_SceneCamera.OnUpdate(ts);

		// Sync camera controller position to scene camera entity
		if (m_SceneCameraEntity && m_SceneCameraEntity.HasComponent<CameraComponent>())
		{
			auto& tc = m_SceneCameraEntity.GetComponent<TransformComponent>();
			if (m_Is2DCamera)
			{
				glm::vec3 pos = m_SceneCamera.GetPosition();
				tc.Transform[3][0] = pos.x;
				tc.Transform[3][1] = pos.y;
				tc.Transform[3][2] = pos.z;
			}
			else
			{
				glm::vec3 pos = m_SceneCamera.GetPosition();
				tc.Transform[3][0] = pos.x;
				tc.Transform[3][1] = pos.y;
				tc.Transform[3][2] = 10.0f; // 2D camera always at z=10
			}
		}

		// Scene update
		switch (m_SceneState)
		{
			case SceneState::Edit:
			case SceneState::Play:
			case SceneState::Pause:
			case SceneState::Step:
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Pause || m_SceneState == SceneState::Step)
					m_ActiveScene->OnUpdate(ts);
				else
					m_ActiveScene->OnUpdateRuntime(ts);
				break;
			}
		}

		if (!m_Is2DCamera)
		{
			glm::vec3 currentPos = m_SceneCamera.GetPosition();
			glm::vec3 targetPos = currentPos;
			if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Pause)
			{
				glm::vec3 newPos = currentPos + (targetPos - currentPos) * (2.0f * (float)ts);
				m_SceneCamera.SetPosition(newPos);
			}
		}
	}

	void EditorLayer::OnImGuiLayer()
	{
		QL_PROFILE_FUNCTION();

		ImGuizmo::BeginFrame();

		bool dockSpaceOpen = true;

		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}


		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;


		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("停靠空间", &dockSpaceOpen, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("文件"))
			{
			if (ImGui::MenuItem("新建场景", "Ctrl+N"))
			{
				if (m_SceneState == SceneState::Edit)
				{
					m_ActiveScene = CreateRef<Scene>();
					m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
					m_SelectedEntity = {};
					m_ScenePath.clear();
					QL_CORE_INFO("New scene created");
				}
			}
			if (ImGui::MenuItem("保存场景", "Ctrl+S"))
			{
				if (m_SceneState == SceneState::Edit)
				{
					if (!m_ScenePath.empty())
					{
						SceneManager::Get().SaveScene(m_ScenePath);
						QL_CORE_INFO("Scene saved: {0}", m_ScenePath);
						m_ShowSaveNotification = true;
						m_SaveNotificationTimer = 2.0f;
					}
					else
					{
						QL_CORE_WARN("No scene path set. Use File > Save As.");
						m_ShowSaveNotification = true;
						m_SaveNotificationTimer = 3.0f;
					}
				}
				else
				{
					QL_CORE_WARN("Cannot save scene while playing. Please stop the scene first.");
					m_ShowSaveNotification = true;
					m_SaveNotificationTimer = 3.0f;
				}
			}
			if (ImGui::MenuItem("另存为...", NULL))
			{
				if (m_SceneState == SceneState::Edit)
				{
					std::string savePath = "assets/scenes/Untitled.scene";
					SceneManager::Get().SaveScene(savePath);
					m_ScenePath = savePath;
					QL_CORE_INFO("Scene saved as: {0}", savePath);
					m_ShowSaveNotification = true;
					m_SaveNotificationTimer = 2.0f;
				}
				else
				{
					QL_CORE_WARN("Cannot save scene while playing. Please stop the scene first.");
					m_ShowSaveNotification = true;
					m_SaveNotificationTimer = 3.0f;
				}
			}
			if (ImGui::MenuItem("打开场景...", "Ctrl+O"))
			{
				if (m_SceneState == SceneState::Edit)
				{
					std::string loadPath = "assets/scenes/Default.scene";
					if (std::filesystem::exists(loadPath))
					{
						m_ActiveScene = CreateRef<Scene>();
						m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
						SceneSerializer serializer(m_ActiveScene.get());
						if (serializer.Deserialize(loadPath))
						{
							m_ScenePath = loadPath;
							m_SelectedEntity = {};
							QL_CORE_INFO("Scene loaded: {0}", loadPath);
							m_ShowSaveNotification = true;
							m_SaveNotificationTimer = 2.0f;
						}
					}
					else
					{
						QL_CORE_WARN("Scene file not found: {0}", loadPath);
						m_ShowSaveNotification = true;
						m_SaveNotificationTimer = 3.0f;
					}
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("退出"))
				Application::Get().Close();
			ImGui::EndMenu();
		}

			if (ImGui::BeginMenu("窗口"))
			{
			// ---- 场景视图 ----
			if (ImGui::BeginMenu("场景视图"))
			{
				ImGui::MenuItem("场景层级 (Hierarchy)", NULL, &m_ShowUIHierarchy);
				ImGui::MenuItem("属性 (Inspector)", NULL, nullptr, false);
				ImGui::EndMenu();
			}

			// ---- 浏览器 ----
			if (ImGui::BeginMenu("浏览器"))
			{
				ImGui::MenuItem("内容浏览器", NULL, &m_IsContentBrowserOpen);
				ImGui::MenuItem("资源浏览器", NULL, &m_ShowAssetBrowser);
				ImGui::EndMenu();
			}

			// ---- 编辑工具 ----
			if (ImGui::BeginMenu("编辑工具"))
			{
				ImGui::MenuItem("材质编辑器", NULL, &m_ShowMaterialEditor);
				ImGui::MenuItem("UI 画布编辑器", NULL, &m_ShowUICanvasEditor);
				ImGui::EndMenu();
			}

			// ---- 动画 ----
			if (ImGui::BeginMenu("动画"))
			{
				ImGui::MenuItem("动画编辑器", NULL, &m_ShowAnimationEditor);
				if (ImGui::MenuItem("动画状态机", NULL, m_AnimatorEditorPanel.IsOpen()))
					m_AnimatorEditorPanel.Toggle();
				ImGui::EndMenu();
			}

			// ---- 地图编辑 ----
			if (ImGui::BeginMenu("地图编辑"))
			{
				ImGui::MenuItem("瓦片地图编辑器", NULL, &m_ShowTileMapEditor);
				if (ImGui::MenuItem("导航网格编辑器", NULL, m_NavMeshEditorPanel.IsOpen()))
					m_NavMeshEditorPanel.Toggle();
				ImGui::Separator();
				ImGui::MenuItem("图集构建器", NULL, &m_ShowAtlasBuilder);
				ImGui::EndMenu();
			}

			// ---- 调试 ----
			if (ImGui::BeginMenu("调试"))
			{
				ImGui::MenuItem("控制台", NULL, &m_ShowConsole);
				ImGui::MenuItem("性能分析器", NULL, &m_ShowProfiler);
				ImGui::Separator();
				ImGui::MenuItem("显示物理碰撞箱", NULL, &m_ShowPhysicsColliders);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			ImGui::MenuItem("重置所有面板布局", NULL, nullptr);
			ImGui::EndMenu();
		}

			if (ImGui::BeginMenu("游戏对象"))
		{
			if (ImGui::BeginMenu("3D 对象"))
			{
				auto createPrimitive = [&](const char* name, PrimitiveRendererComponent::PrimitiveType type) {
					static int counter = 1;
					char buffer[64];
					snprintf(buffer, sizeof(buffer), "%s_%03d", name, counter++);
					Entity entity;
					auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), buffer, &entity);
					m_CommandStack.Execute(cmd);
					entity.AddComponent<PrimitiveRendererComponent>(type);
					m_SelectedEntity = entity;
				};

				if (ImGui::MenuItem("立方体")) createPrimitive("Cube", PrimitiveRendererComponent::PrimitiveType::Cube);
				if (ImGui::MenuItem("球体")) createPrimitive("Sphere", PrimitiveRendererComponent::PrimitiveType::Sphere);
				if (ImGui::MenuItem("圆柱体")) createPrimitive("Cylinder", PrimitiveRendererComponent::PrimitiveType::Cylinder);
				if (ImGui::MenuItem("胶囊体")) createPrimitive("Capsule", PrimitiveRendererComponent::PrimitiveType::Capsule);
				if (ImGui::MenuItem("圆锥体")) createPrimitive("Cone", PrimitiveRendererComponent::PrimitiveType::Cone);
				if (ImGui::MenuItem("圆环")) createPrimitive("Torus", PrimitiveRendererComponent::PrimitiveType::Torus);

				ImGui::Separator();

				if (ImGui::MenuItem("物理演示平面"))
				{
					Entity entity;
					auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), "PhysicsDemoPlane", &entity);
					m_CommandStack.Execute(cmd);
					auto& tc = entity.GetComponent<TransformComponent>();
					tc.Transform = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 0.1f, 10.0f));

					entity.AddComponent<CubeRendererComponent>(glm::vec4(0.5f, 0.8f, 0.5f, 1.0f));

					auto& rb3d = entity.AddComponent<Rigidbody3DComponent>();
					rb3d.Type = Rigidbody3DComponent::BodyType::Kinematic;
					rb3d.Mass = 1.0f;

					auto& bc3d = entity.AddComponent<BoxCollider3DComponent>();
					bc3d.HalfExtent = glm::vec3(5.0f, 0.05f, 5.0f);

					m_SelectedEntity = entity;
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("创建空对象 (Create Empty)"))
			{
				static int counter = 1;
				char buffer[64];
				snprintf(buffer, sizeof(buffer), "Empty_%03d", counter++);
				Entity entity;
				auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), buffer, &entity);
				m_CommandStack.Execute(cmd);
				m_SelectedEntity = entity;
			}

			if (ImGui::BeginMenu("2D 对象"))
			{
				if (ImGui::MenuItem("精灵 (Sprite)"))
				{
					static int counter = 1;
					char buffer[64];
					snprintf(buffer, sizeof(buffer), "Sprite_%03d", counter++);
					Entity entity;
					auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), buffer, &entity);
					m_CommandStack.Execute(cmd);
					entity.AddComponent<SpriteRendererComponent>();
					m_SelectedEntity = entity;
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("复制对象 (Duplicate)"))
			{
				if (m_SelectedEntity)
				{
					auto& oldTag = m_SelectedEntity.GetComponent<TagComponent>();
					auto newEntity = m_ActiveScene->CreateEntity(oldTag.Tag + "_copy");
					if (m_SelectedEntity.HasComponent<TransformComponent>())
						newEntity.AddComponent<TransformComponent>(m_SelectedEntity.GetComponent<TransformComponent>());
					if (m_SelectedEntity.HasComponent<SpriteRendererComponent>())
						newEntity.AddComponent<SpriteRendererComponent>(m_SelectedEntity.GetComponent<SpriteRendererComponent>());
					if (m_SelectedEntity.HasComponent<SpriteTransformComponent>())
						newEntity.AddComponent<SpriteTransformComponent>(m_SelectedEntity.GetComponent<SpriteTransformComponent>());
					if (m_SelectedEntity.HasComponent<TriangleRendererComponent>())
						newEntity.AddComponent<TriangleRendererComponent>(m_SelectedEntity.GetComponent<TriangleRendererComponent>());
					if (m_SelectedEntity.HasComponent<CubeRendererComponent>())
						newEntity.AddComponent<CubeRendererComponent>(m_SelectedEntity.GetComponent<CubeRendererComponent>());
					m_SelectedEntity = newEntity;
				}
			}

			ImGui::EndMenu();
		}

			if (ImGui::BeginMenu("组件 (Component)"))
		{
			if (m_SelectedEntity)
			{
				if (!m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
				{
					if (ImGui::MenuItem("渲染 / 精灵动画 (Sprite Animation)"))
					{
						m_SelectedEntity.AddComponent<SpriteAnimationComponent>();
					}
				}
			}
			ImGui::EndMenu();
		}

		// ---- 游戏控制 ----
		if (ImGui::BeginMenu("游戏"))
		{
			ImGui::Text("视图布局");
			ImGui::Separator();
			const char* layoutNames[] = { "场景视图", "游戏视图" };
			for (int i = 0; i < 2; i++)
			{
				bool selected = (int)m_ViewportLayout == i;
				if (ImGui::MenuItem(layoutNames[i], "", selected))
					m_ViewportLayout = (ViewportLayout)i;
			}

			ImGui::Separator();
			ImGui::Text("播放控制");
			ImGui::Separator();
			const bool isPlaying = m_SceneState == SceneState::Play;
			const bool isPaused = m_SceneState == SceneState::Pause;

			if (ImGui::MenuItem(isPlaying ? "停止" : "播放", "F5", isPlaying))
			{
				if (isPlaying) OnSceneStop();
				else OnScenePlay();
			}
			if (isPlaying || isPaused)
			{
				if (ImGui::MenuItem("暂停", "F6", isPaused))
					OnScenePause();
				if (ImGui::MenuItem("逐帧前进", "F10"))
					OnSceneStep();
			}
			if (isPaused)
			{
				if (ImGui::MenuItem("继续", "F6", false))
					ResumeScenePlay();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("显示物理碰撞箱", nullptr, m_ShowPhysicsColliders))
				m_ShowPhysicsColliders = !m_ShowPhysicsColliders;

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

		// Scene Hierarchy Panel
		ImGui::Begin("场景层级");

		static char searchBuffer[128] = {};
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("搜索", searchBuffer, sizeof(searchBuffer));
		ImGui::Separator();

		// Extract entities to sort
		std::vector<entt::entity> allEntities;
		for (auto entityID : m_ActiveScene->GetRegistry().view<TagComponent>())
		{
			if (searchBuffer[0] != '\0')
			{
				Entity e{ entityID, m_ActiveScene.get() };
				auto& tag = e.GetComponent<TagComponent>().Tag;
				std::string search = searchBuffer;
				std::string lowerTag = tag;
				std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
				std::transform(search.begin(), search.end(), search.begin(), ::tolower);
				if (lowerTag.find(search) == std::string::npos)
					continue;
			}
			allEntities.push_back(entityID);
		}

		auto drawEntityNode = [&](entt::entity entityID) {
			Entity entity{ entityID, m_ActiveScene.get() };
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			bool isSelected = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end();
			ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;

			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entityID, flags, tag.c_str());
			if (ImGui::IsItemClicked())
			{
				bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
				if (shift)
				{
					auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
					if (it != m_SelectedEntities.end())
						m_SelectedEntities.erase(it);
					else
						m_SelectedEntities.push_back(entity);
				}
				else
				{
					m_SelectedEntities.clear();
					m_SelectedEntities.push_back(entity);
				}
				m_SelectedEntity = entity;
			}

			bool entityDeleted = false;
			if (ImGui::BeginPopupContextItem())
			{
				if (entity.HasComponent<CameraComponent>())
				{
					bool isGameCam = entity.GetComponent<CameraComponent>().IsGameCamera;
					if (ImGui::MenuItem(isGameCam ? "✓ 设为游戏摄像机" : "设为游戏摄像机"))
					{
						// Clear all other cameras' IsGameCamera flag
						auto camView = m_ActiveScene->GetRegistry().view<CameraComponent>();
						for (auto e : camView)
						{
							m_ActiveScene->GetRegistry().get<CameraComponent>(e).IsGameCamera = false;
						}
						// Set this camera as game camera
						entity.GetComponent<CameraComponent>().IsGameCamera = true;
						m_ActiveScene->SetActiveGameCamera(entity);
					}
					ImGui::Separator();
				}
				if (ImGui::MenuItem("删除实体"))
					entityDeleted = true;
				ImGui::EndPopup();
			}

			if (opened)
			{
				ImGui::TreePop();
			}

			if (entityDeleted)
			{
				auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
				if (it != m_SelectedEntities.end())
					m_SelectedEntities.erase(it);
				if (m_SelectedEntity == entity)
					m_SelectedEntity = {};
				m_ActiveScene->GetRegistry().destroy(entityID);
			}
		};

		// Draw all entities as a flat list (generic engine, no game-specific grouping)
		for (auto entityID : allEntities)
		{
			drawEntityNode(entityID);
		}

		if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
		{
			m_SelectedEntities.clear();
			m_SelectedEntity = {};
		}

		if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("创建空实体"))
			{
				auto cmd = std::make_shared<CreateEntityCommand>(m_ActiveScene.get(), "Empty Entity", nullptr);
				m_CommandStack.Execute(cmd);
			}
			if (!m_SelectedEntities.empty() && ImGui::MenuItem(("删除选中实体 (" + std::to_string(m_SelectedEntities.size()) + ")").c_str()))
			{
				std::vector<std::shared_ptr<ICommand>> deleteCommands;
				deleteCommands.reserve(m_SelectedEntities.size());
				for (auto& e : m_SelectedEntities)
				{
					deleteCommands.push_back(std::make_shared<DeleteEntityCommand>(
						m_ActiveScene.get(), static_cast<entt::entity>(e)));
				}
				auto batchCmd = std::make_shared<BatchCommand>("Delete Entities", deleteCommands);
				m_CommandStack.Execute(batchCmd);
				m_SelectedEntities.clear();
				m_SelectedEntity = {};
			}
			ImGui::EndPopup();
		}
		ImGui::End();

		// Properties Panel
		ImGui::Begin("属性");
		if (m_SelectedEntity)
		{
			bool has2D = m_SelectedEntity.HasComponent<Rigidbody2DComponent>() || m_SelectedEntity.HasComponent<BoxCollider2DComponent>();
			bool has3D = m_SelectedEntity.HasComponent<Rigidbody3DComponent>() || m_SelectedEntity.HasComponent<BoxCollider3DComponent>();
			bool hasConflict = has2D && has3D;

			if (m_SelectedEntity.HasComponent<TagComponent>())
			{
				auto& tag = m_SelectedEntity.GetComponent<TagComponent>().Tag;
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, sizeof(buffer), tag.c_str());
				if (ImGui::InputText("标签", buffer, sizeof(buffer)))
				{
					tag = std::string(buffer);
				}
			}

			if (m_SelectedEntity.HasComponent<TransformComponent>())
			{
				// 使用 ImGui::TreeNodeEx 结合图标美化属性面板
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
				
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), treeNodeFlags, "🛠 Transform (变换)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& tc = m_SelectedEntity.GetComponent<TransformComponent>().Transform;
					
					float translation[3], rotation[3], scale[3];
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(tc), translation, rotation, scale);

					bool changed = false;
					if (ImGui::DragFloat3("位置 (Position)", translation, 0.1f)) changed = true;
					if (ImGui::DragFloat3("旋转 (Rotation)", rotation, 0.1f)) changed = true;
					if (ImGui::DragFloat3("缩放 (Scale)", scale, 0.1f)) changed = true;

					if (changed)
					{
						ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, glm::value_ptr(tc));
					}
					
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<SpriteTransformComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(SpriteTransformComponent).hash_code(), treeNodeFlags, "🎨 Sprite Renderer (2D渲染)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& color = m_SelectedEntity.GetComponent<SpriteTransformComponent>().Color;
					ImGui::Text("2D 精灵渲染组件，用于控制颜色和基础材质。");
					ImGui::Spacing();
					ImGui::ColorEdit4("基础颜色 (Base Color)", glm::value_ptr(color));
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<SpriteRendererComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(SpriteRendererComponent).hash_code(), treeNodeFlags, "🖼 Sprite Renderer2 (高级2D渲染)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& src = m_SelectedEntity.GetComponent<SpriteRendererComponent>();
					ImGui::Text("高级2D精灵渲染，支持材质、Tiling和翻转。");
					ImGui::Spacing();
					ImGui::ColorEdit4("颜色 (Color)", glm::value_ptr(src.Color));
					ImGui::DragFloat2("尺寸 (Size)", glm::value_ptr(src.Size), 0.1f);
					ImGui::DragFloat("Tiling Factor", &src.TilingFactor, 0.1f, 0.1f, 100.0f);
					ImGui::Checkbox("Flip X", &src.FlipX);
					ImGui::SameLine();
					ImGui::Checkbox("Flip Y", &src.FlipY);

					// Material selector
					{
						const char* preview = src.Material ? src.Material->GetName().c_str() : "(Default Sprite)";
						if (ImGui::BeginCombo("Material", preview))
						{
							if (ImGui::Selectable("(Default Sprite)", !src.Material))
								src.Material = nullptr;

							auto& defaults = MaterialDefaults::GetAllDefaults();
							for (auto& [name, mat] : defaults)
							{
								if (ImGui::Selectable(name.c_str(), src.Material && src.Material->GetName() == name))
									src.Material = mat;
							}

							ImGui::EndCombo();
						}
					}
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(SpriteAnimationComponent).hash_code(), treeNodeFlags, "🎞 Sprite Animation (精灵动画)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& anim = m_SelectedEntity.GetComponent<SpriteAnimationComponent>();
					ImGui::Checkbox("Auto Play", &anim.AutoPlay);
					if (anim.Animator)
					{
						ImGui::Text("State: %s", anim.Animator->IsPlaying() ? "Playing" : "Stopped");
						ImGui::Text("Current Frame: %d", anim.GetCurrentFrameIndex());
						auto clip = anim.GetCurrentClip();
						if (clip)
							ImGui::Text("Clip: %s", clip->GetName().c_str());

						if (ImGui::Button("Play"))
							anim.Play(anim.DefaultClipName);
						ImGui::SameLine();
						if (ImGui::Button("Stop"))
							anim.Stop();
						ImGui::SameLine();
						if (ImGui::Button("Pause"))
							anim.Pause();
					}
					else
					{
						ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "No animator set. Load an animation JSON to initialize.");
					}
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<TriangleRendererComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(TriangleRendererComponent).hash_code(), treeNodeFlags, "🔺 Triangle Renderer (2D三角形渲染)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& color = m_SelectedEntity.GetComponent<TriangleRendererComponent>().Color;
					ImGui::Text("2D 三角形渲染组件。");
					ImGui::Spacing();
					ImGui::ColorEdit4("基础颜色 (Base Color)", glm::value_ptr(color));
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<CubeRendererComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(CubeRendererComponent).hash_code(), treeNodeFlags, "🧊 Cube Renderer (3D渲染)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& color = m_SelectedEntity.GetComponent<CubeRendererComponent>().Color;
					ImGui::ColorEdit4("基础颜色 (Base Color)", glm::value_ptr(color));
					
					auto& cube = m_SelectedEntity.GetComponent<CubeRendererComponent>();
					ImGui::DragFloat("环境光 (Ambient)", &cube.AmbientStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("漫反射 (Diffuse)", &cube.DiffuseStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("高光强度 (Specular)", &cube.SpecularStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("高光粗糙度 (Shininess)", &cube.Shininess, 1.0f, 2.0f, 256.0f);

					ImGui::TreePop();
				}
			}
			
			if (m_SelectedEntity.HasComponent<PrimitiveRendererComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(PrimitiveRendererComponent).hash_code(), treeNodeFlags, "🔺 Primitive Renderer (基础图元渲染)");
				ImGui::PopStyleVar();

				if (open)
				{
					auto& prim = m_SelectedEntity.GetComponent<PrimitiveRendererComponent>();
					ImGui::ColorEdit4("基础颜色 (Base Color)", glm::value_ptr(prim.Color));
					
					int type = (int)prim.Type;
					const char* typeStrings[] = { "Cube", "Sphere", "Cylinder", "Capsule", "Cone", "Torus" };
					if (ImGui::Combo("图元类型 (Type)", &type, typeStrings, 6))
						prim.Type = (PrimitiveRendererComponent::PrimitiveType)type;

					if (prim.Type == PrimitiveRendererComponent::PrimitiveType::Sphere || 
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Cylinder || 
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Capsule ||
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Cone ||
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Torus)
					{
						ImGui::DragInt("分段数 (Segments)", &prim.Segments, 1, 3, 64);
						ImGui::DragFloat("半径 (Radius)", &prim.Radius, 0.05f, 0.1f, 10.0f);
					}

					if (prim.Type == PrimitiveRendererComponent::PrimitiveType::Cylinder || 
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Capsule ||
						prim.Type == PrimitiveRendererComponent::PrimitiveType::Cone)
					{
						ImGui::DragFloat("高度 (Height)", &prim.Height, 0.05f, 0.1f, 10.0f);
					}

					ImGui::DragFloat("环境光 (Ambient)", &prim.AmbientStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("漫反射 (Diffuse)", &prim.DiffuseStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("高光强度 (Specular)", &prim.SpecularStrength, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("高光粗糙度 (Shininess)", &prim.Shininess, 1.0f, 2.0f, 256.0f);

					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<Rigidbody2DComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				if (hasConflict) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(Rigidbody2DComponent).hash_code(), treeNodeFlags, "🏃 Rigidbody 2D (2D刚体)");
				if (hasConflict) ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				if (open)
				{
					if (hasConflict) ImGui::TextColored(ImVec4(1,0,0,1), "冲突: 同时存在3D组件! (Conflict with 3D components!)");
					auto& rb2d = m_SelectedEntity.GetComponent<Rigidbody2DComponent>();
					ImGui::Text("刚体类型 (Body Type)");
					int bodyType = (int)rb2d.Type;
					const char* bodyTypeStrings[] = { "静态 (Static)", "动态 (Dynamic)", "运动学 (Kinematic)" };
					if (ImGui::Combo("##BodyType", &bodyType, bodyTypeStrings, 3))
						rb2d.Type = (Rigidbody2DComponent::BodyType)bodyType;
					
					ImGui::Checkbox("固定旋转 (Fixed Rotation)", &rb2d.FixedRotation);
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<BoxCollider2DComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				if (hasConflict) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(BoxCollider2DComponent).hash_code(), treeNodeFlags, "📦 Box Collider 2D (2D碰撞体)");
				if (hasConflict) ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				if (open)
				{
					if (hasConflict) ImGui::TextColored(ImVec4(1,0,0,1), "冲突: 同时存在3D组件! (Conflict with 3D components!)");
					auto& bc2d = m_SelectedEntity.GetComponent<BoxCollider2DComponent>();
					ImGui::Checkbox("显示碰撞箱 (Show Collider)", &bc2d.ShowCollider);
					ImGui::DragFloat2("偏移 (Offset)", glm::value_ptr(bc2d.Offset), 0.1f);
					ImGui::DragFloat2("尺寸 (Size)", glm::value_ptr(bc2d.Size), 0.1f);
					ImGui::DragFloat("密度 (Density)", &bc2d.Density, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("摩擦力 (Friction)", &bc2d.Friction, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("弹性 (Restitution)", &bc2d.Restitution, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("弹性阈值 (Restitution Threshold)", &bc2d.RestitutionThreshold, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<TriangleCollider2DComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				if (hasConflict) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(TriangleCollider2DComponent).hash_code(), treeNodeFlags, "🔺 Triangle Collider 2D (2D三角形碰撞体)");
				if (hasConflict) ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				if (open)
				{
					if (hasConflict) ImGui::TextColored(ImVec4(1,0,0,1), "冲突: 同时存在3D组件! (Conflict with 3D components!)");
					auto& tc2d = m_SelectedEntity.GetComponent<TriangleCollider2DComponent>();
					ImGui::Checkbox("显示碰撞箱 (Show Collider)", &tc2d.ShowCollider);
					ImGui::DragFloat2("偏移 (Offset)", glm::value_ptr(tc2d.Offset), 0.1f);
					ImGui::DragFloat2("尺寸 (Size)", glm::value_ptr(tc2d.Size), 0.1f);
					ImGui::DragFloat("密度 (Density)", &tc2d.Density, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("摩擦力 (Friction)", &tc2d.Friction, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("弹性 (Restitution)", &tc2d.Restitution, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("弹性阈值 (Restitution Threshold)", &tc2d.RestitutionThreshold, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<Rigidbody3DComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				if (hasConflict) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(Rigidbody3DComponent).hash_code(), treeNodeFlags, "🏃 Rigidbody 3D (3D刚体)");
				if (hasConflict) ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				if (open)
				{
					if (hasConflict) ImGui::TextColored(ImVec4(1,0,0,1), "冲突: 同时存在2D组件! (Conflict with 2D components!)");
					auto& rb3d = m_SelectedEntity.GetComponent<Rigidbody3DComponent>();
					ImGui::Text("刚体类型 (Body Type)");
					int bodyType = (int)rb3d.Type;
					const char* bodyTypeStrings[] = { "静态 (Static)", "动态 (Dynamic)", "运动学 (Kinematic)" };
					if (ImGui::Combo("##BodyType3D", &bodyType, bodyTypeStrings, 3))
						rb3d.Type = (Rigidbody3DComponent::BodyType)bodyType;
					
					if (rb3d.Type != Rigidbody3DComponent::BodyType::Static)
					{
						ImGui::DragFloat("质量 (Mass)", &rb3d.Mass, 0.1f, 0.01f, 1000.0f);
					}
					ImGui::TreePop();
				}
			}

			if (m_SelectedEntity.HasComponent<BoxCollider3DComponent>())
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				if (hasConflict) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::Separator();
				bool open = ImGui::TreeNodeEx((void*)typeid(BoxCollider3DComponent).hash_code(), treeNodeFlags, "📦 Box Collider 3D (3D碰撞体)");
				if (hasConflict) ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				if (open)
				{
					if (hasConflict) ImGui::TextColored(ImVec4(1,0,0,1), "冲突: 同时存在2D组件! (Conflict with 2D components!)");
					auto& bc3d = m_SelectedEntity.GetComponent<BoxCollider3DComponent>();
					ImGui::Checkbox("显示碰撞箱 (Show Collider)", &bc3d.ShowCollider);
					ImGui::DragFloat3("偏移 (Offset)", glm::value_ptr(bc3d.Offset), 0.1f);
					ImGui::DragFloat3("半尺寸 (Half Extent)", glm::value_ptr(bc3d.HalfExtent), 0.1f);
					ImGui::DragFloat("摩擦力 (Friction)", &bc3d.Friction, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("弹性 (Restitution)", &bc3d.Restitution, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}
			}


			ImGui::Separator();
			
			float windowWidth = ImGui::GetWindowSize().x;
			float buttonWidth = 150.0f;
			ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
			if (ImGui::Button("添加组件 (Add Component)", ImVec2(buttonWidth, 30.0f)))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			bool showComponentError = false;

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				bool has2D = m_SelectedEntity.HasComponent<Rigidbody2DComponent>() || m_SelectedEntity.HasComponent<BoxCollider2DComponent>();
				bool has3D = m_SelectedEntity.HasComponent<Rigidbody3DComponent>() || m_SelectedEntity.HasComponent<BoxCollider3DComponent>();

				auto addComponentSafe = [&](const char* name, bool is2DComp, bool is3DComp, auto addFunc) {
					if (ImGui::MenuItem(name))
					{
						if ((is2DComp && has3D) || (is3DComp && has2D))
						{
							showComponentError = true;
						}
						else
						{
							addFunc();
						}
					}
				};

				if (!m_SelectedEntity.HasComponent<TransformComponent>())
				{
					addComponentSafe("变换 (Transform)", false, false, [&]() { m_SelectedEntity.AddComponent<TransformComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<SpriteTransformComponent>())
				{
					addComponentSafe("2D 精灵渲染 (Sprite Renderer)", false, false, [&]() { m_SelectedEntity.AddComponent<SpriteTransformComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<CubeRendererComponent>())
				{
					addComponentSafe("3D 立方体渲染 (Cube Renderer)", false, false, [&]() { m_SelectedEntity.AddComponent<CubeRendererComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<PrimitiveRendererComponent>())
				{
					addComponentSafe("基础图元渲染 (Primitive Renderer)", false, false, [&]() { m_SelectedEntity.AddComponent<PrimitiveRendererComponent>(); });
				}
				
				if (!m_SelectedEntity.HasComponent<Rigidbody2DComponent>())
				{
					addComponentSafe("2D 刚体 (Rigidbody 2D)", true, false, [&]() { m_SelectedEntity.AddComponent<Rigidbody2DComponent>(); });
				}
				
				if (!m_SelectedEntity.HasComponent<BoxCollider2DComponent>())
				{
					addComponentSafe("2D 碰撞体 (Box Collider 2D)", true, false, [&]() { m_SelectedEntity.AddComponent<BoxCollider2DComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<TriangleCollider2DComponent>())
				{
					addComponentSafe("2D 三角形碰撞体 (Triangle Collider 2D)", true, false, [&]() { m_SelectedEntity.AddComponent<TriangleCollider2DComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<TriangleRendererComponent>())
				{
					addComponentSafe("2D 三角形渲染 (Triangle Renderer)", false, false, [&]() { m_SelectedEntity.AddComponent<TriangleRendererComponent>(); });
				}

				if (!m_SelectedEntity.HasComponent<Rigidbody3DComponent>())
				{
					addComponentSafe("3D 刚体 (Rigidbody 3D)", false, true, [&]() { m_SelectedEntity.AddComponent<Rigidbody3DComponent>(); });
				}
				
				if (!m_SelectedEntity.HasComponent<BoxCollider3DComponent>())
				{
					addComponentSafe("3D 碰撞体 (Box Collider 3D)", false, true, [&]() { m_SelectedEntity.AddComponent<BoxCollider3DComponent>(); });
				}

				ImGui::EndPopup();
			}

			if (showComponentError)
			{
				ImGui::OpenPopup("Component Compatibility Error");
			}

			bool openErrorModal = true;
			if (ImGui::BeginPopupModal("Component Compatibility Error", &openErrorModal, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Component Compatibility Error! (组件兼容性冲突)");
				ImGui::Separator();
				ImGui::Text("Cannot mix 2D and 3D physics components on the same entity.");
				ImGui::Text("无法在同一个实体上混用 2D 和 3D 物理组件。");
				ImGui::Separator();
				if (ImGui::Button("确定 (OK)", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
				ImGui::EndPopup();
			}

		}
		ImGui::End();

		// Settings Panel
		ImGui::Begin("设置 (Settings)");
		auto stats = Renderer2D::GetStatistics();
		ImGui::Text("Renderer2D 统计信息:");
		ImGui::Text("绘制调用 (Draw Calls): %d", stats.DrawCalls);
		ImGui::Text("四边形数量 (Quads): %d", stats.QuadCount);
		ImGui::Text("顶点数 (Vertices): %d", stats.GetTotalVertexCount());
		ImGui::Text("索引数 (Indices): %d", stats.GetTotalIndexCount());
		ImGui::Separator();
		
		ImGui::Text("相机设置 (Camera Settings)");
		if (ImGui::RadioButton("2D 相机", m_Is2DCamera))
		{
			m_Is2DCamera = true;
			m_SceneCamera.Set2DMode(8.0f);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("3D 相机", !m_Is2DCamera))
		{
			m_Is2DCamera = false;
			m_SceneCamera.Set3DMode(60.0f);
			m_SceneCamera.SetPitchForTopDown(-60.0f);
			m_SceneCamera.SetPosition({ 0.0f, 0.0f, 5.0f });
		}
		
		ImGui::Separator();
		ImGui::Text("描边设置 (Outline Settings)");
		ImGui::ColorEdit4("描边颜色 (Outline Color)", glm::value_ptr(m_OutlineColor));
		ImGui::SliderInt("描边宽度 (Outline Width)", &m_OutlineWidth, 1, 10);
		ImGui::SliderFloat("描边强度 (Outline Intensity)", &m_OutlineIntensity, 0.1f, 5.0f);

		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		// =====================================================================
		// SCENE VIEWPORT WINDOW - Always visible
		// =====================================================================
		{
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("场景视口 (Scene)"))
			{
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float sceneW = avail.x;
				float sceneH = avail.y;
				if (sceneW <= 0) sceneW = 800.0f;
				if (sceneH <= 0) sceneH = 600.0f;

				m_ViewportSize = { sceneW, sceneH };
				m_CachedSceneViewportSize = { sceneW, sceneH };

				// Cache and apply scene viewport size
				if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
				{
					m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
					m_SceneCamera.OnResize(m_ViewportSize.x, m_ViewportSize.y);
					m_SceneCamera.OnResize(m_ViewportSize.x, m_ViewportSize.y);
					m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				}

				// Update viewport bounds for picking and event blocking
				auto vpMinRegion = ImGui::GetWindowContentRegionMin();
				auto vpMaxRegion = ImGui::GetWindowContentRegionMax();
				auto vpOffset = ImGui::GetWindowPos();
				m_ViewportBounds[0] = { vpMinRegion.x + vpOffset.x, vpMinRegion.y + vpOffset.y };
				m_ViewportBounds[1] = { vpMaxRegion.x + vpOffset.x, vpMaxRegion.y + vpOffset.y };

				bool sceneFocused = ImGui::IsWindowFocused();
				bool sceneHovered = ImGui::IsWindowHovered();
				Application::Get().GetImGuiLayer()->BlockEvents(!sceneFocused || !sceneHovered);

				// === OPENGL RENDERING: Render scene content to m_Framebuffer using editor camera ===
				{
					// Guard against null scene or invalid sizes
					if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0 && m_ActiveScene)
					{
						m_Framebuffer->Bind();
						m_Framebuffer->ClearAttachment(1, -1);
						RenderCommand::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
						RenderCommand::Clear();
						RenderCommand::SetDepthTest(true);

						PerspectiveCamera& cam = m_SceneCamera.GetCamera();
						Renderer3D::BeginScene(cam);

						// Sky
						if (m_ShowSkybox)
						{
							if (!m_Is2DCamera)
								m_SkyRenderer.Render(cam.GetViewMatrix(), cam.GetProjectionMatrix(), cam.GetPosition());
							else
								m_SkyRenderer.RenderOrthographic(cam.GetPosition());
						}

						// Default cube
						if (m_CubeEntity)
						{
							auto& tc = m_CubeEntity.GetComponent<TransformComponent>();
							Renderer3D::DrawCube(tc.Transform, { 0.8f, 0.2f, 0.3f, 1.0f }, (int)(uint32_t)m_CubeEntity);
						}

						// All cubes
						auto cubeView = m_ActiveScene->GetRegistry().view<TransformComponent, CubeRendererComponent>();
						cubeView.each([](auto entityID, auto& tc, auto& cube)
						{
							Renderer3D::DrawCube(tc.Transform, cube.Color, (int)(uint32_t)entityID, cube.AmbientStrength, cube.DiffuseStrength, cube.SpecularStrength, cube.Shininess);
						});

						// Primitives
						auto primView = m_ActiveScene->GetRegistry().view<TransformComponent, PrimitiveRendererComponent>();
						primView.each([this](auto entityID, TransformComponent& tc, PrimitiveRendererComponent& prim)
						{
							if (m_Is2DCamera)
								Renderer3D::DrawCube(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
							else
							{
								if (prim.Type == PrimitiveRendererComponent::PrimitiveType::Capsule)
									Renderer3D::DrawCapsule(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
								else
									Renderer3D::DrawCube(tc.Transform, prim.Color, (int)(uint32_t)entityID, prim.AmbientStrength, prim.DiffuseStrength, prim.SpecularStrength, prim.Shininess);
							}
						});

						// 3D colliders
						if (m_ShowPhysicsColliders && !m_Is2DCamera)
						{
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
							glLineWidth(2.0f);
							auto bc3dView = m_ActiveScene->GetRegistry().view<TransformComponent, BoxCollider3DComponent>();
							for (auto entity : bc3dView)
							{
								auto [tc, bc3d] = bc3dView.get<TransformComponent, BoxCollider3DComponent>(entity);
								if (!bc3d.ShowCollider) continue;
								glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
								glm::vec3 position = tc.Transform[3];
								glm::vec3 offset = bc3d.Offset;
								glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
								rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
								rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
								rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
								glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), bc3d.HalfExtent * scale * 2.0f);
								Renderer3D::DrawCube(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
							}
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}

						// Model
						if (m_Model && m_ModelEntity)
						{
							auto& tc = m_ModelEntity.GetComponent<TransformComponent>();
							Renderer3D::DrawModel(tc.Transform, *m_Model, glm::vec4(1.0f), (int)(uint32_t)m_ModelEntity);
						}

						Renderer3D::EndScene();
						Renderer2D::BeginScene(cam);

						// Sprites
						auto spriteView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteTransformComponent>();
						for (auto entity : spriteView)
						{
							auto [transform, sprite] = spriteView.get<TransformComponent, SpriteTransformComponent>(entity);
							glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
							if (sprite.Texture)
								Renderer2D::DrawQuad(worldTransform, sprite.Texture, 1.0f, sprite.Color, (int)(uint32_t)entity);
							else
								Renderer2D::DrawQuad(worldTransform, sprite.Color, (int)(uint32_t)entity);
						}
						auto srView = m_ActiveScene->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
						for (auto entity : srView)
						{
							auto [transform, src] = srView.get<TransformComponent, SpriteRendererComponent>(entity);
							Ref<Texture2D> texToDraw = src.SubTexture ? src.SubTexture->GetTexture() : src.Texture;
							auto* animComp = m_ActiveScene->GetRegistry().try_get<SpriteAnimationComponent>(entity);
							if (animComp && animComp->AtlasBinding)
							{
								auto subTex = animComp->GetCurrentSubTexture();
								if (subTex) texToDraw = subTex->GetTexture();
							}
							glm::mat4 worldTransform = transform.GetWorldTransform(m_ActiveScene->GetRegistry());
							if (texToDraw)
								Renderer2D::DrawQuad(worldTransform, texToDraw, src.TilingFactor, src.Color, (int)(uint32_t)entity);
							else
								Renderer2D::DrawQuad(worldTransform, src.Color, (int)(uint32_t)entity);
						}

						// 2D colliders
						if (m_ShowPhysicsColliders && m_Is2DCamera)
						{
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
							glLineWidth(2.0f);
							auto bc2dView = m_ActiveScene->GetRegistry().view<TransformComponent, BoxCollider2DComponent>();
							for (auto entity : bc2dView)
							{
								auto [tc, bc2d] = bc2dView.get<TransformComponent, BoxCollider2DComponent>(entity);
								if (!bc2d.ShowCollider) continue;
								glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
								glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
								glm::vec3 offset = glm::vec3(bc2d.Offset.x, bc2d.Offset.y, 0.0f);
								glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
								rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
								rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
								rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
								glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(bc2d.Size.x * scale.x, bc2d.Size.y * scale.y, 1.0f));
								Renderer2D::DrawQuad(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
							}
							auto triView = m_ActiveScene->GetRegistry().view<TransformComponent, TriangleCollider2DComponent>();
							for (auto entity : triView)
							{
								auto [tc, tc2d] = triView.get<TransformComponent, TriangleCollider2DComponent>(entity);
								if (!tc2d.ShowCollider) continue;
								glm::vec3 scale( glm::length(glm::vec3(tc.Transform[0])), glm::length(glm::vec3(tc.Transform[1])), glm::length(glm::vec3(tc.Transform[2])) );
								glm::vec3 position = tc.Transform[3]; position.z += 0.01f;
								glm::vec3 offset = glm::vec3(tc2d.Offset.x, tc2d.Offset.y, 0.0f);
								glm::mat4 rotMat = tc.Transform; rotMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
								rotMat[0] = glm::vec4(glm::normalize(glm::vec3(rotMat[0])), 0.0f);
								rotMat[1] = glm::vec4(glm::normalize(glm::vec3(rotMat[1])), 0.0f);
								rotMat[2] = glm::vec4(glm::normalize(glm::vec3(rotMat[2])), 0.0f);
								glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotMat * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), glm::vec3(tc2d.Size.x * scale.x, tc2d.Size.y * scale.y, 1.0f));
								Renderer2D::DrawTriangle(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)(uint32_t)entity);
							}
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}

						Renderer2D::EndScene();
						m_Framebuffer->UnBind();
					}
				}

				// Outline rendering
				if (m_SelectedEntity && m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
				{
					bool hasSprite = m_SelectedEntity.HasComponent<SpriteTransformComponent>();
					bool hasTri = m_SelectedEntity.HasComponent<TriangleRendererComponent>();
					bool isCube = (m_SelectedEntity == m_CubeEntity);
					bool isModel = (m_SelectedEntity == m_ModelEntity);
					if (hasSprite || hasTri || isCube || isModel)
					{
						m_Framebuffer->Bind();
						RenderCommand::SetDepthTest(false);
						GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
						glDrawBuffers(1, drawBuffers);
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						m_OutlineShader->Bind();
						uint32_t ow = m_Framebuffer->GetSpecification().Width;
						uint32_t oh = m_Framebuffer->GetSpecification().Height;
						m_OutlineShader->SetFloat2("u_TexSize", glm::vec2(1.0f / ow, 1.0f / oh));
						m_OutlineShader->SetInt("u_SelectedEntity", (int)(uint32_t)m_SelectedEntity);
						m_OutlineShader->SetFloat4("u_OutlineColor", m_OutlineColor);
						m_OutlineShader->SetInt("u_OutlineWidth", m_OutlineWidth);
						m_OutlineShader->SetFloat("u_OutlineIntensity", m_OutlineIntensity);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, m_Framebuffer->GetColorAttachmentRendererID(1));
						m_OutlineShader->SetInt("u_EntityIDTexture", 1);
						glBindVertexArray(m_EmptyVAO);
						glDrawArrays(GL_TRIANGLES, 0, 3);
						glBindVertexArray(0);
						m_Framebuffer->UnBind();
						glDisable(GL_BLEND);
						RenderCommand::SetDepthTest(true);
					}
				}

				// Render the scene framebuffer to ImGui
				uint32_t texID = m_Framebuffer->GetColorAttachmentRendererID();
				ImGui::Image((void*)(intptr_t)texID, ImVec2{ sceneW, sceneH }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				// Draw label using draw list (on top of the image)
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				auto drawOffset = ImGui::GetWindowPos();
				drawList->AddRectFilled(ImVec2(drawOffset.x, drawOffset.y), ImVec2(drawOffset.x + sceneW, drawOffset.y + 26),
					IM_COL32(20, 20, 25, 180), 0.0f);
				drawList->AddText(ImVec2(drawOffset.x + 8, drawOffset.y + 6),
					IM_COL32(102, 179, 255, 255), "Scene View");

				// Mouse pick: read entity ID from framebuffer's entity texture
				if (ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver())
				{
					ImVec2 mousePos = ImGui::GetMousePos();
					int mx = (int)(mousePos.x - m_ViewportBounds[0].x);
					int my = (int)(mousePos.y - m_ViewportBounds[0].y);
					int entityID = m_Framebuffer->ReadPixel(1, mx, my);
					if (entityID > 0)
					{
						m_HoveredEntity = Entity((entt::entity)entityID, m_ActiveScene.get());
						m_SelectedEntity = m_HoveredEntity;
					}
					else
					{
						m_HoveredEntity = {};
						m_SelectedEntity = {};
					}
				}

				HandleViewportDragDrop(m_ViewportSize);
			}
			ImGui::End();
		}

		// =====================================================================
		// GAME VIEWPORT WINDOW - Only shown when GameOnly layout is active
		// =====================================================================
		if (m_ViewportLayout == ViewportLayout::GameOnly)
		{
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("游戏视口 (Game)"))
			{
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float gameW = avail.x;
				float gameH = avail.y;
				if (gameW <= 0) gameW = 800.0f;
				if (gameH <= 0) gameH = 600.0f;

				m_GameViewportSize = { gameW, gameH };
				m_CachedGameViewportSize = { gameW, gameH };

				// Cache and apply game viewport size
				if (m_GameViewportSize.x > 0 && m_GameViewportSize.y > 0)
				{
					m_GameFramebuffer->Resize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
					m_GameCamera.OnResize(m_GameViewportSize.x, m_GameViewportSize.y);
					m_GameCamera.OnResize(m_GameViewportSize.x, m_GameViewportSize.y);

					// Also resize the scene's game camera entity's internal camera
					Entity gameCam = m_ActiveScene->GetActiveGameCamera();
					if (gameCam && gameCam.HasComponent<CameraComponent>())
					{
						auto& camComp = gameCam.GetComponent<CameraComponent>();
						camComp._camera.SetViewportSize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
					}
				}

				// Update game viewport bounds
				auto vpMinRegion = ImGui::GetWindowContentRegionMin();
				auto vpMaxRegion = ImGui::GetWindowContentRegionMax();
				auto vpOffset = ImGui::GetWindowPos();
				m_GameViewportBounds[0] = { vpMinRegion.x + vpOffset.x, vpMinRegion.y + vpOffset.y };
				m_GameViewportBounds[1] = { vpMaxRegion.x + vpOffset.x, vpMaxRegion.y + vpOffset.y };

				// === OPENGL RENDERING: Render scene content to m_GameFramebuffer using game camera entity ===
				{
					// Check if game viewport should render at all
					if (m_GameViewportSize.x > 0 && m_GameViewportSize.y > 0 && m_ActiveScene)
					{
						Entity gameCam = m_ActiveScene->GetActiveGameCamera();
						if (gameCam && gameCam.HasComponent<CameraComponent>() && gameCam.HasComponent<TransformComponent>())
						{
							auto& camComp = gameCam.GetComponent<CameraComponent>();
							auto& tc = gameCam.GetComponent<TransformComponent>();

							// Sync camera position from transform to camera
							camComp._camera.SetPosition(tc.Transform[3]);

							// Extract euler angles from rotation matrix
							glm::mat3 rot(tc.Transform);
							rot[0] = glm::normalize(rot[0]);
							rot[1] = glm::normalize(rot[1]);
							rot[2] = glm::normalize(rot[2]);
							float yaw = glm::degrees(glm::atan(rot[0][2], rot[2][2]));
							float pitch = glm::degrees(glm::asin(-rot[1][2]));
							float roll = glm::degrees(glm::atan(rot[1][0], rot[1][1]));
							camComp._camera.SetRotation(glm::vec3(pitch, yaw, roll));

							RenderSceneToFramebuffer(m_GameFramebuffer, camComp._camera, m_GameViewportSize, true);
						}
						else
						{
							// Fallback: use game camera controller
							RenderSceneToFramebuffer(m_GameFramebuffer, m_GameCamera.GetCamera(), m_GameViewportSize, true);
						}
					}
				}

				// Render the game framebuffer
				uint32_t gameTexID = m_GameFramebuffer->GetColorAttachmentRendererID();
				ImGui::Image((void*)(intptr_t)gameTexID, ImVec2{ gameW, gameH }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				// Draw label
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				auto drawOffset = ImGui::GetWindowPos();
				drawList->AddRectFilled(ImVec2(drawOffset.x, drawOffset.y), ImVec2(drawOffset.x + gameW, drawOffset.y + 26),
					IM_COL32(20, 20, 25, 180), 0.0f);
				drawList->AddText(ImVec2(drawOffset.x + 8, drawOffset.y + 6),
					IM_COL32(255, 179, 77, 255), "Game View");
			}
			ImGui::End();
		}

		ImGui::PopStyleVar();

		// ---- Post-Viewport Render ----

		// TileMap brush painting - scene view only
		HandleViewportTileMapBrush(m_ViewportSize);

		if (m_ShowQuickAddPanel)
		{
			const float quickAddScale = ImGui::GetMainViewport() ? ImGui::GetMainViewport()->DpiScale : 1.0f;
			ImGui::SetNextWindowSize(ImVec2(320.0f * quickAddScale, 420.0f * quickAddScale), ImGuiCond_FirstUseEver);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * quickAddScale, 12.0f * quickAddScale));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * quickAddScale, 7.0f * quickAddScale));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * quickAddScale, 8.0f * quickAddScale));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * quickAddScale);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 0.98f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.13f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.26f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.27f, 0.30f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.31f, 0.35f, 0.40f, 1.0f));
			if (ImGui::Begin("快速添加实体 (Quick Add)", &m_ShowQuickAddPanel))
			{
				ImGui::TextUnformatted("快速添加 (Quick Add)");
				ImGui::Separator();
				ImGui::InputTextWithHint("##Search", "搜索预设体...", m_QuickAddSearchBuffer, sizeof(m_QuickAddSearchBuffer));
				ImGui::Separator();

				std::string searchStr = m_QuickAddSearchBuffer;
				std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

				auto addPrefab = [&](const std::string& name, auto instantiateFunc) {
					std::string lowerName = name;
					std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
					if (searchStr.empty() || lowerName.find(searchStr) != std::string::npos)
					{
						if (ImGui::Button(name.c_str(), ImVec2(-1.0f, 36.0f * quickAddScale)))
						{
							instantiateFunc();
						}
					}
				};

				ImGui::BeginChild("PrefabList");
				
				addPrefab("物理立方体 (Physics Cube)", [&]() {
					Entity entity = m_ActiveScene->CreateEntity("Physics Cube");
					auto& tc = entity.GetComponent<TransformComponent>();
					// If hovered on viewport and not over gizmo, place at mouse, else origin
					if (m_HoveredEntity)
					{
						auto& hitTc = m_HoveredEntity.GetComponent<TransformComponent>();
						// very rough approximation: place on top of hovered entity
						tc.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(hitTc.Transform[3]) + glm::vec3(0.0f, 1.0f, 0.0f));
					}
					entity.AddComponent<CubeRendererComponent>(glm::vec4(0.8f, 0.3f, 0.2f, 1.0f));
					auto& rb = entity.AddComponent<Rigidbody3DComponent>();
					rb.Type = Rigidbody3DComponent::BodyType::Dynamic;
					rb.Mass = 1.0f;
					auto& bc = entity.AddComponent<BoxCollider3DComponent>();
					bc.HalfExtent = glm::vec3(0.5f);
					m_SelectedEntity = entity;
				});

				addPrefab("物理球体 (Physics Sphere)", [&]() {
					Entity entity = m_ActiveScene->CreateEntity("Physics Sphere");
					auto& tc = entity.GetComponent<TransformComponent>();
					if (m_HoveredEntity)
					{
						auto& hitTc = m_HoveredEntity.GetComponent<TransformComponent>();
						tc.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(hitTc.Transform[3]) + glm::vec3(0.0f, 1.0f, 0.0f));
					}
					entity.AddComponent<CubeRendererComponent>(glm::vec4(0.2f, 0.3f, 0.8f, 1.0f));
					auto& rb = entity.AddComponent<Rigidbody3DComponent>();
					rb.Type = Rigidbody3DComponent::BodyType::Dynamic;
					rb.Mass = 1.0f;
					auto& bc = entity.AddComponent<BoxCollider3DComponent>();
					bc.HalfExtent = glm::vec3(0.5f); // Using box collider as proxy for now
					m_SelectedEntity = entity;
				});

				addPrefab("物理演示平面 (Physics Demo Plane)", [&]() {
					Entity plane = m_ActiveScene->CreateEntity("PhysicsDemoPlane");
					auto& tc = plane.GetComponent<TransformComponent>();
					tc.Transform = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 0.1f, 10.0f));
					plane.AddComponent<CubeRendererComponent>(glm::vec4(0.5f, 0.8f, 0.5f, 1.0f));
					auto& rb3d = plane.AddComponent<Rigidbody3DComponent>();
					rb3d.Type = Rigidbody3DComponent::BodyType::Kinematic;
					rb3d.Mass = 1.0f;
					auto& bc3d = plane.AddComponent<BoxCollider3DComponent>();
					bc3d.HalfExtent = glm::vec3(5.0f, 0.05f, 5.0f);
					m_SelectedEntity = plane;
				});

				addPrefab("2D 物理方块 (2D Physics Box)", [&]() {
					Entity entity = m_ActiveScene->CreateEntity("2D Physics Box");
					entity.AddComponent<SpriteTransformComponent>(glm::vec4{ 0.2f,0.8f,0.2f,1.0f });
					auto& rb2d = entity.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = Rigidbody2DComponent::BodyType::Dynamic;
					entity.AddComponent<BoxCollider2DComponent>();
					m_SelectedEntity = entity;
				});

				ImGui::EndChild();
			}
			ImGui::End();
			ImGui::PopStyleColor(5);
			ImGui::PopStyleVar(4);
		}

		// Gizmos
		Entity selectedEntity = m_SelectedEntity;
		if (selectedEntity && m_GizmoType != -1)
		{
			ImGuizmo::SetOrthographic(!m_Is2DCamera);
			ImGuizmo::SetDrawlist();
			
			// Fix Gizmo interactivity: Use viewport bounds instead of window width/height, 
			// because the ImGui window includes title bar and tabs, which offsets the mouse calculation!
			float windowWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
			float windowHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
			ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, windowWidth, windowHeight);

			// Camera
			glm::mat4 cameraProjection = m_Is2DCamera ? m_SceneCamera.GetCamera().GetProjectionMatrix() : m_SceneCamera.GetCamera().GetProjectionMatrix();
			glm::mat4 cameraView = m_Is2DCamera ? m_SceneCamera.GetCamera().GetViewMatrix() : m_SceneCamera.GetCamera().GetViewMatrix();

			// [Debug] Store matrices before manipulation to verify if perspective distortion occurs
			glm::mat4 preManipProj = cameraProjection;
			glm::mat4 preManipView = cameraView;

			// Entity transform
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.Transform;

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 0.5f; // Snap to 0.5m for translation/scale
			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
				snapValue = 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			// 提取物体的缩放并创建一个单位缩放的矩阵用于 Translate/Rotate 的绘制和拾取，消除缩放导致的箭头偏移
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), matrixTranslation, matrixRotation, matrixScale);
			
			glm::mat4 gizmoTransform = transform;
			if (m_GizmoType != ImGuizmo::OPERATION::SCALE)
			{
				float unitScale[3] = { 1.0f, 1.0f, 1.0f };
				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, unitScale, glm::value_ptr(gizmoTransform));
			}

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(gizmoTransform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				// Debug output to verify if the camera matrices were accidentally modified by ImGuizmo
				glm::mat4 postManipProj = m_Is2DCamera ? m_SceneCamera.GetCamera().GetProjectionMatrix() : m_SceneCamera.GetCamera().GetProjectionMatrix();
				glm::mat4 postManipView = m_Is2DCamera ? m_SceneCamera.GetCamera().GetViewMatrix() : m_SceneCamera.GetCamera().GetViewMatrix();

				if (preManipProj != postManipProj || preManipView != postManipView)
				{
					QL_CORE_ERROR("Camera perspective/view distorted after object transform!");
				}

				if (m_GizmoType == ImGuizmo::OPERATION::SCALE)
				{
					tc.Transform = gizmoTransform;
				}
				else
				{
					float newTranslation[3], newRotation[3], newScale[3];
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(gizmoTransform), newTranslation, newRotation, newScale);
					ImGuizmo::RecomposeMatrixFromComponents(newTranslation, newRotation, matrixScale, glm::value_ptr(tc.Transform));
				}
			}
		}

		// Content Browser Panel
		if (m_IsContentBrowserOpen)
		{
			ImGui::Begin("内容浏览器 (Content Browser)", &m_IsContentBrowserOpen);
			if (m_CurrentDirectory != "assets")
			{
				if (ImGui::Button("<- 返回 (Back)"))
				{
					m_CurrentDirectory = std::filesystem::path(m_CurrentDirectory).parent_path().string();
				}
			}

			static float padding = 16.0f;
			static float thumbnailSize = 128.0f;
			float cellSize = thumbnailSize + padding;

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = (int)(panelWidth / cellSize);
			if (columnCount < 1)
				columnCount = 1;

			ImGui::Columns(columnCount, 0, false);

			for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				const auto& path = directoryEntry.path();
				auto relativePath = std::filesystem::relative(path, "assets");
				std::string filenameString = relativePath.filename().string();
				std::string extension = path.extension().string();

				Ref<Texture2D> icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;

				if (!directoryEntry.is_directory())
				{
					if (extension == ".uasset") icon = m_IconUASSET;
					else if (extension == ".umap") icon = m_IconUMAP;
					else if (extension == ".fbx" || extension == ".obj" || extension == ".gltf") icon = m_IconFBX;
					else if (extension == ".wav" || extension == ".ogg" || extension == ".mp3") icon = m_IconWAV;
					else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") icon = m_IconPNG;
				}

				ImVec4 tintColor = ImVec4(1, 1, 1, 1);
				ImGui::PushID(filenameString.c_str());
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				if (ImGui::ImageButton((ImTextureID)(intptr_t)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 }, -1, ImVec4(0,0,0,0), tintColor))
				{
					if (directoryEntry.is_directory())
					{
						m_CurrentDirectory = (std::filesystem::path(m_CurrentDirectory) / path.filename()).string();
					}
				}

				// Right-click context menu for files and directories
				if (ImGui::BeginPopupContextItem("AssetContextMenu"))
				{
					if (!directoryEntry.is_directory())
					{
						if (ImGui::MenuItem("重命名 (Rename)"))
						{
							m_RenamingAsset = path.string();
							memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
							strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer) - 1, filenameString.c_str(), _TRUNCATE);
						}
					}

					if (ImGui::MenuItem("在资源管理器中显示 (Show in Explorer)"))
					{
						std::string absPath = std::filesystem::absolute(path).string();
						ShellExecuteA(NULL, "open", "explorer.exe", ("/select,\"" + absPath + "\"").c_str(), NULL, SW_SHOWNORMAL);
					}

					if (!directoryEntry.is_directory())
					{
						ImGui::Separator();
						if (ImGui::MenuItem("删除 (Delete)", NULL, false, true))
						{
							m_DeletingAsset = path.string();
							ImGui::OpenPopup("ConfirmDelete");
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopStyleColor();
				ImGui::PopID();

				ImGui::TextWrapped("%s", filenameString.c_str());
				ImGui::NextColumn();
			}

			// Rename input overlay
			if (!m_RenamingAsset.empty())
			{
				std::filesystem::path oldPath(m_RenamingAsset);
				std::string oldName = oldPath.filename().string();
				ImGui::SetNextWindowSize(ImVec2(350, 100), ImGuiCond_Always);
				if (ImGui::Begin("重命名资产", &m_IsRenamePopupOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
				{
					ImGui::Text("重命名: %s", oldName.c_str());
					if (ImGui::InputText("新名称", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (strlen(m_RenameBuffer) > 0)
						{
							std::filesystem::path newPath = oldPath.parent_path() / m_RenameBuffer;
							if (!std::filesystem::exists(newPath))
							{
								std::filesystem::rename(oldPath, newPath);
								QL_CORE_INFO("Renamed: %s -> %s", oldPath.string().c_str(), newPath.string().c_str());
							}
							else
							{
								QL_CORE_WARN("File already exists: %s", newPath.string().c_str());
							}
						}
						m_RenamingAsset.clear();
						m_IsRenamePopupOpen = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("确定"))
					{
						if (strlen(m_RenameBuffer) > 0)
						{
							std::filesystem::path newPath = oldPath.parent_path() / m_RenameBuffer;
							if (!std::filesystem::exists(newPath))
							{
								std::filesystem::rename(oldPath, newPath);
								QL_CORE_INFO("Renamed: %s -> %s", oldPath.string().c_str(), newPath.string().c_str());
							}
						}
						m_RenamingAsset.clear();
						m_IsRenamePopupOpen = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("取消"))
					{
						m_RenamingAsset.clear();
						m_IsRenamePopupOpen = false;
					}
					ImGui::End();
				}
			}

			// Delete confirmation dialog
			if (!m_DeletingAsset.empty())
			{
				ImGui::SetNextWindowSize(ImVec2(350, 120), ImGuiCond_Always);
				if (ImGui::Begin("确认删除", &m_IsDeletePopupOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
				{
					std::string fn = std::filesystem::path(m_DeletingAsset).filename().string();
					ImGui::Text("确定要删除以下文件吗？此操作不可撤销。");
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  %s", fn.c_str());
					ImGui::Spacing();
					if (ImGui::Button("删除"))
					{
						if (std::filesystem::remove(m_DeletingAsset))
							QL_CORE_INFO("Deleted: %s", m_DeletingAsset.c_str());
						else
							QL_CORE_WARN("Failed to delete: %s", m_DeletingAsset.c_str());
						m_DeletingAsset.clear();
						m_IsDeletePopupOpen = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("取消"))
					{
						m_DeletingAsset.clear();
						m_IsDeletePopupOpen = false;
					}
					ImGui::End();
				}
			}

			ImGui::Columns(1);
			ImGui::End();
		}

		// TileMap Editor Panel
		if (m_ShowTileMapEditor)
		{
			m_TileMapEditor.SetTileMap(&m_EditorTileMap);
			m_TileMapEditor.OnImGuiRender();
		}

		// UI Hierarchy Panel
		if (m_ShowUIHierarchy)
		{
			m_UIHierarchyPanel.OnImGuiRender();
		}

		// UI Canvas Editor
		if (m_ShowUICanvasEditor)
		{
			m_UICanvasEditor.OnImGuiRender();
		}

		// Material Editor
		if (m_ShowMaterialEditor)
		{
			m_MaterialEditor.OnImGuiRender();
		}

		// Console Panel
		if (m_ShowConsole)
		{
			m_ConsolePanel.OnImGuiRender();
		}

		// Profiler Panel
		if (m_ShowProfiler)
		{
			m_ProfilerOverlay.CollectMetrics();
			m_ProfilerOverlay.OnImGuiRender();
		}

		// Asset Browser Panel (new)
		if (m_ShowAssetBrowser)
		{
			m_AssetBrowserPanel.OnImGuiRender();
		}

		// Animation Editor Panel
		if (m_ShowAnimationEditor)
		{
			m_AnimationEditorPanel.SetScene(m_ActiveScene.get());
			m_AnimationEditorPanel.OnImGuiRender();
		}

		// Atlas Builder Panel
		if (m_ShowAtlasBuilder)
		{
			m_AtlasBuilderPanel.SetScene(m_ActiveScene.get());
			m_AtlasBuilderPanel.OnImGuiRender();
		}

		// NavMesh Editor Panel
		m_NavMeshEditorPanel.OnImGuiRender();

		// Animator Editor Panel
		m_AnimatorEditorPanel.OnImGuiRender();

		ImGui::End(); // Viewport

	}

	// ---- Helper Methods ----

	void EditorLayer::HandleViewportDragDrop(const glm::vec2& vpSize)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path fullPath = std::filesystem::path("assets") / path;

				if (fullPath.extension() == ".obj" || fullPath.extension() == ".fbx")
				{
					QL_CORE_INFO("Loading Model: {0}", fullPath.string());
					Entity newEntity = m_ActiveScene->CreateEntity(fullPath.filename().string());
					m_ModelEntity = newEntity;
					m_Model = CreateRef<Model>(fullPath.string());
					m_SelectedEntity = newEntity;
				}
				else if (fullPath.extension() == ".png" || fullPath.extension() == ".jpg")
				{
					QL_CORE_INFO("Loading Texture: {0}", fullPath.string());
					Entity newEntity = m_ActiveScene->CreateEntity(fullPath.filename().string());
					newEntity.AddComponent<SpriteTransformComponent>();
					m_SelectedEntity = newEntity;
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void EditorLayer::HandleViewportTileMapBrush(const glm::vec2& vpSize)
	{
		if (!m_ShowTileMapEditor || !ImGui::IsWindowHovered() || ImGuizmo::IsOver() || ImGui::IsAnyItemActive())
			return;

		bool clicked = ImGui::IsMouseClicked(0);
		bool released = ImGui::IsMouseReleased(0);
		bool dragging = ImGui::IsMouseDown(0);

		if (released)
		{
			m_TileMapEditor.HandleBrushStrokeContinuousReset();
			return;
		}

		if (!clicked && !dragging)
			return;

		auto [mx, my] = Input::GetMousePosition();
		float vpW = vpSize.x;
		float vpH = vpSize.y;
		if (vpW <= 0 || vpH <= 0)
			return;

		glm::vec2 ndc = (glm::vec2(mx, my) / glm::vec2(vpW, vpH)) * 2.0f - 1.0f;
		ndc.y = -ndc.y;
		glm::mat4 invVP = glm::inverse(m_SceneCamera.GetCamera().GetViewProjectionMatrix());
		glm::vec4 world4 = invVP * glm::vec4(ndc, 0.0f, 1.0f);
		glm::vec3 worldPos = glm::vec3(world4) / world4.w;
		float tileSize = m_EditorTileMap.GetTileSize();
		glm::ivec2 gridPos(
			static_cast<int>(std::floor(worldPos.x / tileSize)),
			static_cast<int>(std::floor(worldPos.y / tileSize)));
		if (clicked)
			m_TileMapEditor.HandleBrushStroke(gridPos);
		else
			m_TileMapEditor.HandleBrushStrokeContinuous(gridPos);
	}

}
