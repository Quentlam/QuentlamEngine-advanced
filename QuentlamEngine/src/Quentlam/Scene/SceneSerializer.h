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

	private:
		Ref<Scene> m_Scene;
	};
}
