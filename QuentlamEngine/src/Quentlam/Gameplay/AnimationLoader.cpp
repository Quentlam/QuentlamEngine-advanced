#include "qlpch.h"
#include "AnimationLoader.h"
#include "AnimationModule.h"
#include "Quentlam/Renderer/Texture.h"
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
		if (doc.HasMember("atlas") && doc["atlas"].IsString())
			atlasPath = doc["atlas"].GetString();

		int columns = 1, rows = 1;
		if (doc.HasMember("columns") && doc["columns"].IsInt())
			columns = doc["columns"].GetInt();
		if (doc.HasMember("rows") && doc["rows"].IsInt())
			rows = doc["rows"].GetInt();

		outAtlasTexture = Texture2D::Create(atlasPath);

		outAtlasBinding = CreateRef<SpriteAtlasBinding>();
		outAtlasBinding->AtlasPath = atlasPath;
		outAtlasBinding->AtlasColumns = columns;
		outAtlasBinding->AtlasRows = rows;
		outAtlasBinding->DefaultFrameSize = { 32, 32 };

		if (doc.HasMember("frameWidth") && doc["frameWidth"].IsInt())
			outAtlasBinding->DefaultFrameSize.x = doc["frameWidth"].GetInt();
		if (doc.HasMember("frameHeight") && doc["frameHeight"].IsInt())
			outAtlasBinding->DefaultFrameSize.y = doc["frameHeight"].GetInt();

		outClips.clear();
		if (doc.HasMember("clips") && doc["clips"].IsArray())
		{
			const auto& clips = doc["clips"].GetArray();
			for (const auto& c : clips)
			{
				if (c.HasMember("name") && c.HasMember("frames"))
				{
					std::string clipName = c["name"].GetString();
					outClips.push_back(LoadClipFromJson(clipName, c, outAtlasBinding));
				}
			}
		}

		return true;
	}

}
