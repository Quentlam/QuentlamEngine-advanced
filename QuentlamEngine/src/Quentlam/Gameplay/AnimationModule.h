#pragma once
#include "Quentlam/Core/Base.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace Quentlam
{
	class AnimationClip;

	enum class EAnimationWrapMode : uint8_t
	{
		Once = 0,
		Loop = 1,
		PingPong = 2,
		ClampForever = 3
	};

	enum class EAnimationDirection : uint8_t
	{
		Forward = 0,
		Backward = 1
	};

	struct AnimationFrame
	{
		int32_t SpriteIndex = 0;
		float Duration = 0.1f;
		std::string EventName;
		std::unordered_map<std::string, float> FloatParams;
		std::unordered_map<std::string, int32_t> IntParams;
		std::unordered_map<std::string, std::string> StringParams;
	};

	struct QUENTLAM_API SpriteAtlasBinding
	{
		std::string AtlasPath;
		int32_t AtlasColumns = 1;
		int32_t AtlasRows = 1;
		glm::ivec2 DefaultFrameSize = { 32, 32 };

		glm::vec2 GetUVForFrame(int32_t frameIndex) const;
		glm::vec2 GetUVSize() const { return { 1.0f / AtlasColumns, 1.0f / AtlasRows }; }
	};

	class QUENTLAM_API AnimationClip
	{
	public:
		AnimationClip() = default;
		AnimationClip(const std::string& name, float fps = 12.0f);
		AnimationClip(const std::string& name, const SpriteAtlasBinding& atlas, float fps = 12.0f);
		AnimationClip(const std::string& name, Ref<SpriteAtlasBinding> atlas, float fps = 12.0f);

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		float GetFPS() const { return m_FPS; }
		void SetFPS(float fps) { m_FPS = fps; }

		float GetDuration() const;
		int32_t GetFrameCount() const { return static_cast<int32_t>(m_Frames.size()); }

		void AddFrame(const AnimationFrame& frame);
		void AddFrame(int32_t spriteIndex, float duration = 0.1f);
		void RemoveFrame(int32_t index);
		void ClearFrames();

		const AnimationFrame* GetFrame(int32_t index) const;
		AnimationFrame* GetFrame(int32_t index);

		float GetFrameStartTime(int32_t frameIndex) const;
		int32_t GetFrameAtTime(float time) const;

		EAnimationWrapMode GetWrapMode() const { return m_WrapMode; }
		void SetWrapMode(EAnimationWrapMode mode) { m_WrapMode = mode; }

		bool IsValid() const { return !m_Frames.empty(); }

		const SpriteAtlasBinding& GetAtlasBinding() const { return m_AtlasBinding; }
		void SetAtlasBinding(const SpriteAtlasBinding& binding) { m_AtlasBinding = binding; }
		bool HasAtlasBinding() const { return !m_AtlasBinding.AtlasPath.empty(); }

		std::string SerializeToJson() const;
		bool DeserializeFromJson(const std::string& json);

	private:
		std::string m_Name;
		std::vector<AnimationFrame> m_Frames;
		float m_FPS = 12.0f;
		EAnimationWrapMode m_WrapMode = EAnimationWrapMode::Loop;
		SpriteAtlasBinding m_AtlasBinding;
	};

	class QUENTLAM_API Animator
	{
	public:
		Animator() = default;
		Animator(Ref<AnimationClip> defaultClip);

		void Play(Ref<AnimationClip> clip, float blendTime = 0.0f);
		void Play(const std::string& clipName, float blendTime = 0.0f);
		void Stop();
		void Pause();
		void Resume();
		void Reset();

		void Update(float deltaTime);

		bool IsPlaying() const { return m_IsPlaying; }
		bool IsPaused() const { return m_IsPaused; }

		float GetCurrentTime() const { return m_CurrentTime; }
		void SetCurrentTime(float time);

		int32_t GetCurrentFrame() const { return GetCurrentClip() ? GetCurrentClip()->GetFrameAtTime(m_CurrentTime) : -1; }
		const AnimationFrame* GetCurrentFrameData() const;

		Ref<AnimationClip> GetCurrentClip() const { return m_CurrentClip; }
		Ref<AnimationClip> GetPreviousClip() const { return m_PreviousClip; }

		float GetBlendFactor() const { return m_BlendFactor; }
		bool IsBlending() const { return m_BlendFactor < 1.0f; }

		void SetFrameFinishedCallback(std::function<void()> callback);
		void SetAnimationFinishedCallback(std::function<void()> callback);
		void SetFrameEventCallback(std::function<void(const AnimationFrame&)> callback);

		void RegisterClip(const std::string& name, Ref<AnimationClip> clip) { m_Clips[name] = clip; }

		const std::string& GetCurrentState() const { return m_CurrentStateName; }
		void SetCurrentState(const std::string& state) { m_CurrentStateName = state; }

	private:
		void AdvanceFrame(float deltaTime);
		int32_t GetNextFrameIndex(int32_t current) const;

		Ref<AnimationClip> m_CurrentClip;
		Ref<AnimationClip> m_PreviousClip;
		Ref<AnimationClip> m_BaseLayerClip;

		float m_CurrentTime = 0.0f;
		int32_t m_CurrentFrameIndex = 0;
		EAnimationDirection m_Direction = EAnimationDirection::Forward;

		bool m_IsPlaying = false;
		bool m_IsPaused = false;

		float m_BlendTime = 0.0f;
		float m_BlendTimer = 0.0f;
		float m_BlendFactor = 1.0f;

		std::string m_CurrentStateName;

		std::function<void()> m_FrameFinishedCallback;
		std::function<void()> m_AnimationFinishedCallback;
		std::function<void(const AnimationFrame&)> m_FrameEventCallback;

		std::unordered_map<std::string, Ref<AnimationClip>> m_Clips;
	};

	class QUENTLAM_API AnimationStateMachine
	{
	public:
		struct StateTransition
		{
			std::string TargetState;
			float Duration = 0.0f;
			std::function<bool()> Condition;
			bool HasExitTime = false;
			float ExitTime = 0.0f;
		};

		struct State
		{
			std::string Name;
			Ref<AnimationClip> Clip;
			std::vector<StateTransition> Transitions;
			bool IsEntryState = false;
		};

		AnimationStateMachine();

		void AddState(const std::string& name, Ref<AnimationClip> clip, bool isEntry = false);
		void RemoveState(const std::string& name);
		State* GetState(const std::string& name);

		void AddTransition(const std::string& from, const std::string& to,
			float duration = 0.0f, bool hasExitTime = false, float exitTime = 0.0f);
		void AddTransition(const std::string& from, const std::string& to,
			std::function<bool()> condition, float duration = 0.0f);
		void RemoveTransition(const std::string& from, const std::string& to);

		void SetEntryState(const std::string& name);
		void ForceState(const std::string& name);

		void Update(Animator& animator, float deltaTime);

		const std::string& GetCurrentStateName() const { return m_CurrentStateName; }
		State* GetCurrentState();
		const State* GetCurrentState() const;

	private:
		void EvaluateTransitions();

		std::unordered_map<std::string, State> m_States;
		std::string m_EntryStateName;
		std::string m_CurrentStateName;
		std::string m_PreviousStateName;

		float m_StateTime = 0.0f;
		bool m_Transitioning = false;
		float m_TransitionProgress = 0.0f;
		StateTransition* m_PendingTransition = nullptr;
	};

	inline glm::vec2 SpriteAtlasBinding::GetUVForFrame(int32_t frameIndex) const
	{
		int32_t col = frameIndex % AtlasColumns;
		int32_t row = frameIndex / AtlasColumns;
		return { static_cast<float>(col) / AtlasColumns, static_cast<float>(row) / AtlasRows };
	}

	inline AnimationClip::AnimationClip(const std::string& name, float fps)
		: m_Name(name), m_FPS(fps) {}

	inline AnimationClip::AnimationClip(const std::string& name, const SpriteAtlasBinding& atlas, float fps)
		: m_Name(name), m_AtlasBinding(atlas), m_FPS(fps) {}

	inline AnimationClip::AnimationClip(const std::string& name, Ref<SpriteAtlasBinding> atlas, float fps)
		: m_Name(name), m_AtlasBinding(atlas ? *atlas : SpriteAtlasBinding{}), m_FPS(fps) {}

	inline float AnimationClip::GetDuration() const
	{
		float duration = 0.0f;
		for (const auto& frame : m_Frames)
			duration += frame.Duration;
		if (duration == 0.0f && !m_Frames.empty())
			duration = m_Frames.size() / m_FPS;
		return duration;
	}

	inline void AnimationClip::AddFrame(const AnimationFrame& frame)
	{
		m_Frames.push_back(frame);
	}

	inline void AnimationClip::AddFrame(int32_t spriteIndex, float duration)
	{
		AnimationFrame frame;
		frame.SpriteIndex = spriteIndex;
		frame.Duration = duration > 0 ? duration : (1.0f / m_FPS);
		m_Frames.push_back(frame);
	}

	inline void AnimationClip::RemoveFrame(int32_t index)
	{
		if (index >= 0 && index < static_cast<int32_t>(m_Frames.size()))
			m_Frames.erase(m_Frames.begin() + index);
	}

	inline void AnimationClip::ClearFrames()
	{
		m_Frames.clear();
	}

	inline const AnimationFrame* AnimationClip::GetFrame(int32_t index) const
	{
		if (index < 0 || index >= static_cast<int32_t>(m_Frames.size()))
			return nullptr;
		return &m_Frames[index];
	}

	inline AnimationFrame* AnimationClip::GetFrame(int32_t index)
	{
		if (index < 0 || index >= static_cast<int32_t>(m_Frames.size()))
			return nullptr;
		return &m_Frames[index];
	}

	inline float AnimationClip::GetFrameStartTime(int32_t frameIndex) const
	{
		float time = 0.0f;
		for (int32_t i = 0; i < frameIndex && i < static_cast<int32_t>(m_Frames.size()); ++i)
			time += m_Frames[i].Duration;
		return time;
	}

	inline int32_t AnimationClip::GetFrameAtTime(float time) const
	{
		if (m_Frames.empty()) return -1;

		float accumulated = 0.0f;
		for (int32_t i = 0; i < static_cast<int32_t>(m_Frames.size()); ++i)
		{
			accumulated += m_Frames[i].Duration;
			if (time < accumulated)
				return i;
		}

		if (m_WrapMode == EAnimationWrapMode::Loop)
			return static_cast<int32_t>(m_Frames.size()) - 1;
		return 0;
	}

	inline Animator::Animator(Ref<AnimationClip> defaultClip)
		: m_CurrentClip(defaultClip)
	{
		if (m_CurrentClip)
			Play(m_CurrentClip);
	}

	inline void Animator::Play(Ref<AnimationClip> clip, float blendTime)
	{
		if (!clip) return;

		if (m_CurrentClip && blendTime > 0.0f)
		{
			m_PreviousClip = m_CurrentClip;
			m_BlendTime = blendTime;
			m_BlendTimer = 0.0f;
			m_BlendFactor = 0.0f;
		}
		else
		{
			m_PreviousClip = nullptr;
			m_BlendFactor = 1.0f;
		}

		m_CurrentClip = clip;
		m_CurrentTime = 0.0f;
		m_CurrentFrameIndex = 0;
		m_IsPlaying = true;
		m_IsPaused = false;
	}

	inline void Animator::Play(const std::string& clipName, float blendTime)
	{
		auto it = m_Clips.find(clipName);
		if (it != m_Clips.end())
			Play(it->second, blendTime);
	}

	inline void Animator::Stop()
	{
		m_IsPlaying = false;
		m_IsPaused = false;
	}

	inline void Animator::Pause()
	{
		if (m_IsPlaying)
			m_IsPaused = true;
	}

	inline void Animator::Resume()
	{
		if (m_IsPlaying && m_IsPaused)
			m_IsPaused = false;
	}

	inline void Animator::Reset()
	{
		m_CurrentTime = 0.0f;
		m_CurrentFrameIndex = 0;
	}

	inline void Animator::SetCurrentTime(float time)
	{
		if (!m_CurrentClip) return;
		m_CurrentTime = glm::clamp(time, 0.0f, m_CurrentClip->GetDuration());
		m_CurrentFrameIndex = m_CurrentClip->GetFrameAtTime(m_CurrentTime);
	}

	inline const AnimationFrame* Animator::GetCurrentFrameData() const
	{
		if (!m_CurrentClip) return nullptr;
		return m_CurrentClip->GetFrame(m_CurrentFrameIndex);
	}

	inline void Animator::SetFrameFinishedCallback(std::function<void()> callback)
	{
		m_FrameFinishedCallback = std::move(callback);
	}

	inline void Animator::SetAnimationFinishedCallback(std::function<void()> callback)
	{
		m_AnimationFinishedCallback = std::move(callback);
	}

	inline void Animator::SetFrameEventCallback(std::function<void(const AnimationFrame&)> callback)
	{
		m_FrameEventCallback = std::move(callback);
	}

	inline int32_t Animator::GetNextFrameIndex(int32_t current) const
	{
		if (!m_CurrentClip) return -1;
		int32_t count = m_CurrentClip->GetFrameCount();
		if (count == 0) return -1;

		switch (m_CurrentClip->GetWrapMode())
		{
		case EAnimationWrapMode::Once:
			return (current + 1 < count) ? current + 1 : -1;
		case EAnimationWrapMode::Loop:
			return (current + 1) % count;
		case EAnimationWrapMode::PingPong:
			return (current + 1 < count) ? current + 1 : current;
		case EAnimationWrapMode::ClampForever:
			return count - 1;
		default:
			return current + 1;
		}
	}

	inline void Animator::AdvanceFrame(float deltaTime)
	{
		if (!m_CurrentClip || !m_IsPlaying || m_IsPaused) return;

		int32_t previousFrame = m_CurrentFrameIndex;
		m_CurrentTime += deltaTime;

		float duration = m_CurrentClip->GetDuration();
		if (duration == 0.0f) return;

		EAnimationWrapMode wrapMode = m_CurrentClip->GetWrapMode();

		switch (wrapMode)
		{
		case EAnimationWrapMode::Once:
			if (m_CurrentTime >= duration)
			{
				m_CurrentTime = duration;
				m_CurrentFrameIndex = m_CurrentClip->GetFrameCount() - 1;
				m_IsPlaying = false;
				if (m_AnimationFinishedCallback)
					m_AnimationFinishedCallback();
			}
			else
			{
				m_CurrentFrameIndex = m_CurrentClip->GetFrameAtTime(m_CurrentTime);
			}
			break;

		case EAnimationWrapMode::Loop:
			m_CurrentTime = std::fmod(m_CurrentTime, duration);
			m_CurrentFrameIndex = m_CurrentClip->GetFrameAtTime(m_CurrentTime);
			break;

		case EAnimationWrapMode::PingPong:
			m_CurrentTime = std::fmod(m_CurrentTime, duration * 2.0f);
			if (m_CurrentTime > duration)
				m_CurrentTime = duration * 2.0f - m_CurrentTime;
			m_CurrentFrameIndex = m_CurrentClip->GetFrameAtTime(m_CurrentTime);
			break;

		case EAnimationWrapMode::ClampForever:
			if (m_CurrentTime >= duration)
				m_CurrentTime = duration;
			m_CurrentFrameIndex = m_CurrentClip->GetFrameCount() - 1;
			break;
		}

		if (previousFrame != m_CurrentFrameIndex)
		{
			if (m_FrameFinishedCallback)
				m_FrameFinishedCallback();

			if (m_FrameEventCallback)
			{
				auto* frameData = m_CurrentClip->GetFrame(m_CurrentFrameIndex);
				if (frameData && !frameData->EventName.empty())
					m_FrameEventCallback(*frameData);
			}
		}
	}

	inline void Animator::Update(float deltaTime)
	{
		if (IsBlending())
		{
			m_BlendTimer += deltaTime;
			m_BlendFactor = glm::clamp(m_BlendTimer / m_BlendTime, 0.0f, 1.0f);

			if (m_BlendFactor >= 1.0f)
			{
				m_PreviousClip = nullptr;
				m_BlendFactor = 1.0f;
			}
		}

		AdvanceFrame(deltaTime);
	}

	inline AnimationStateMachine::AnimationStateMachine()
	{
	}

	inline void AnimationStateMachine::AddState(const std::string& name, Ref<AnimationClip> clip, bool isEntry)
	{
		State state;
		state.Name = name;
		state.Clip = clip;
		state.IsEntryState = isEntry;
		m_States[name] = state;

		if (isEntry)
			m_EntryStateName = name;
	}

	inline void AnimationStateMachine::RemoveState(const std::string& name)
	{
		m_States.erase(name);
		if (m_CurrentStateName == name)
			m_CurrentStateName.clear();
		if (m_EntryStateName == name)
			m_EntryStateName.clear();
	}

	inline AnimationStateMachine::State* AnimationStateMachine::GetState(const std::string& name)
	{
		auto it = m_States.find(name);
		return it != m_States.end() ? &it->second : nullptr;
	}

	inline void AnimationStateMachine::AddTransition(const std::string& from, const std::string& to,
		float duration, bool hasExitTime, float exitTime)
	{
		auto* state = GetState(from);
		if (!state) return;

		StateTransition transition;
		transition.TargetState = to;
		transition.Duration = duration;
		transition.HasExitTime = hasExitTime;
		transition.ExitTime = exitTime;
		state->Transitions.push_back(transition);
	}

	inline void AnimationStateMachine::AddTransition(const std::string& from, const std::string& to,
		std::function<bool()> condition, float duration)
	{
		auto* state = GetState(from);
		if (!state) return;

		StateTransition transition;
		transition.TargetState = to;
		transition.Duration = duration;
		transition.Condition = condition;
		state->Transitions.push_back(transition);
	}

	inline void AnimationStateMachine::RemoveTransition(const std::string& from, const std::string& to)
	{
		auto* state = GetState(from);
		if (!state) return;

		state->Transitions.erase(
			std::remove_if(state->Transitions.begin(), state->Transitions.end(),
				[&to](const StateTransition& t) { return t.TargetState == to; }),
			state->Transitions.end()
		);
	}

	inline void AnimationStateMachine::SetEntryState(const std::string& name)
	{
		m_EntryStateName = name;
		auto* state = GetState(name);
		if (state)
			state->IsEntryState = true;
	}

	inline void AnimationStateMachine::ForceState(const std::string& name)
	{
		auto* state = GetState(name);
		if (!state) return;

		m_PreviousStateName = m_CurrentStateName;
		m_CurrentStateName = name;
		m_StateTime = 0.0f;
		m_Transitioning = false;
		m_TransitionProgress = 1.0f;
	}

	inline void AnimationStateMachine::Update(Animator& animator, float deltaTime)
	{
		if (m_EntryStateName.empty() && !m_CurrentStateName.empty())
		{
			m_CurrentStateName = m_EntryStateName;
		}

		auto* currentState = GetCurrentState();
		if (!currentState || !currentState->Clip)
			return;

		if (!animator.IsPlaying() || animator.GetCurrentClip() != currentState->Clip)
		{
			animator.Play(currentState->Clip);
		}

		if (m_Transitioning && m_PendingTransition)
		{
			m_TransitionProgress += deltaTime / m_PendingTransition->Duration;
			if (m_TransitionProgress >= 1.0f)
			{
				m_Transitioning = false;
				m_PreviousStateName = m_CurrentStateName;
				m_CurrentStateName = m_PendingTransition->TargetState;
				m_StateTime = 0.0f;

				auto* newState = GetState(m_CurrentStateName);
				if (newState && newState->Clip)
					animator.Play(newState->Clip, 0.0f);
			}
		}
		else
		{
			m_StateTime += deltaTime;
			EvaluateTransitions();
		}

		animator.Update(deltaTime);
	}

	inline AnimationStateMachine::State* AnimationStateMachine::GetCurrentState()
	{
		if (m_CurrentStateName.empty())
			return nullptr;
		return GetState(m_CurrentStateName);
	}

	inline const AnimationStateMachine::State* AnimationStateMachine::GetCurrentState() const
	{
		if (m_CurrentStateName.empty())
			return nullptr;
		auto it = m_States.find(m_CurrentStateName);
		return it != m_States.end() ? &it->second : nullptr;
	}

	inline void AnimationStateMachine::EvaluateTransitions()
	{
		auto* currentState = GetCurrentState();
		if (!currentState) return;

		for (auto& transition : currentState->Transitions)
		{
			bool canTransition = false;

			if (transition.Condition)
			{
				canTransition = transition.Condition();
			}
			else if (transition.HasExitTime && currentState->Clip)
			{
				float normalizedTime = m_StateTime / currentState->Clip->GetDuration();
				canTransition = normalizedTime >= transition.ExitTime;
			}
			else
			{
				canTransition = true;
			}

			if (canTransition)
			{
				m_Transitioning = true;
				m_PendingTransition = &transition;
				m_TransitionProgress = 0.0f;
				break;
			}
		}
	}
}
