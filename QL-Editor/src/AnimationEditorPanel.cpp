#include "AnimationEditorPanel.h"
#include "Quentlam/Gameplay/AnimationLoader.h"
#include "Quentlam/Resource/ResourceManager.h"
#include "imgui/imgui.h"
#include <algorithm>

namespace Quentlam
{

static const char* WrapModeNames[] = { "Once", "Loop", "PingPong", "ClampForever" };
static const char* DirectionNames[] = { "Down", "Left", "Right", "Up", "DownLeft", "DownRight", "UpLeft", "UpRight" };

void AnimationEditorPanel::LoadAnimationData(const std::string& filepath)
{
	m_AnimationDataPath = filepath;
	Ref<Texture2D> tex;
	Ref<SpriteAtlasBinding> atlas;
	if (AnimationLoader::LoadAnimationData(filepath, tex, atlas, m_LoadedClips))
	{
		m_PreviewTexture = tex;
		m_PreviewAtlas = atlas;
		m_SelectedClip = nullptr;
		if (!m_LoadedClips.empty())
			m_SelectedClip = m_LoadedClips[0];
		QL_CORE_INFO("AnimationEditor: Loaded {0} clips from {1}", m_LoadedClips.size(), filepath);
	}
	else
	{
		m_PreviewTexture = nullptr;
		m_PreviewAtlas = nullptr;
		m_LoadedClips.clear();
		QL_CORE_ERROR("AnimationEditor: Failed to load animation data from {0}", filepath);
	}
}

void AnimationEditorPanel::OnImGuiRender()
{
	if (!m_IsOpen) return;

	ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Animation Editor", &m_IsOpen,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Animation JSON..."))
			{
				ImGui::OpenPopup("OpenAnimationFile");
			}
			if (ImGui::MenuItem("Save"))
			{
				if (m_SelectedClip && !m_AnimationDataPath.empty())
				{
					QL_CORE_INFO("AnimationEditor: Save requested for {0}", m_AnimationDataPath);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Columns(2, "anim_editor", true);

	ImGui::SetColumnWidth(0, 250.0f);
	DrawAssetSelector();
	ImGui::NextColumn();
	DrawAnimationPreview();
	DrawClipEditor();

	ImGui::Columns(1);

	if (ImGui::BeginPopup("OpenAnimationFile"))
	{
		ImGui::Text("Enter animation JSON path:");
		ImGui::InputText("##AnimPath", m_AnimDataPathBuf, sizeof(m_AnimDataPathBuf));
		if (ImGui::Button("Load"))
		{
			if (m_AnimDataPathBuf[0] != '\0')
			{
				LoadAnimationData(m_AnimDataPathBuf);
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::End();
}

void AnimationEditorPanel::DrawAssetSelector()
{
	ImGui::Text("Animation Clips");
	ImGui::Separator();

	if (ImGui::Button("Load JSON", ImVec2(-1.0f, 0.0f)))
	{
		ImGui::OpenPopup("OpenAnimationFile");
	}

	if (!m_AnimationDataPath.empty())
	{
		ImGui::Text("File: %s", m_AnimationDataPath.c_str());
		ImGui::Text("%zu clips loaded", m_LoadedClips.size());
	}

	ImGui::Separator();

	if (ImGui::BeginChild("ClipList", ImVec2(0, 200.0f)))
	{
		for (size_t i = 0; i < m_LoadedClips.size(); ++i)
		{
			auto& clip = m_LoadedClips[i];
			bool isSelected = (m_SelectedClip == clip);
			std::string label = clip->GetName() + " (" + std::to_string(clip->GetFrameCount()) + " frames)";

			if (ImGui::Selectable(label.c_str(), isSelected))
			{
				m_SelectedClip = clip;
				m_SelectedFrameIndex = -1;
			}
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Text("Create New Clip");

	ImGui::InputText("Clip Name", m_NewClipName, sizeof(m_NewClipName));
	ImGui::SliderFloat("FPS", &m_NewClipFPS, 1.0f, 60.0f, "%.1f");
	ImGui::Combo("Wrap Mode", &m_NewClipWrapMode, WrapModeNames, 4);

	if (ImGui::Button("Create Clip") && m_NewClipName[0] != '\0' && m_PreviewAtlas)
	{
		auto newClip = CreateRef<AnimationClip>(
			m_NewClipName, m_PreviewAtlas, m_NewClipFPS);
		newClip->SetWrapMode(static_cast<EAnimationWrapMode>(m_NewClipWrapMode));
		m_LoadedClips.push_back(newClip);
		m_SelectedClip = newClip;
		m_NewClipName[0] = '\0';
	}
}

void AnimationEditorPanel::DrawAnimationPreview()
{
	ImGui::Text("Preview");
	ImGui::Separator();

	if (m_PreviewTexture && m_SelectedClip)
	{
		int32_t frameIdx = m_SelectedClip->GetFrameCount() > 0 ? 0 : -1;
		if (frameIdx >= 0)
		{
			const auto* frameData = m_SelectedClip->GetFrame(frameIdx);
			if (frameData)
			{
				int32_t spriteIdx = frameData->SpriteIndex;
				int32_t col = spriteIdx % m_PreviewAtlas->AtlasColumns;
				int32_t row = spriteIdx / m_PreviewAtlas->AtlasColumns;
				float uvSizeX = 1.0f / m_PreviewAtlas->AtlasColumns;
				float uvSizeY = 1.0f / m_PreviewAtlas->AtlasRows;

				ImVec2 uv0(col * uvSizeX, row * uvSizeY);
				ImVec2 uv1(uv0.x + uvSizeX, uv0.y + uvSizeY);

				ImGui::Image(
					reinterpret_cast<void*>(static_cast<intptr_t>(m_PreviewTexture->GetRendererID())),
					ImVec2(128.0f, 128.0f),
					uv0, uv1
				);
			}
		}

		ImGui::Text("Clip: %s", m_SelectedClip->GetName().c_str());
		ImGui::Text("Frames: %d", m_SelectedClip->GetFrameCount());
		ImGui::Text("FPS: %.1f", m_SelectedClip->GetFPS());
		ImGui::Text("Duration: %.2fs", m_SelectedClip->GetDuration());

		const char* wm = WrapModeNames[static_cast<int>(m_SelectedClip->GetWrapMode())];
		ImGui::Text("Wrap: %s", wm);

		ImGui::Separator();

		if (ImGui::Button(m_IsPreviewPlaying ? "Stop Preview" : "Play Preview"))
		{
			m_IsPreviewPlaying = !m_IsPreviewPlaying;
			if (m_IsPreviewPlaying)
				m_PlaybackTime = 0.0f;
		}

		if (m_IsPreviewPlaying)
		{
			float dt = ImGui::GetIO().DeltaTime;
			m_PlaybackTime += dt;
			float dur = m_SelectedClip->GetDuration();
			if (dur > 0.0f)
				m_PlaybackTime = fmodf(m_PlaybackTime, dur);

			int32_t curFrame = m_SelectedClip->GetFrameAtTime(m_PlaybackTime);
			ImGui::Text("Playing frame %d", curFrame);
			ImGui::ProgressBar(m_PlaybackTime / dur, ImVec2(-1, 0));
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f),
			"Load a JSON to preview animation");
		ImGui::Text("Use Load JSON button on the left");
	}
}

void AnimationEditorPanel::DrawClipEditor()
{
	if (!m_SelectedClip) return;

	ImGui::Separator();
	ImGui::Text("Clip Editor: %s", m_SelectedClip->GetName().c_str());

	float fps = m_SelectedClip->GetFPS();
	if (ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f))
		m_SelectedClip->SetFPS(fps);

	int wrapModeIdx = static_cast<int>(m_SelectedClip->GetWrapMode());
	if (ImGui::Combo("Wrap Mode", &wrapModeIdx, WrapModeNames, 4))
		m_SelectedClip->SetWrapMode(static_cast<EAnimationWrapMode>(wrapModeIdx));

	DrawFrameList();

	ImGui::Separator();
	ImGui::Text("Add Frame");
	static int newSpriteIdx = 0;
	static float newFrameDuration = 0.1f;
	ImGui::InputInt("Sprite Index", &newSpriteIdx);
	ImGui::SliderFloat("Duration", &newFrameDuration, 0.01f, 1.0f);

	if (ImGui::Button("Add Frame"))
	{
		AnimationFrame frame;
		frame.SpriteIndex = newSpriteIdx;
		frame.Duration = newFrameDuration;
		m_SelectedClip->AddFrame(frame);
	}

	if (m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < m_SelectedClip->GetFrameCount())
	{
		ImGui::Separator();
		ImGui::Text("Frame %d", m_SelectedFrameIndex);
		if (ImGui::Button("Delete Frame"))
		{
			m_SelectedClip->RemoveFrame(m_SelectedFrameIndex);
			m_SelectedFrameIndex = -1;
		}
	}
}

void AnimationEditorPanel::DrawFrameList()
{
	if (!ImGui::BeginChild("FrameList", ImVec2(0, 150.0f)))
	{
		ImGui::EndChild();
		return;
	}

	int32_t count = m_SelectedClip->GetFrameCount();
	for (int32_t i = 0; i < count; ++i)
	{
		const auto* frame = m_SelectedClip->GetFrame(i);
		if (!frame) continue;

		std::string label = "Frame " + std::to_string(i) +
			" | Sprite " + std::to_string(frame->SpriteIndex) +
			" | " + std::to_string(frame->Duration * 1000.0f).substr(0, 4) + "ms";

		bool isSelected = (m_SelectedFrameIndex == i);
		if (ImGui::Selectable(label.c_str(), isSelected))
			m_SelectedFrameIndex = i;
	}

	ImGui::EndChild();
}

} // namespace Quentlam
