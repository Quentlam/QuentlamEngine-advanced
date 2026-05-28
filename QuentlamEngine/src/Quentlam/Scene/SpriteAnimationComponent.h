#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Renderer/SubTexture2D.h"
#include <string>

namespace Quentlam
{
	class SpriteAnimationComponent
	{
	public:
		Ref<Animator> Animator;
		Ref<Texture2D> AtlasTexture;
		Ref<SpriteAtlasBinding> AtlasBinding;

		std::string DefaultClipName;

		bool AutoPlay = true;
		bool IsPlaying() const { return Animator && Animator->IsPlaying(); }

		void Play(const std::string& clipName)
		{
			if (Animator)
				Animator->Play(clipName);
		}

		void Stop()
		{
			if (Animator)
				Animator->Stop();
		}

		void Pause()
		{
			if (Animator)
				Animator->Pause();
		}

		void Resume()
		{
			if (Animator)
				Animator->Resume();
		}

		Ref<AnimationClip> GetCurrentClip() const
		{
			return Animator ? Animator->GetCurrentClip() : nullptr;
		}

		int32_t GetCurrentFrameIndex() const
		{
			return Animator ? Animator->GetCurrentFrame() : -1;
		}

		Ref<SubTexture2D> GetCurrentSubTexture() const
		{
			if (!Animator || !AtlasBinding || !GetCurrentClip() || !AtlasTexture)
				return nullptr;

			const auto* frameData = Animator->GetCurrentFrameData();
			if (!frameData)
				return nullptr;

			int32_t spriteIdx = frameData->SpriteIndex;
			int32_t col = spriteIdx % AtlasBinding->AtlasColumns;
			int32_t row = spriteIdx / AtlasBinding->AtlasColumns;

			return SubTexture2D::CreateFromCoords(
				AtlasTexture,
				{ static_cast<float>(col), static_cast<float>(row) },
				{ 1.0f, 1.0f },
				{ 1, 1 }
			);
		}

		SpriteAnimationComponent() = default;
		SpriteAnimationComponent(const SpriteAnimationComponent&) = default;
	};
}
