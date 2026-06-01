#include "qlpch.h"
#include "AnimationLoader.h"
#include "AnimationModule.h"
#include "Quentlam/Renderer/Texture.h"
#include "Quentlam/Resource/ResourceManager.h"
#include <fstream>
#include <rapidjson/document.h>
#include <sstream>

namespace Quentlam
{

	bool AnimationLoader::LoadAnimationData(
		const std::string& filepath,
		Ref<Texture2D>& outAtlasTexture,
		Ref<SpriteAtlasBinding>& outAtlasBinding,
		std::vector<Ref<AnimationClip>>& outClips)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			QL_CORE_ERROR("AnimationLoader: Could not open file '{0}'", filepath);
			return false;
		}

		std::stringstream ss;
		ss << file.rdbuf();
		std::string content = ss.str();
		file.close();

		rapidjson::Document doc;
		doc.Parse(content.c_str());
		if (doc.HasParseError())
		{
			QL_CORE_ERROR("AnimationLoader: JSON parse error in '{0}'", filepath);
			return false;
		}

		std::string atlasPath;
		int columns = 1, rows = 1;
		glm::ivec2 frameSize = { 32, 32 };

		if (doc.HasMember("atlas"))
		{
			const rapidjson::Value& atlasVal = doc["atlas"];
			if (atlasVal.IsString())
			{
				atlasPath = atlasVal.GetString();
			}
			else if (atlasVal.IsObject())
			{
				if (atlasVal.HasMember("path") && atlasVal["path"].IsString())
					atlasPath = atlasVal["path"].GetString();
				if (atlasVal.HasMember("columns") && atlasVal["columns"].IsInt())
					columns = atlasVal["columns"].GetInt();
				if (atlasVal.HasMember("rows") && atlasVal["rows"].IsInt())
					rows = atlasVal["rows"].GetInt();
				if (atlasVal.HasMember("frameWidth") && atlasVal["frameWidth"].IsInt())
					frameSize.x = atlasVal["frameWidth"].GetInt();
				if (atlasVal.HasMember("frameHeight") && atlasVal["frameHeight"].IsInt())
					frameSize.y = atlasVal["frameHeight"].GetInt();
			}
		}

		if (doc.HasMember("columns") && doc["columns"].IsInt())
			columns = doc["columns"].GetInt();
		if (doc.HasMember("rows") && doc["rows"].IsInt())
			rows = doc["rows"].GetInt();
		if (doc.HasMember("frameWidth") && doc["frameWidth"].IsInt())
			frameSize.x = doc["frameWidth"].GetInt();
		if (doc.HasMember("frameHeight") && doc["frameHeight"].IsInt())
			frameSize.y = doc["frameHeight"].GetInt();

		if (!atlasPath.empty())
		{
			outAtlasTexture = ResourceManager::Load<Texture2D>(atlasPath, atlasPath);
		}
		else
		{
			outAtlasTexture = nullptr;
		}

		outAtlasBinding = CreateRef<SpriteAtlasBinding>();
		outAtlasBinding->AtlasPath = atlasPath;
		outAtlasBinding->AtlasColumns = columns;
		outAtlasBinding->AtlasRows = rows;
		outAtlasBinding->DefaultFrameSize = frameSize;

		outClips.clear();
		if (doc.HasMember("clips") && doc["clips"].IsArray())
		{
			const auto& clips = doc["clips"].GetArray();
			for (const auto& c : clips)
			{
				if (c.HasMember("name"))
				{
					std::string clipName = c["name"].GetString();
					outClips.push_back(LoadClipFromJson(clipName, c, outAtlasBinding));
				}
			}
		}

		return true;
	}

} // namespace Quentlam
