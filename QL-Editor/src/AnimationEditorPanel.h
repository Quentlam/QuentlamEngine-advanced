#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Renderer/Texture.h"
#include <string>
#include <vector>

namespace Quentlam
{
	class QUENTLAM_API AnimationEditorPanel
	{
	public:
		AnimationEditorPanel() = default;

		void OnImGuiRender();
		void SetScene(class Scene* scene) { m_Scene = scene; }

		bool IsOpen() const { return m_IsOpen; }
		void SetOpen(bool open) { m_IsOpen = open; }

	private:
		void DrawAssetSelector();
		void DrawAnimationPreview();
		void DrawClipEditor();
		void DrawFrameList();
		void LoadAnimationData(const std::string& filepath);

		class Scene* m_Scene = nullptr;
		bool m_IsOpen = true;

		std::string m_AnimationDataPath;
		char m_AnimDataPathBuf[256] = { 0 };

		Ref<Texture2D> m_PreviewTexture;
		Ref<SpriteAtlasBinding> m_PreviewAtlas;
		std::vector<Ref<AnimationClip>> m_LoadedClips;
		Ref<AnimationClip> m_SelectedClip;
		int32_t m_SelectedFrameIndex = -1;

		char m_NewClipName[64] = { 0 };
		float m_NewClipFPS = 12.0f;
		int m_NewClipWrapMode = 1;
		float m_PlaybackTime = 0.0f;
		bool m_IsPreviewPlaying = false;
	};
}
