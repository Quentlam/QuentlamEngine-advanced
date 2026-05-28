#include "qlpch.h"
#include "AudioBackend.h"

namespace Quentlam
{

AudioPlaybackNode::AudioPlaybackNode(ma_engine* engine)
    : m_Engine(engine)
{
    m_Sound = new ma_sound();
}

AudioPlaybackNode::~AudioPlaybackNode()
{
    Invalidate();
}

AudioPlaybackNode::AudioPlaybackNode(AudioPlaybackNode&& other) noexcept
    : m_Engine(other.m_Engine)
    , m_Sound(other.m_Sound)
    , m_Owned(other.m_Owned)
{
    other.m_Sound = nullptr;
    other.m_Owned = false;
}

AudioPlaybackNode& AudioPlaybackNode::operator=(AudioPlaybackNode&& other) noexcept
{
    if (this != &other)
    {
        Invalidate();
        m_Engine = other.m_Engine;
        m_Sound = other.m_Sound;
        m_Owned = other.m_Owned;
        other.m_Sound = nullptr;
        other.m_Owned = false;
    }
    return *this;
}

void AudioPlaybackNode::Invalidate()
{
    if (m_Sound && m_Owned)
    {
        if (ma_sound_is_playing(m_Sound))
            ma_sound_stop(m_Sound);
        ma_sound_uninit(m_Sound);
        delete m_Sound;
        m_Sound = nullptr;
        m_Owned = false;
    }
}

bool AudioPlaybackNode::IsPlaying() const
{
    return m_Sound && ma_sound_is_playing(m_Sound);
}

bool AudioPlaybackNode::IsLooping() const
{
    return m_Sound && ma_sound_is_looping(m_Sound);
}

float AudioPlaybackNode::GetVolume() const
{
    if (!m_Sound) return 0.0f;
    return ma_sound_get_volume(m_Sound);
}

float AudioPlaybackNode::GetPitch() const
{
    if (!m_Sound) return 1.0f;
    return ma_sound_get_pitch(m_Sound);
}

void AudioPlaybackNode::SetVolume(float volume)
{
    if (m_Sound) ma_sound_set_volume(m_Sound, volume);
}

void AudioPlaybackNode::SetPitch(float pitch)
{
    if (m_Sound) ma_sound_set_pitch(m_Sound, pitch);
}

void AudioPlaybackNode::SetLooping(bool loop)
{
    if (m_Sound) ma_sound_set_looping(m_Sound, loop);
}

void AudioPlaybackNode::Play()
{
    if (m_Sound && !ma_sound_is_playing(m_Sound))
        ma_sound_start(m_Sound);
}

void AudioPlaybackNode::Stop()
{
    if (m_Sound && ma_sound_is_playing(m_Sound))
        ma_sound_stop(m_Sound);
}

void AudioPlaybackNode::Pause()
{
    Stop();
}

// --- AudioBackend ---

AudioBackend::AudioBackend()
{
}

AudioBackend::~AudioBackend()
{
    Shutdown();
}

bool AudioBackend::Initialize()
{
    ma_result result = ma_engine_init(nullptr, &m_Engine);
    if (result != MA_SUCCESS)
    {
        QL_CORE_ERROR("AudioBackend: Failed to initialize engine: {0}", (int)result);
        return false;
    }

    m_Initialized = true;
    QL_CORE_INFO("AudioBackend initialized successfully.");
    return true;
}

void AudioBackend::Shutdown()
{
    if (!m_Initialized) return;

    for (auto* node : m_ManagedNodes)
    {
        node->Stop();
        delete node;
    }
    m_ManagedNodes.clear();

    ma_engine_uninit(&m_Engine);
    m_Initialized = false;
    QL_CORE_INFO("AudioBackend shutdown complete.");
}

void AudioBackend::Update(float)
{
}

AudioPlaybackNode* AudioBackend::CreateSound(const std::string& filePath, bool spatial)
{
    AudioPlaybackNode* node = new AudioPlaybackNode(&m_Engine);

    ma_result result = ma_sound_init_from_file(
        &m_Engine,
        filePath.c_str(),
        MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
        nullptr,
        nullptr,
        node->m_Sound
    );

    if (result != MA_SUCCESS)
    {
        QL_CORE_WARN("AudioBackend: Failed to load sound '{0}': {1}", filePath, (int)result);
        delete node;
        return nullptr;
    }

    if (spatial)
    {
        ma_sound_set_spatialization_enabled(node->m_Sound, true);
    }

    node->m_Owned = true;
    m_ManagedNodes.push_back(node);
    return node;
}

void AudioBackend::DestroySound(AudioPlaybackNode* node)
{
    if (!node) return;
    node->Invalidate();
    auto it = std::find(m_ManagedNodes.begin(), m_ManagedNodes.end(), node);
    if (it != m_ManagedNodes.end())
        m_ManagedNodes.erase(it);
    delete node;
}

void AudioBackend::SetMasterVolume(float volume)
{
    m_MasterVolume = volume;
    if (m_Initialized)
        ma_engine_set_volume(&m_Engine, volume);
}

void AudioBackend::SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward)
{
    if (!m_Initialized) return;
    ma_engine_listener_set_position(&m_Engine, 0, pos.x, pos.y, pos.z);
    ma_engine_listener_set_direction(&m_Engine, 0, forward.x, forward.y, forward.z);
}

}
