#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/World/TileMap.h"
#include <string>
#include <vector>

namespace Quentlam
{
	class TileMapSerializer
	{
	public:
		explicit TileMapSerializer(TileMap* tileMap);

		bool Serialize(const std::string& filepath) const;
		bool Deserialize(const std::string& filepath);

		bool SerializeToString(std::string* output) const;
		bool DeserializeFromString(const std::string& data);

		static std::string GetFileExtension() { return ".json"; }
		static std::string GetFileFilter() { return "TileMap JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0"; }

	private:
		TileMap* m_TileMap = nullptr;
	};
}
