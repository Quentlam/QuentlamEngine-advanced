#pragma once
#include "Quentlam/Core/Base.h"
#include "miniaudio.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

namespace Quentlam
{

class AudioPlaybackNode
{
public:
    AudioPlaybackNode() = default;
    explicit AudioPlaybackNode(ma_engine* engine);
    ~AudioPlaybackNode();

    AudioPlaybackNode(const AudioPlaybackNode&) = delete;
    AudioPlaybackNode& operator=(const AudioPlaybackNode&) = delete;
    AudioPlaybackNode(AudioPlaybackNode&& other) noexcept;
    AudioPlaybackNode& operator=(AudioPlaybackNode&& other) noexcept;

    bool IsValid() const { return m_Sound != nullptr; }
    bool IsPlaying() const;
    bool IsLooping() const;
    float GetVolume() const;
    float GetPitch() const;
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetLooping(bool loop);

    void Play();
    void Stop();
    void Pause();

    ma_sound* m_Sound = nullptr;
    bool m_Owned = false;
    void Invalidate();

private:
    ma_engine* m_Engine = nullptr;
};

class AudioBackend
{
public:
    AudioBackend();
    ~AudioBackend();

    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    AudioPlaybackNode* CreateSound(const std::string& filePath, bool spatial = false);
    void DestroySound(AudioPlaybackNode* node);

    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_MasterVolume; }

    void SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward = {0, 0, -1});

    ma_engine* GetEngine() { return &m_Engine; }

private:
    ma_engine m_Engine;
    bool m_Initialized = false;
    float m_MasterVolume = 1.0f;
    std::vector<AudioPlaybackNode*> m_ManagedNodes;
};

}
