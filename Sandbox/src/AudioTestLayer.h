#pragma once

#include "Quentlam/Core/Layer.h"
#include "Quentlam/Audio/AudioModule.h"

class AudioTestLayer : public Quentlam::Layer
{
public:
	AudioTestLayer();
	~AudioTestLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Quentlam::Timestep ts) override;
	void OnEvent(Quentlam::Event& event) override;
	void OnImGuiLayer() override;

private:
	Quentlam::OrthographicCameraController m_CameraController;
	float m_CameraSpeed = 5.0f;

	uint32_t m_CurrentMusicId = 0;
	std::string m_CurrentMusicPath = "assets/audio/bgm_test.wav";

	float m_MasterVolume = 1.0f;
	float m_MusicVolume = 1.0f;
	float m_SfxVolume = 1.0f;

	bool m_MusicPlaying = false;
};
