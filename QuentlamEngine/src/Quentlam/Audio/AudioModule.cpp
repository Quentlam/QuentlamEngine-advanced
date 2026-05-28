#include "qlpch.h"
#include "AudioModule.h"
#include "AudioBackend.h"
#include "AudioState.h"
#include "miniaudio.h"
#include <glm/glm.hpp>

namespace Quentlam
{

AudioSource::AudioSource(const std::string& path, EAudioSourceType type)
	: m_Path(path), m_Type(type) {}

AudioBus::AudioBus(const std::string& name, EAudioBusType type)
	: m_Name(name), m_Type(type) {}

SfxEmitter::SfxEmitter(const std::string& id)
	: m_Id(id) {}

AmbientZone::AmbientZone(const std::string& id)
	: m_Id(id) {}

void AmbientZone::AddSound(const std::string& soundPath, float weight)
{
	m_Sounds.push_back({ soundPath, weight });
}

void AmbientZone::RemoveSound(const std::string& soundPath)
{
	m_Sounds.erase(
		std::remove_if(m_Sounds.begin(), m_Sounds.end(),
			[&soundPath](const auto& p) { return p.first == soundPath; }),
		m_Sounds.end()
	);
}

void AudioMixerState::RegisterBus(const AudioBus& bus)
{
	m_Buses[bus.GetName()] = bus;
}

void AudioMixerState::RemoveBus(const std::string& name)
{
	m_Buses.erase(name);
}

AudioBus* AudioMixerState::GetBus(const std::string& name)
{
	return const_cast<AudioBus*>(static_cast<const AudioMixerState*>(this)->GetBus(name));
}

const AudioBus* AudioMixerState::GetBus(const std::string& name) const
{
	auto it = m_Buses.find(name);
	return it != m_Buses.end() ? &it->second : nullptr;
}

float AudioMixerState::GetBusVolume(const std::string& name) const
{
	auto* bus = GetBus(name);
	return bus ? bus->GetVolume() : 1.0f;
}

void AudioMixerState::SetBusVolume(const std::string& name, float volume)
{
	auto* bus = GetBus(name);
	if (bus) bus->SetVolume(volume);
}

void AudioMixerState::MuteBus(const std::string& name, bool muted)
{
	auto* bus = GetBus(name);
	if (bus) bus->SetMuted(muted);
}

void AudioMixerState::PauseBus(const std::string& name, bool paused)
{
	auto* bus = GetBus(name);
	if (bus) bus->SetPaused(paused);
}

void AudioMixerState::MuteAll(bool muted)
{
	for (auto& [name, bus] : m_Buses)
		bus.SetMuted(muted);
}

void AudioMixerState::PauseAll(bool paused)
{
	for (auto& [name, bus] : m_Buses)
		bus.SetPaused(paused);
}

void AudioMixerState::PushState()
{
	m_StateStack.push_back(m_Buses);
}

void AudioMixerState::PopState()
{
	if (!m_StateStack.empty())
	{
		m_Buses = m_StateStack.back();
		m_StateStack.pop_back();
	}
}

float AudioMixerState::GetEffectiveVolume(const std::string& busName) const
{
	return CalculateEffectiveVolume(busName);
}

float AudioMixerState::CalculateEffectiveVolume(const std::string& busName) const
{
	auto* bus = GetBus(busName);
	if (!bus) return 1.0f;

	float volume = bus->GetVolume();
	if (bus->IsMuted()) return 0.0f;

	if (!bus->GetParentName().empty())
		volume *= CalculateEffectiveVolume(bus->GetParentName());

	return volume;
}

void MusicController::Play(const std::string& trackPath, float fadeInDuration)
{
	m_PreviousTrack = m_CurrentTrack;
	m_CurrentTrack = trackPath;
	m_TheState = EAUDIOSTATE::Playing;
	m_FadeProgress = 0.0f;
	m_FadeDuration = fadeInDuration;
	m_Fading = true;
}

void MusicController::Stop(float fadeOutDuration)
{
	m_PreviousTrack = m_CurrentTrack;
	m_FadeProgress = 0.0f;
	m_FadeDuration = fadeOutDuration;
	m_Fading = true;
	m_TheState = EAUDIOSTATE::Stopped;
}

void MusicController::Pause()
{
	m_TheState = EAUDIOSTATE::Paused;
}

void MusicController::Resume()
{
	m_TheState = EAUDIOSTATE::Playing;
}

void MusicController::CrossfadeTo(const std::string& trackPath, float fadeDuration)
{
	m_PreviousTrack = m_CurrentTrack;
	m_CurrentTrack = trackPath;
	m_FadeProgress = 0.0f;
	m_FadeDuration = fadeDuration;
	m_Fading = true;
	m_TheState = EAUDIOSTATE::Playing;
}

void MusicController::SetVolume(float volume, float fadeDuration)
{
	m_Volume = volume;
	if (fadeDuration > 0.0f)
	{
		m_FadeDuration = fadeDuration;
		m_FadeProgress = 0.0f;
		m_Fading = true;
	}
}

void MusicController::SetNextTrack(const std::string& trackPath, float fadeDuration)
{
	m_NextTrack = trackPath;
	m_NextTrackFadeDuration = fadeDuration;
}

// --- AudioModule ---

AudioModule::AudioModule()
{
}

AudioModule::~AudioModule()
{
	Shutdown();
}

AudioModule& AudioModule::Get()
{
	static AudioModule instance;
	return instance;
}

void AudioModule::Initialize()
{
	m_Backend = std::make_unique<AudioBackend>();
	if (!m_Backend->Initialize())
	{
		QL_CORE_ERROR("AudioModule: Backend initialization failed!");
		m_Backend.reset();
		return;
	}

	m_Mixer.RegisterBus(AudioBus("Master", EAudioBusType::Master));
	m_Mixer.RegisterBus(AudioBus("Music", EAudioBusType::MusicBus));
	m_Mixer.RegisterBus(AudioBus("SFX", EAudioBusType::SFXBus));
	m_Mixer.RegisterBus(AudioBus("Ambient", EAudioBusType::AmbientBus));
	m_Mixer.RegisterBus(AudioBus("Voice", EAudioBusType::VoiceBus));
	m_Mixer.RegisterBus(AudioBus("UI", EAudioBusType::UIBus));

	auto* master = m_Mixer.GetBus("Master");
	if (master) master->AddChild("Music");
	if (master) master->AddChild("SFX");
	if (master) master->AddChild("Ambient");
	if (master) master->AddChild("Voice");
	if (master) master->AddChild("UI");

	auto* music = m_Mixer.GetBus("Music");
	if (music) music->SetParent("Master");
	auto* sfx = m_Mixer.GetBus("SFX");
	if (sfx) sfx->SetParent("Master");
	auto* ambient = m_Mixer.GetBus("Ambient");
	if (ambient) ambient->SetParent("Master");
	auto* voice = m_Mixer.GetBus("Voice");
	if (voice) voice->SetParent("Master");
	auto* ui = m_Mixer.GetBus("UI");
	if (ui) ui->SetParent("Master");

	QL_CORE_INFO("AudioModule initialized successfully.");
}

void AudioModule::Shutdown()
{
	if (m_Backend)
	{
		StopAll();
		m_Backend->Shutdown();
		m_Backend.reset();
	}
	m_AmbientZones.clear();
	m_ActiveInstances.clear();
	QL_CORE_INFO("AudioModule shutdown complete.");
}

void AudioModule::Update(float deltaTime)
{
	if (m_Backend)
		m_Backend->Update(deltaTime);

	auto it = m_ActiveInstances.begin();
	while (it != m_ActiveInstances.end())
	{
		bool stillPlaying = false;
		if (it->second.PlaybackNode)
			stillPlaying = it->second.PlaybackNode->IsPlaying();

		if (!stillPlaying && it->second.TheState == EAUDIOSTATE::Playing)
		{
			it->second.TheState = EAUDIOSTATE::Stopped;
			if (OnSoundFinished)
				OnSoundFinished(it->second.Path);
			if (it->second.Type != EAudioSourceType::Music)
			{
				it = m_ActiveInstances.erase(it);
				continue;
			}
		}
		++it;
	}
}

uint32_t AudioModule::Play(const std::string& path, EAudioSourceType type)
{
	AudioSource source(path, type);
	return Play(source);
}

uint32_t AudioModule::Play(const AudioSource& source)
{
	if (!m_Backend)
		return 0;

	uint32_t id = GenerateInstanceId();
	auto node = std::make_shared<AudioPlaybackNode>();
	node->m_Sound = new ma_sound();
	ma_result result = ma_sound_init_from_file(
		m_Backend->GetEngine(),
		source.GetPath().c_str(),
		MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
		nullptr,
		nullptr,
		node->m_Sound
	);

	if (result != MA_SUCCESS)
	{
		QL_CORE_WARN("AudioModule: Failed to load sound '{0}': {1}", source.GetPath(), (int)result);
		delete node->m_Sound;
		return 0;
	}

	if (source.IsSpatial())
	{
		ma_sound_set_spatialization_enabled(node->m_Sound, true);
		ma_sound_set_position(node->m_Sound, source.GetPosition().x, source.GetPosition().y, source.GetPosition().z);
	}

	node->SetVolume(source.GetVolume());
	node->SetPitch(source.GetPitch());
	node->SetLooping(source.IsLooping());
	node->Play();

	AudioInstance inst;
	inst.InstanceId = id;
	inst.Type = source.GetType();
	inst.Path = source.GetPath();
	inst.TheState = EAUDIOSTATE::Playing;
	inst.Volume = source.GetVolume();
	inst.Pitch = source.GetPitch();
	inst.IsLooping = source.IsLooping();
	inst.IsSpatial = source.IsSpatial();
	inst.Position = source.GetPosition();
	inst.PlaybackNode = node;

	m_ActiveInstances[id] = inst;

	if (source.GetType() == EAudioSourceType::Music)
	{
		m_MusicController.SetPlaybackNode(node);
		m_MusicController.Play(source.GetPath());
	}

	QL_CORE_TRACE("AudioModule: Playing '{0}' (id={1}, type={2})", source.GetPath(), id, (int)source.GetType());
	return id;
}

void AudioModule::Stop(uint32_t instanceId)
{
	auto it = m_ActiveInstances.find(instanceId);
	if (it != m_ActiveInstances.end())
	{
		if (it->second.PlaybackNode)
			it->second.PlaybackNode->Stop();
		it->second.TheState = EAUDIOSTATE::Stopped;
		if (OnSoundFinished)
			OnSoundFinished(it->second.Path);
		m_ActiveInstances.erase(it);
	}
}

void AudioModule::Pause(uint32_t instanceId)
{
	auto it = m_ActiveInstances.find(instanceId);
	if (it != m_ActiveInstances.end())
	{
		if (it->second.PlaybackNode)
			it->second.PlaybackNode->Pause();
		it->second.TheState = EAUDIOSTATE::Paused;
	}
}

void AudioModule::Resume(uint32_t instanceId)
{
	auto it = m_ActiveInstances.find(instanceId);
	if (it != m_ActiveInstances.end())
	{
		if (it->second.PlaybackNode)
			it->second.PlaybackNode->Play();
		it->second.TheState = EAUDIOSTATE::Playing;
	}
}

void AudioModule::StopAll(EAudioSourceType type)
{
	for (auto it = m_ActiveInstances.begin(); it != m_ActiveInstances.end(); )
	{
		if (type == EAudioSourceType::SFX || it->second.Type == type)
		{
			if (it->second.PlaybackNode)
				it->second.PlaybackNode->Stop();
			if (OnSoundFinished)
				OnSoundFinished(it->second.Path);
			it = m_ActiveInstances.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void AudioModule::PauseAll(EAudioSourceType type)
{
	for (auto& [id, inst] : m_ActiveInstances)
	{
		if (type == EAudioSourceType::SFX || inst.Type == type)
		{
			if (inst.PlaybackNode)
				inst.PlaybackNode->Pause();
			inst.TheState = EAUDIOSTATE::Paused;
		}
	}
}

void AudioModule::ResumeAll()
{
	for (auto& [id, inst] : m_ActiveInstances)
	{
		if (inst.PlaybackNode)
			inst.PlaybackNode->Play();
		inst.TheState = EAUDIOSTATE::Playing;
	}
}

bool AudioModule::IsPlaying(uint32_t instanceId) const
{
	auto it = m_ActiveInstances.find(instanceId);
	return it != m_ActiveInstances.end() && it->second.TheState == EAUDIOSTATE::Playing;
}

float AudioModule::GetVolume(uint32_t instanceId) const
{
	auto it = m_ActiveInstances.find(instanceId);
	if (it != m_ActiveInstances.end())
	{
		if (it->second.PlaybackNode)
			return it->second.PlaybackNode->GetVolume();
		return it->second.Volume;
	}
	return 0.0f;
}

void AudioModule::SetVolume(uint32_t instanceId, float volume)
{
	auto it = m_ActiveInstances.find(instanceId);
	if (it != m_ActiveInstances.end())
	{
		it->second.Volume = volume;
		if (it->second.PlaybackNode)
			it->second.PlaybackNode->SetVolume(volume);
	}
}

void AudioModule::RegisterAmbientZone(Ref<AmbientZone> zone)
{
	if (zone)
		m_AmbientZones[zone->GetId()] = zone;
}

void AudioModule::UnregisterAmbientZone(const std::string& zoneId)
{
	m_AmbientZones.erase(zoneId);
}

AmbientZone* AudioModule::GetAmbientZone(const std::string& zoneId)
{
	auto it = m_AmbientZones.find(zoneId);
	return it != m_AmbientZones.end() ? it->second.get() : nullptr;
}

void AudioModule::UpdateAmbientZones(const glm::vec3& listenerPos)
{
	if (!m_Backend) return;

	for (auto& [id, zone] : m_AmbientZones)
	{
		float dist = glm::distance(glm::vec2(listenerPos.x, listenerPos.y),
								   glm::vec2(zone->GetCenter().x, zone->GetCenter().y));

		float influence = 0.0f;
		if (dist < zone->GetRadius())
		{
			influence = 1.0f - (dist / zone->GetRadius());
		}

		bool wasActive = zone->IsActive();
		zone->SetActive(influence > 0.0f);

		if (influence > 0.0f)
		{
			if (!zone->PlaybackNode || !zone->PlaybackNode->IsPlaying())
			{
				if (!zone->GetSounds().empty())
				{
					auto node = std::make_shared<AudioPlaybackNode>();
					node->m_Sound = new ma_sound();
					ma_result result = ma_sound_init_from_file(
						m_Backend->GetEngine(),
						zone->GetSounds()[0].first.c_str(),
						MA_SOUND_FLAG_DECODE,
						nullptr,
						nullptr,
						node->m_Sound
					);
					if (result == MA_SUCCESS)
					{
						node->SetLooping(true);
						node->SetVolume(influence * zone->GetTransitionDuration());
						node->Play();
						zone->PlaybackNode = node;
					}
				}
			}
			else if (zone->PlaybackNode)
			{
				float targetVol = influence;
				float currentVol = zone->PlaybackNode->GetVolume();
				zone->PlaybackNode->SetVolume(currentVol + (targetVol - currentVol) * 0.1f);
			}
			zone->SetCurrentVolume(influence);
		}
		else if (wasActive && zone->PlaybackNode)
		{
			zone->PlaybackNode->Stop();
			zone->SetCurrentSoundIndex(-1);
		}
	}
}

void AudioModule::SetMasterVolume(float volume)
{
	m_MasterVolume = volume;
	if (m_Backend)
		m_Backend->SetMasterVolume(volume);
}

void AudioModule::SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward)
{
	m_ListenerPosition = pos;
	m_ListenerForward = forward;
	if (m_Backend)
		m_Backend->SetListenerPosition(pos, forward);
}

}
