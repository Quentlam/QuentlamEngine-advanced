#pragma once
#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace Quentlam
{

class AudioBackend;
class AudioPlaybackNode;

enum class EAudioSourceType : uint8_t
{
	SFX = 0,
	Music = 1,
	Ambient = 2,
	Voice = 3,
	UI = 4,
	Custom = 100
};

enum class EAUDIOSTATE : uint8_t
{
	Stopped = 0,
	Playing = 1,
	Paused = 2
};

enum class EAudioBusType : uint8_t
{
	Master = 0,
	MusicBus = 1,
	SFXBus = 2,
	VoiceBus = 3,
	AmbientBus = 4,
	UIBus = 5
};

struct AudioInstance
{
	uint32_t InstanceId = 0;
	EAudioSourceType Type = EAudioSourceType::SFX;
	std::string Path;
	EAUDIOSTATE TheState = EAUDIOSTATE::Stopped;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	bool IsLooping = false;
	bool IsSpatial = false;
	glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	std::shared_ptr<AudioPlaybackNode> PlaybackNode;
};

class QUENTLAM_API AudioSource
{
public:
	AudioSource() = default;
	AudioSource(const std::string& path, EAudioSourceType type);

	void SetPath(const std::string& path) { m_Path = path; }
	const std::string& GetPath() const { return m_Path; }

	void SetType(EAudioSourceType type) { m_Type = type; }
	EAudioSourceType GetType() const { return m_Type; }

	void SetVolume(float volume) { m_Volume = volume; }
	float GetVolume() const { return m_Volume; }

	void SetPitch(float pitch) { m_Pitch = pitch; }
	float GetPitch() const { return m_Pitch; }

	void SetLooping(bool looping) { m_IsLooping = looping; }
	bool IsLooping() const { return m_IsLooping; }

	void SetPosition(const glm::vec3& pos) { m_Position = pos; }
	const glm::vec3& GetPosition() const { return m_Position; }

	void SetSpatial(bool spatial) { m_IsSpatial = spatial; }
	bool IsSpatial() const { return m_IsSpatial; }

	void SetFadeInDuration(float seconds) { m_FadeInDuration = seconds; }
	float GetFadeInDuration() const { return m_FadeInDuration; }
	void SetFadeOutDuration(float seconds) { m_FadeOutDuration = seconds; }
	float GetFadeOutDuration() const { return m_FadeOutDuration; }

	void SetInstanceId(uint32_t id) { m_InstanceId = id; }
	uint32_t GetInstanceId() const { return m_InstanceId; }

	const std::string& GetBusName() const { return m_BusName; }
	void SetBusName(const std::string& name) { m_BusName = name; }

	std::shared_ptr<AudioPlaybackNode> PlaybackNode;

private:
	std::string m_Path;
	EAudioSourceType m_Type = EAudioSourceType::SFX;
	float m_Volume = 1.0f;
	float m_Pitch = 1.0f;
	bool m_IsLooping = false;
	bool m_IsSpatial = false;
	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_FadeInDuration = 0.0f;
	float m_FadeOutDuration = 0.0f;
	uint32_t m_InstanceId = 0;
	std::string m_BusName = "Master";
};

class QUENTLAM_API AudioBus
{
public:
	AudioBus() = default;
	explicit AudioBus(const std::string& name, EAudioBusType type = EAudioBusType::Master);

	const std::string& GetName() const { return m_Name; }
	EAudioBusType GetType() const { return m_Type; }

	void SetVolume(float volume) { m_Volume = volume; }
	float GetVolume() const { return m_Volume; }

	void SetMuted(bool muted) { m_Muted = muted; }
	bool IsMuted() const { return m_Muted; }

	void SetPaused(bool paused) { m_Paused = paused; }
	bool IsPaused() const { return m_Paused; }

	void SetParent(const std::string& parentName) { m_ParentName = parentName; }
	const std::string& GetParentName() const { return m_ParentName; }

	void AddChild(const std::string& childName) { m_Children.push_back(childName); }
	const std::vector<std::string>& GetChildren() const { return m_Children; }

private:
	std::string m_Name;
	EAudioBusType m_Type = EAudioBusType::Master;
	float m_Volume = 1.0f;
	bool m_Muted = false;
	bool m_Paused = false;
	std::string m_ParentName;
	std::vector<std::string> m_Children;
};

class QUENTLAM_API AudioMixerState
{
public:
	AudioMixerState() = default;

	void RegisterBus(const AudioBus& bus);
	void RemoveBus(const std::string& name);
	AudioBus* GetBus(const std::string& name);
	const AudioBus* GetBus(const std::string& name) const;
	const std::unordered_map<std::string, AudioBus>& GetAllBuses() const { return m_Buses; }

	float GetBusVolume(const std::string& name) const;
	void SetBusVolume(const std::string& name, float volume);
	void MuteBus(const std::string& name, bool muted);
	void PauseBus(const std::string& name, bool paused);
	void MuteAll(bool muted);
	void PauseAll(bool paused);

	void PushState();
	void PopState();

	float GetEffectiveVolume(const std::string& busName) const;

private:
	float CalculateEffectiveVolume(const std::string& busName) const;

	std::unordered_map<std::string, AudioBus> m_Buses;
	std::vector<std::unordered_map<std::string, AudioBus>> m_StateStack;
};

class QUENTLAM_API SfxEmitter
{
public:
	SfxEmitter() = default;
	explicit SfxEmitter(const std::string& id);

	const std::string& GetId() const { return m_Id; }

	void SetPosition(const glm::vec3& pos) { m_Position = pos; }
	const glm::vec3& GetPosition() const { return m_Position; }

	void SetRadius(float radius) { m_Radius = radius; }
	float GetRadius() const { return m_Radius; }

	void SetVolume(float volume) { m_Volume = volume; }
	float GetVolume() const { return m_Volume; }

	void SetPitch(float pitch) { m_Pitch = pitch; }
	float GetPitch() const { return m_Pitch; }

	void SetSpatial(bool spatial) { m_Spatial = spatial; }
	bool IsSpatial() const { return m_Spatial; }

	const std::string& GetCurrentSound() const { return m_CurrentSound; }
	bool IsPlaying() const { return m_IsPlaying; }
	void SetPlaying(bool playing) { m_IsPlaying = playing; }

private:
	std::string m_Id;
	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_Radius = 10.0f;
	float m_Volume = 1.0f;
	float m_Pitch = 1.0f;
	bool m_Spatial = false;
	std::string m_CurrentSound;
	bool m_IsPlaying = false;
};

class QUENTLAM_API AmbientZone
{
public:
	AmbientZone() = default;
	explicit AmbientZone(const std::string& id);

	const std::string& GetId() const { return m_Id; }

	void SetCenter(const glm::vec3& center) { m_Center = center; }
	const glm::vec3& GetCenter() const { return m_Center; }

	void SetRadius(float radius) { m_Radius = radius; }
	float GetRadius() const { return m_Radius; }

	void AddSound(const std::string& soundPath, float weight = 1.0f);
	void RemoveSound(const std::string& soundPath);
	const std::vector<std::pair<std::string, float>>& GetSounds() const { return m_Sounds; }

	void SetTransitionDuration(float seconds) { m_TransitionDuration = seconds; }
	float GetTransitionDuration() const { return m_TransitionDuration; }

	void SetActive(bool active) { m_Active = active; }
	bool IsActive() const { return m_Active; }

	int32_t GetCurrentSoundIndex() const { return m_CurrentSoundIndex; }
	void SetCurrentSoundIndex(int32_t idx) { m_CurrentSoundIndex = idx; }

	float GetCurrentVolume() const { return m_CurrentVolume; }
	void SetCurrentVolume(float vol) { m_CurrentVolume = vol; }

	std::shared_ptr<AudioPlaybackNode> PlaybackNode;

private:
	std::string m_Id;
	glm::vec3 m_Center = { 0.0f, 0.0f, 0.0f };
	float m_Radius = 50.0f;
	std::vector<std::pair<std::string, float>> m_Sounds;
	float m_TransitionDuration = 2.0f;
	bool m_Active = false;
	int32_t m_CurrentSoundIndex = -1;
	float m_CurrentVolume = 0.0f;
	float m_TransitionProgress = 0.0f;
};

class QUENTLAM_API MusicController
{
public:
	MusicController() = default;

	void Play(const std::string& trackPath, float fadeInDuration = 1.0f);
	void Stop(float fadeOutDuration = 1.0f);
	void Pause();
	void Resume();

	void CrossfadeTo(const std::string& trackPath, float fadeDuration = 2.0f);
	void SetVolume(float volume, float fadeDuration = 0.0f);

	const std::string& GetCurrentTrack() const { return m_CurrentTrack; }
	const std::string& GetPreviousTrack() const { return m_PreviousTrack; }
	EAUDIOSTATE GetAudioState() const { return m_TheState; }

	void SetNextTrack(const std::string& trackPath, float fadeDuration = 1.0f);
	const std::string& GetNextTrack() const { return m_NextTrack; }
	float GetNextTrackFadeDuration() const { return m_NextTrackFadeDuration; }

	void SetVolumeOnly(float vol) { m_Volume = vol; }
	float GetVolumeOnly() const { return m_Volume; }

	void SetPlaybackNode(std::shared_ptr<AudioPlaybackNode> node) { m_PlaybackNode = node; }
	std::shared_ptr<AudioPlaybackNode> GetPlaybackNode() const { return m_PlaybackNode; }

	void SetFadeProgress(float p) { m_FadeProgress = p; }
	float GetFadeProgress() const { return m_FadeProgress; }
	void SetFadeDuration(float d) { m_FadeDuration = d; }
	float GetFadeDuration() const { return m_FadeDuration; }
	void SetFading(bool f) { m_Fading = f; }
	bool IsFading() const { return m_Fading; }

private:
	std::string m_CurrentTrack;
	std::string m_PreviousTrack;
	std::string m_NextTrack;
	float m_NextTrackFadeDuration = 1.0f;
	float m_Volume = 1.0f;
	EAUDIOSTATE m_TheState = EAUDIOSTATE::Stopped;
	float m_FadeProgress = 0.0f;
	float m_FadeDuration = 0.0f;
	bool m_Fading = false;
	std::shared_ptr<AudioPlaybackNode> m_PlaybackNode;
};

class QUENTLAM_API AudioModule
{
public:
	AudioModule();
	~AudioModule();

	static AudioModule& Get();

	void Initialize();
	void Shutdown();

	void Update(float deltaTime);

	uint32_t Play(const std::string& path, EAudioSourceType type = EAudioSourceType::SFX);
	uint32_t Play(const AudioSource& source);
	void Stop(uint32_t instanceId);
	void Pause(uint32_t instanceId);
	void Resume(uint32_t instanceId);
	void StopAll(EAudioSourceType type = EAudioSourceType::SFX);
	void PauseAll(EAudioSourceType type = EAudioSourceType::SFX);
	void ResumeAll();

	bool IsPlaying(uint32_t instanceId) const;
	float GetVolume(uint32_t instanceId) const;
	void SetVolume(uint32_t instanceId, float volume);

	AudioMixerState& GetMixer() { return m_Mixer; }
	const AudioMixerState& GetMixer() const { return m_Mixer; }

	MusicController& GetMusicController() { return m_MusicController; }
	const MusicController& GetMusicController() const { return m_MusicController; }

	void RegisterAmbientZone(Ref<AmbientZone> zone);
	void UnregisterAmbientZone(const std::string& zoneId);
	AmbientZone* GetAmbientZone(const std::string& zoneId);
	void UpdateAmbientZones(const glm::vec3& listenerPos);

	void SetMasterVolume(float volume);
	float GetMasterVolume() const { return m_MasterVolume; }

	void SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward = {0, 0, -1});
	const glm::vec3& GetListenerPosition() const { return m_ListenerPosition; }

	uint32_t GenerateInstanceId() { return ++m_InstanceIdCounter; }

	std::function<void(const std::string& path)> OnSoundFinished;

	AudioBackend* GetBackend() { return m_Backend.get(); }

private:
	float m_MasterVolume = 1.0f;
	glm::vec3 m_ListenerPosition = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_ListenerForward = { 0.0f, 0.0f, -1.0f };
	uint32_t m_InstanceIdCounter = 0;

	std::unique_ptr<AudioBackend> m_Backend;
	AudioMixerState m_Mixer;
	MusicController m_MusicController;
	std::unordered_map<std::string, Ref<AmbientZone>> m_AmbientZones;
	std::unordered_map<uint32_t, AudioInstance> m_ActiveInstances;
};

}
