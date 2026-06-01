#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Scene.h"
#include <string>

namespace Quentlam
{
	class SceneSerializer
	{
	public:
		SceneSerializer(Ref<Scene> scene);
		SceneSerializer(Scene* scene);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		std::string SerializeToString();
		bool DeserializeFromString(const std::string& data);

		Entity DeserializeEntity(Scene* scene, const std::string& entityJson);

	private:
		Ref<Scene> m_Scene;
		Scene* m_DeserializationTarget = nullptr;
		entt::entity m_CurrentEntity = entt::null;

		std::string WriteIndent(int depth);

		bool ParseJSON(const std::string& data, size_t& pos);
		bool ParseObject(const std::string& data, size_t& pos);
		bool ParseArray(const std::string& data, size_t& pos);
		void ParseComponents(const std::string& data, size_t& pos);
		void ParseComponentData(const std::string& data, size_t& pos, const std::string& compName);

		std::string ParseString(const std::string& data, size_t& pos);
		float ParseNumber(const std::string& data, size_t& pos);
		glm::vec3 ParseVec3(const std::string& data, size_t& pos);
		bool ParseBool(const std::string& data, size_t& pos);
		bool ParseValue(const std::string& data, size_t& pos);
		void SkipObject(const std::string& data, size_t& pos);
		void SkipArray(const std::string& data, size_t& pos);
		void SkipComma(const std::string& data, size_t& pos);
		void SkipWhitespace(const std::string& data, size_t& pos);
	};
}
