#include <Quentlam.h>
#include "AudioTestLayer.h"
#include "Quentlam/Events/ApplicationEvent.h"
#include "imgui/imgui.h"

AudioTestLayer::AudioTestLayer()
	: Layer("AudioTest"), m_CameraController(1280.0f / 720.0f)
{
}

void AudioTestLayer::OnAttach()
{
	// AudioModule 由 Application/Sandbox 统一初始化，这里不再重复
}

void AudioTestLayer::OnDetach()
{
	// AudioModule 由 Application/Sandbox 统一关闭，这里不再重复
}

void AudioTestLayer::OnUpdate(Quentlam::Timestep ts)
{
	QL_PROFILE_FUNCTION();
	m_CameraController.OnUpdate(ts);

	Quentlam::RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.1f, 1.0f));
	Quentlam::RenderCommand::Clear();

	Quentlam::AudioModule::Get().Update(ts);
}

void AudioTestLayer::OnEvent(Quentlam::Event& event)
{
	m_CameraController.OnEvent(event);
}

void AudioTestLayer::OnImGuiLayer()
{
	QL_PROFILE_FUNCTION();

	ImGui::Begin("Audio Test Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::CollapsingHeader("Master Controls", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::SliderFloat("Master Volume", &m_MasterVolume, 0.0f, 1.0f))
		{
			Quentlam::AudioModule::Get().SetMasterVolume(m_MasterVolume);
		}
	}

	if (ImGui::CollapsingHeader("Music", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char musicPathBuf[256] = {};
		strncpy_s(musicPathBuf, m_CurrentMusicPath.c_str(), sizeof(musicPathBuf) - 1);
		if (ImGui::InputText("Music Path", musicPathBuf, sizeof(musicPathBuf)))
		{
			m_CurrentMusicPath = musicPathBuf;
		}

		ImGui::SliderFloat("Music Volume", &m_MusicVolume, 0.0f, 1.0f);
		if (ImGui::Button("Play Music"))
		{
			if (m_CurrentMusicId != 0)
			{
				Quentlam::AudioModule::Get().Stop(m_CurrentMusicId);
			}
			Quentlam::AudioSource source(m_CurrentMusicPath, Quentlam::EAudioSourceType::Music);
			source.SetLooping(true);
			source.SetVolume(m_MusicVolume);
			m_CurrentMusicId = Quentlam::AudioModule::Get().Play(source);
			m_MusicPlaying = (m_CurrentMusicId != 0);
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop Music"))
		{
			if (m_CurrentMusicId != 0)
			{
				Quentlam::AudioModule::Get().Stop(m_CurrentMusicId);
				m_CurrentMusicId = 0;
				m_MusicPlaying = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Pause Music"))
		{
			if (m_CurrentMusicId != 0)
			{
				Quentlam::AudioModule::Get().Pause(m_CurrentMusicId);
				m_MusicPlaying = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Resume Music"))
		{
			if (m_CurrentMusicId != 0)
			{
				Quentlam::AudioModule::Get().Resume(m_CurrentMusicId);
				m_MusicPlaying = true;
			}
		}

		ImGui::Text("Status: %s", m_MusicPlaying ? "Playing" : "Stopped");
	}

	if (ImGui::CollapsingHeader("SFX", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static float sfxVol = 1.0f;
		ImGui::SliderFloat("SFX Volume", &sfxVol, 0.0f, 1.0f);

		if (ImGui::Button("Play Click SFX"))
		{
			Quentlam::AudioSource src("assets/audio/sfx_click.wav", Quentlam::EAudioSourceType::SFX);
			src.SetVolume(sfxVol);
			Quentlam::AudioModule::Get().Play(src);
		}
		ImGui::SameLine();
		if (ImGui::Button("Play Step SFX"))
		{
			Quentlam::AudioSource src("assets/audio/sfx_step.wav", Quentlam::EAudioSourceType::SFX);
			src.SetVolume(sfxVol);
			Quentlam::AudioModule::Get().Play(src);
		}
		ImGui::SameLine();
		if (ImGui::Button("Play Door SFX"))
		{
			Quentlam::AudioSource src("assets/audio/sfx_door.wav", Quentlam::EAudioSourceType::SFX);
			src.SetVolume(sfxVol);
			Quentlam::AudioModule::Get().Play(src);
		}
	}

	if (ImGui::CollapsingHeader("Ambient Zones", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Create Forest Zone"))
		{
			auto zone = Quentlam::CreateRef<Quentlam::AmbientZone>("forest_zone");
			zone->SetCenter({ 0.0f, 0.0f, 0.0f });
			zone->SetRadius(50.0f);
			zone->AddSound("assets/audio/ambient_forest.wav");
			Quentlam::AudioModule::Get().RegisterAmbientZone(zone);
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Forest Zone"))
		{
			Quentlam::AudioModule::Get().UnregisterAmbientZone("forest_zone");
		}

		ImGui::Text("Move camera near zone center to hear ambient audio.");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Hint: Place audio files in assets/audio/");
	ImGui::Text("Supported formats: WAV, MP3, OGG, FLAC");

	ImGui::End();
}
