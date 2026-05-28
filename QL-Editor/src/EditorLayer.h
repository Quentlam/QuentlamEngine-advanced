#pragma once

#include "Quentlam/Core/Layer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Quentlam/Renderer/PerspectiveCamera.h"
#include "Quentlam/Renderer/PerspectiveCameraController.h"
#include "Quentlam/Renderer/Model.h"
#include "Quentlam/Scene/Scene.h"
#include "Quentlam/Editor/TileMapEditorPanel.h"
#include "Quentlam/Editor/UIHierarchyPanel.h"
#include "Quentlam/Editor/UICanvasEditor.h"
#include "Quentlam/Editor/MaterialEditorPanel.h"
#include "Quentlam/Editor/EditorCommand.h"


namespace Quentlam
{

	enum class SceneState
	{
		Edit = 0, Play = 1, Pause = 2
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer() = default;
		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& event) override;
		void OnUpdate(Timestep ts) override;
		void OnImGuiLayer() override;



		void OnOverlayRender();
		void UI_Toolbar();

	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		
		void OnScenePlay();
		void OnSceneStop();
		void OnScenePause();
		void ResumeScenePlay();

		OrthographicCameraController  m_CameraController;
		PerspectiveCameraController   m_PerspCameraController;
		bool m_Is3DCamera = false;

		//Temp
		Ref<Texture2D>		m_Texture2D;
		Ref<Texture2D>		m_CheckerboardTexture;

		Ref<FrameBuffer>	m_Framebuffer;
		glm::vec2 m_ViewportSize{ 0.0f,0.0f };

		bool m_ViewportFocused = false, m_ViewportHovered = false;

		Ref<VertexArray>	m_VertexArray;
		Ref<Shader>         m_FlatColorShader;

		Entity m_SquareEntity;
		Entity m_CubeEntity;
		Entity m_ModelEntity;


		Ref<Scene>	m_ActiveScene;

		glm::vec4 m_Square_Color{ 0.3f, 0.3f, 0.8f,1.0f };

		std::vector<Entity> m_SelectedEntities;
		Entity m_SelectedEntity; // primary selection (last clicked)
		Entity m_HoveredEntity;
		glm::vec2 m_ViewportBounds[2];

		int m_GizmoType = -1; // -1 = None, 0 = Translate, 1 = Rotate, 2 = Scale

		bool m_IsContentBrowserOpen = true;
		std::string m_CurrentDirectory = "assets";
		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_IconFBX;
		Ref<Texture2D> m_IconPNG;
		Ref<Texture2D> m_IconWAV;
		Ref<Texture2D> m_IconUASSET;
		Ref<Texture2D> m_IconUMAP;

		Ref<Model> m_Model;

		Ref<Shader> m_OutlineShader;
		uint32_t m_EmptyVAO = 0;

		SceneState m_SceneState = SceneState::Edit;
		Ref<Texture2D> m_IconPlay;
		Ref<Texture2D> m_IconPause;
		Ref<Texture2D> m_IconStop;
		Ref<Texture2D> m_IconAdd;
		bool m_bIsPaused = false;
		bool m_IsSceneTransitioning = false;
		bool m_ShowQuickAddPanel = false;
		char m_QuickAddSearchBuffer[128] = "";

		bool m_ShowSaveNotification = false;
		float m_SaveNotificationTimer = 0.0f;

		SceneState m_LastToolbarVisualState = SceneState::Edit;
		float m_ToolbarTransitionProgress = 1.0f;
		std::string m_LastPlayFailure;

		// Editor outline properties
		glm::vec4 m_OutlineColor = { 1.0f, 0.5f, 0.0f, 1.0f };
		int m_OutlineWidth = 3;
		float m_OutlineIntensity = 1.0f;

		bool m_ShowPhysicsColliders = true;

		// Scene save/load
		std::string m_ScenePath;
		bool m_NeedsAutosave = false;

		// TileMap editor
		bool m_ShowTileMapEditor = false;
		TileMapEditorPanel m_TileMapEditor;
		TileMap m_EditorTileMap;

		// UI Hierarchy Panel
		bool m_ShowUIHierarchy = true;
		UIHierarchyPanel m_UIHierarchyPanel;

		// UI Canvas Editor
		bool m_ShowUICanvasEditor = false;
		UICanvasEditor m_UICanvasEditor;

		// Material Editor
		bool m_ShowMaterialEditor = false;
		MaterialEditorPanel m_MaterialEditor;

		// Command Stack (Undo/Redo)
		EditorCommandStack m_CommandStack;
		std::shared_ptr<ICommand> m_LastCommand;
		entt::entity m_CopiedEntity = entt::null;

		// Asset pipeline
		bool m_ShowAssetBrowser = false;
		std::string m_RenamingAsset;
		std::string m_DeletingAsset;
		bool m_IsRenamePopupOpen = true;
		bool m_IsDeletePopupOpen = true;
		char m_RenameBuffer[128] = { 0 };
	};

}
