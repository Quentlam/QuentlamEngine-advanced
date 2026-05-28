#include "qlpch.h"
#include "TileMapSerializer.h"
#include "Quentlam/Core/Log.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace rapidjson;

namespace Quentlam
{
	TileMapSerializer::TileMapSerializer(TileMap* tileMap)
		: m_TileMap(tileMap)
	{
	}

	bool TileMapSerializer::Serialize(const std::string& filepath) const
	{
		if (!m_TileMap) return false;

		std::string json;
		if (!SerializeToString(&json))
			return false;

		std::ofstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			QL_CORE_ERROR("TileMapSerializer: Failed to open file for writing: {0}", filepath);
			return false;
		}

		file.write(json.c_str(), static_cast<std::streamsize>(json.size()));
		file.close();

		QL_CORE_INFO("TileMapSerializer: Saved map '{0}' ({1}x{2}) to {3}",
			m_TileMap->GetName(),
			m_TileMap->GetMapSize().x,
			m_TileMap->GetMapSize().y,
			filepath);

		return true;
	}

	bool TileMapSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			QL_CORE_ERROR("TileMapSerializer: Failed to open file for reading: {0}", filepath);
			return false;
		}

		file.seekg(0, std::ios::end);
		size_t size = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		std::string data(size, '\0');
		file.read(data.data(), static_cast<std::streamsize>(size));
		file.close();

		return DeserializeFromString(data);
	}

	bool TileMapSerializer::SerializeToString(std::string* output) const
	{
		if (!m_TileMap || !output) return false;

		StringBuffer sb;
		Writer<StringBuffer> writer(sb);

		writer.StartObject();

		writer.Key("mapId");
		writer.String(m_TileMap->GetMapId().empty() ? "unknown" : m_TileMap->GetMapId().c_str());

		writer.Key("name");
		writer.String(m_TileMap->GetName().c_str());

		writer.Key("width");
		writer.Int(m_TileMap->GetMapSize().x);

		writer.Key("height");
		writer.Int(m_TileMap->GetMapSize().y);

		writer.Key("tileSize");
		writer.Double(m_TileMap->GetTileSize());

		writer.Key("version");
		writer.Int(m_TileMap->GetVersion());

		writer.Key("chunks");
		writer.StartArray();
		{
			const auto& chunks = m_TileMap->GetAllChunks();
			for (const auto& [coord, chunk] : chunks)
			{
				if (!chunk) continue;

				writer.StartObject();
				writer.Key("cx");
				writer.Int(coord.x);
				writer.Key("cy");
				writer.Int(coord.y);

				writer.Key("tiles");
				writer.StartArray();
				{
					for (int ly = 0; ly < Chunk::Size; ++ly)
					{
						for (int lx = 0; lx < Chunk::Size; ++lx)
						{
							const Tile* tile = chunk->GetTile(lx, ly);
							if (!tile) continue;

							glm::ivec2 worldTile(coord.x * Chunk::Size + lx, coord.y * Chunk::Size + ly);
							if (!m_TileMap->IsValidPosition(worldTile)) continue;

							if (tile->IsAllEmpty() && !tile->IsHighlighted)
								continue;

							bool hasLayerData = false;
							for (size_t li = 0; li < static_cast<size_t>(ETileLayer::Count); ++li)
							{
								if (!tile->Layers[li].IsActive()) continue;
								hasLayerData = true;
								break;
							}
							if (!hasLayerData && !tile->IsHighlighted)
								continue;

							writer.StartObject();
							writer.Key("x");
							writer.Int(lx);
							writer.Key("y");
							writer.Int(ly);

							bool wroteLayer = false;
							for (size_t li = 0; li < static_cast<size_t>(ETileLayer::Count); ++li)
							{
								const TileLayerData& layer = tile->Layers[li];
								if (!layer.IsActive()) continue;

								const char* layerName = nullptr;
								switch (static_cast<ETileLayer>(li))
								{
								case ETileLayer::Ground:   layerName = "g"; break;
								case ETileLayer::Objects:  layerName = "o"; break;
								case ETileLayer::Water:   layerName = "w"; break;
								case ETileLayer::Weather:  layerName = "r"; break;
								default: continue;
								}

								writer.Key(layerName);
								writer.StartObject();
								writer.Key("t");
								writer.Int(static_cast<int>(layer.Type));
								if (layer.Variant != 0)
								{
									writer.Key("v");
									writer.Int(layer.Variant);
								}
								if (layer.Variation != 0)
								{
									writer.Key("x");
									writer.Int(layer.Variation);
								}
								writer.EndObject();
								wroteLayer = true;
							}

							if (tile->IsHighlighted)
							{
								writer.Key("h");
								writer.Bool(true);
								wroteLayer = true;
							}

							writer.EndObject();
							(void)wroteLayer;
						}
					}
				}
				writer.EndArray();
				writer.EndObject();
			}
		}
		writer.EndArray();

		writer.Key("regions");
		writer.StartArray();
		{
			for (const auto& region : m_TileMap->GetRegions())
			{
				writer.StartObject();
				writer.Key("name");
				writer.String(region.GetName().c_str());
				writer.Key("type");
				writer.Int(static_cast<int>(region.GetRegionType()));
				writer.Key("minX");
				writer.Int(region.GetBoundsMin().x);
				writer.Key("minY");
				writer.Int(region.GetBoundsMin().y);
				writer.Key("maxX");
				writer.Int(region.GetBoundsMax().x);
				writer.Key("maxY");
				writer.Int(region.GetBoundsMax().y);
				writer.EndObject();
			}
		}
		writer.EndArray();

		writer.Key("portals");
		writer.StartArray();
		{
			for (const auto& [pos, portal] : m_TileMap->GetPortals())
			{
				writer.StartObject();
				writer.Key("x");
				writer.Int(pos.x);
				writer.Key("y");
				writer.Int(pos.y);
				writer.Key("targetMap");
				writer.String(portal.GetTargetMap().c_str());
				writer.Key("targetX");
				writer.Int(portal.GetTargetPos().x);
				writer.Key("targetY");
				writer.Int(portal.GetTargetPos().y);
				writer.Key("id");
				writer.String(portal.GetId().c_str());
				writer.EndObject();
			}
		}
		writer.EndArray();

		writer.EndObject();

		*output = sb.GetString();
		return true;
	}

	bool TileMapSerializer::DeserializeFromString(const std::string& data)
	{
		if (!m_TileMap) return false;
		if (data.empty()) return false;

		Document doc;
		doc.Parse(data.c_str(), data.size());
		if (doc.HasParseError())
		{
			QL_CORE_ERROR("TileMapSerializer: JSON parse error at offset {0}", doc.GetErrorOffset());
			return false;
		}

		if (!doc.IsObject())
		{
			QL_CORE_ERROR("TileMapSerializer: Root must be an object");
			return false;
		}

		m_TileMap->Clear();

		if (doc.HasMember("mapId") && doc["mapId"].IsString())
			m_TileMap->SetMapId(doc["mapId"].GetString());

		std::string name = doc.HasMember("name") && doc["name"].IsString()
			? doc["name"].GetString() : "UnnamedMap";
		m_TileMap->SetName(name);

		int width = doc.HasMember("width") && doc["width"].IsInt()
			? doc["width"].GetInt() : 100;
		int height = doc.HasMember("height") && doc["height"].IsInt()
			? doc["height"].GetInt() : 100;
		float tileSize = doc.HasMember("tileSize") && doc["tileSize"].IsFloat()
			? doc["tileSize"].GetFloat() : 1.0f;

		m_TileMap->Resize(glm::ivec2(width, height));
		m_TileMap->SetTileSize(tileSize);

		if (doc.HasMember("version") && doc["version"].IsInt())
			m_TileMap->SetVersion(doc["version"].GetInt());

		if (doc.HasMember("chunks") && doc["chunks"].IsArray())
	{
		for (const Value& chunkVal : doc["chunks"].GetArray())
		{
			if (!chunkVal.IsObject()) continue;

			int cx = chunkVal.HasMember("cx") ? chunkVal["cx"].GetInt() : 0;
			int cy = chunkVal.HasMember("cy") ? chunkVal["cy"].GetInt() : 0;

			glm::ivec2 chunkCoord(cx, cy);
			m_TileMap->EnsureChunkExists(chunkCoord);
			Chunk* chunk = m_TileMap->GetChunk(chunkCoord);
			if (!chunk) continue;

			if (chunkVal.HasMember("tiles") && chunkVal["tiles"].IsArray())
			{
				for (const Value& tileVal : chunkVal["tiles"].GetArray())
				{
					if (!tileVal.IsObject()) continue;

					int lx = tileVal.HasMember("x") ? tileVal["x"].GetInt() : 0;
					int ly = tileVal.HasMember("y") ? tileVal["y"].GetInt() : 0;

					Tile tile;
					const char* layerKeys[] = { "g", "o", "w", "r" };
					ETileLayer layerEnums[] = { ETileLayer::Ground, ETileLayer::Objects, ETileLayer::Water, ETileLayer::Weather };

					for (size_t li = 0; li < 4; ++li)
					{
						if (!tileVal.HasMember(layerKeys[li]) || !tileVal[layerKeys[li]].IsObject()) continue;

						const Value& layerVal = tileVal[layerKeys[li]];
						tile[layerEnums[li]].Type = static_cast<ETileType>(
							layerVal.HasMember("t") ? layerVal["t"].GetInt() : 0);
						tile[layerEnums[li]].Variant = layerVal.HasMember("v") ? layerVal["v"].GetInt() : 0;
						tile[layerEnums[li]].Variation = layerVal.HasMember("x") ?
							static_cast<uint8_t>(layerVal["x"].GetInt()) : 0;
					}

					if (tileVal.HasMember("h"))
						tile.IsHighlighted = tileVal["h"].GetBool();

					chunk->SetTile(lx, ly, tile);
				}
			}
		}
	}

		if (doc.HasMember("regions") && doc["regions"].IsArray())
		{
			for (const Value& regionVal : doc["regions"].GetArray())
			{
				if (!regionVal.IsObject()) continue;

				std::string rname = regionVal.HasMember("name") ? regionVal["name"].GetString() : "Region";
				int minX = regionVal.HasMember("minX") ? regionVal["minX"].GetInt() : 0;
				int minY = regionVal.HasMember("minY") ? regionVal["minY"].GetInt() : 0;
				int maxX = regionVal.HasMember("maxX") ? regionVal["maxX"].GetInt() : 0;
				int maxY = regionVal.HasMember("maxY") ? regionVal["maxY"].GetInt() : 0;

				Region region(rname, { minX, minY }, { maxX, maxY });
				if (regionVal.HasMember("type"))
					region.SetRegionType(static_cast<ERegionType>(regionVal["type"].GetInt()));

				m_TileMap->AddRegion(region);
			}
		}

		if (doc.HasMember("portals") && doc["portals"].IsArray())
		{
			for (const Value& portalVal : doc["portals"].GetArray())
			{
				if (!portalVal.IsObject()) continue;

				int px = portalVal.HasMember("x") ? portalVal["x"].GetInt() : 0;
				int py = portalVal.HasMember("y") ? portalVal["y"].GetInt() : 0;
				std::string targetMap = portalVal.HasMember("targetMap") ? portalVal["targetMap"].GetString() : "";
				int targetX = portalVal.HasMember("targetX") ? portalVal["targetX"].GetInt() : 0;
				int targetY = portalVal.HasMember("targetY") ? portalVal["targetY"].GetInt() : 0;
				std::string pid = portalVal.HasMember("id") ? portalVal["id"].GetString() : "";

				Portal portal(targetMap, { targetX, targetY }, pid);
				m_TileMap->AddPortal({ px, py }, portal);
			}
		}

		QL_CORE_INFO("TileMapSerializer: Loaded map '{0}' ({1}x{2})",
			m_TileMap->GetName(),
			m_TileMap->GetMapSize().x,
			m_TileMap->GetMapSize().y);

		return true;
	}
}
