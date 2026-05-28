#include "qlpch.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include "Quentlam/Core/Log.h"
#include <fstream>

namespace Quentlam
{
	SceneSerializer::SceneSerializer(Ref<Scene> scene)
		: m_Scene(scene)
	{
	}

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		std::string data = SerializeToString();
		std::ofstream file(filepath);
		if (file.is_open())
		{
			file << data;
			file.close();
			QL_CORE_INFO("Scene serialized to: {0}", filepath);
		}
		else
		{
			QL_CORE_ERROR("Failed to open file for writing: {0}", filepath);
		}
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			QL_CORE_ERROR("Failed to open file for reading: {0}", filepath);
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string data = buffer.str();
		file.close();

		return DeserializeFromString(data);
	}

	std::string SceneSerializer::SerializeToString()
	{
		std::stringstream ss;
		ss << "// Quentlam Scene File\n";
		ss << "// Auto-generated\n";
		return ss.str();
	}

	bool SceneSerializer::DeserializeFromString(const std::string& data)
	{
		if (data.empty())
			return false;

		QL_CORE_TRACE("Scene deserialized from string ({0} chars)", data.size());
		return true;
	}
}
