#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Renderer/Texture.h"
#include <string>
#include <vector>
#include <rapidjson/document.h>

namespace Quentlam
{
	class QUENTLAM_API AnimationLoader
	{
	public:
		static bool LoadAnimationData(const std::string& filepath,
			Ref<Texture2D>& outAtlasTexture,
			Ref<SpriteAtlasBinding>& outAtlasBinding,
			std::vector<Ref<AnimationClip>>& outClips);

		static Ref<AnimationClip> LoadClipFromJson(
			const std::string& name,
			const rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>& data,
			Ref<SpriteAtlasBinding> atlas)
		{
			float fps = 12.0f;
			if (data.HasMember("fps") && data["fps"].IsNumber())
				fps = data["fps"].GetFloat();

			auto clip = CreateRef<AnimationClip>(name, atlas, fps);

			if (data.HasMember("wrapMode") && data["wrapMode"].IsString())
			{
				std::string mode = data["wrapMode"].GetString();
				if (mode == "Once") clip->SetWrapMode(EAnimationWrapMode::Once);
				else if (mode == "PingPong") clip->SetWrapMode(EAnimationWrapMode::PingPong);
				else if (mode == "ClampForever") clip->SetWrapMode(EAnimationWrapMode::ClampForever);
				else clip->SetWrapMode(EAnimationWrapMode::Loop);
			}

			if (data.HasMember("frames") && data["frames"].IsArray())
			{
				const auto& frames = data["frames"].GetArray();
				for (const auto& f : frames)
				{
					if (f.IsInt())
					{
						AnimationFrame frame;
						frame.SpriteIndex = f.GetInt();
						frame.Duration = 1.0f / fps;
						clip->AddFrame(frame);
					}
					else if (f.IsObject())
					{
						AnimationFrame frame;
						if (f.HasMember("spriteIndex") && f["spriteIndex"].IsInt())
							frame.SpriteIndex = f["spriteIndex"].GetInt();
						if (f.HasMember("duration") && f["duration"].IsNumber())
							frame.Duration = f["duration"].GetFloat();
						if (f.HasMember("event") && f["event"].IsString())
							frame.EventName = f["event"].GetString();
						clip->AddFrame(frame);
					}
				}
			}

			return clip;
		}
	};
}
