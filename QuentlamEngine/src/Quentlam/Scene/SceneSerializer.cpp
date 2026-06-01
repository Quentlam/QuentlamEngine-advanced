#include "qlpch.h"
#include "Quentlam/Scene/SceneSerializer.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/Scene/SpriteRendererComponent.h"
#include "Quentlam/Scene/Entity.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>

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

	std::string SceneSerializer::WriteIndent(int depth)
	{
		return std::string(depth * 2, ' ');
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
		ss << "{\n";
		ss << WriteIndent(1) << "\"entities\": [\n";

		auto& registry = (m_DeserializationTarget ? m_DeserializationTarget : m_Scene.get())->GetRegistry();
		bool firstEntity = true;

		for (auto entity : registry.view<TagComponent>())
		{
			if (!firstEntity)
				ss << ",\n";
			firstEntity = false;

			ss << WriteIndent(2) << "{\n";
			ss << WriteIndent(3) << "\"entity_id\": " << static_cast<uint32_t>(entity) << ",\n";

			auto& tag = registry.get<TagComponent>(entity);
			ss << WriteIndent(3) << "\"tag\": \"" << tag.Tag << "\",\n";

			ss << WriteIndent(3) << "\"components\": {\n";

			bool firstComp = true;
			auto writeCompHeader = [&](const char* name) {
				if (!firstComp) ss << ",\n";
				firstComp = false;
				ss << WriteIndent(4) << "\"" << name << "\": ";
			};

			if (registry.any_of<TransformComponent>(entity))
			{
				auto& tc = registry.get<TransformComponent>(entity);
				writeCompHeader("TransformComponent");
				ss << "{ \"matrix\": [";
				for (int c = 0; c < 4; c++)
				{
					ss << "[";
					for (int r = 0; r < 4; r++)
					{
						ss << tc.Transform[c][r];
						if (r < 3) ss << ", ";
					}
					ss << "]";
					if (c < 3) ss << ", ";
				}
				ss << "] }";
			}

			if (registry.any_of<SpriteRendererComponent>(entity))
			{
				auto& src = registry.get<SpriteRendererComponent>(entity);
				writeCompHeader("SpriteRendererComponent");
				ss << "{ \"color\": [" << src.Color.r << ", " << src.Color.g << ", " << src.Color.b << ", " << src.Color.a << "], ";
				ss << "\"size\": [" << src.Size.x << ", " << src.Size.y << "], ";
				ss << "\"tiling_factor\": " << src.TilingFactor << ", ";
				ss << "\"sorting_order\": " << src.SortingOrder << ", ";
				ss << "\"flip_x\": " << (src.FlipX ? "true" : "false") << ", ";
				ss << "\"flip_y\": " << (src.FlipY ? "true" : "false") << " }";
			}

			if (registry.any_of<SpriteTransformComponent>(entity))
			{
				auto& stc = registry.get<SpriteTransformComponent>(entity);
				writeCompHeader("SpriteTransformComponent");
				ss << "{ \"color\": [" << stc.Color.r << ", " << stc.Color.g << ", " << stc.Color.b << ", " << stc.Color.a << "] }";
			}

			if (registry.any_of<TriangleRendererComponent>(entity))
			{
				auto& trc = registry.get<TriangleRendererComponent>(entity);
				writeCompHeader("TriangleRendererComponent");
				ss << "{ \"color\": [" << trc.Color.r << ", " << trc.Color.g << ", " << trc.Color.b << ", " << trc.Color.a << "] }";
			}

			if (registry.any_of<DirectionalLightComponent>(entity))
			{
				auto& dlc = registry.get<DirectionalLightComponent>(entity);
				writeCompHeader("DirectionalLightComponent");
				ss << "{ \"color\": [" << dlc.Color.r << ", " << dlc.Color.g << ", " << dlc.Color.b << "], ";
				ss << "\"intensity\": " << dlc.Intensity << ", ";
				ss << "\"direction\": [" << dlc.Direction.x << ", " << dlc.Direction.y << ", " << dlc.Direction.z << "] }";
			}

			if (registry.any_of<SceneGroupComponent>(entity))
			{
				auto& grp = registry.get<SceneGroupComponent>(entity);
				writeCompHeader("SceneGroupComponent");
				ss << "{ \"expanded\": " << (grp.Expanded ? "true" : "false") << " }";
			}

			if (registry.any_of<WeatherSystemComponent>(entity))
			{
				auto& wsc = registry.get<WeatherSystemComponent>(entity);
				writeCompHeader("WeatherSystemComponent");
				ss << "{ \"time_of_day\": " << wsc.TimeOfDay << ", ";
				ss << "\"weather_intensity\": " << wsc.WeatherIntensity << ", ";
				ss << "\"rain_intensity\": " << wsc.RainIntensity << ", ";
				ss << "\"enabled\": " << (wsc.Enabled ? "true" : "false") << " }";
			}

			if (registry.any_of<SkyboxComponent>(entity))
			{
				auto& sky = registry.get<SkyboxComponent>(entity);
				writeCompHeader("SkyboxComponent");
				ss << "{ \"visible\": " << (sky.Visible ? "true" : "false") << ", ";
				ss << "\"mode\": " << static_cast<int>(sky.Mode) << " }";
			}

			if (registry.any_of<CubeRendererComponent>(entity))
			{
				auto& crc = registry.get<CubeRendererComponent>(entity);
				writeCompHeader("CubeRendererComponent");
				ss << "{ \"color\": [" << crc.Color.r << ", " << crc.Color.g << ", " << crc.Color.b << ", " << crc.Color.a << "], ";
				ss << "\"ambient\": " << crc.AmbientStrength << ", ";
				ss << "\"diffuse\": " << crc.DiffuseStrength << ", ";
				ss << "\"specular\": " << crc.SpecularStrength << ", ";
				ss << "\"shininess\": " << crc.Shininess << " }";
			}

			if (registry.any_of<LuaScriptComponent>(entity))
			{
				auto& lc = registry.get<LuaScriptComponent>(entity);
				writeCompHeader("LuaScriptComponent");
				ss << "{ \"script_path\": \"" << lc.ScriptPath << "\" }";
			}

			if (registry.any_of<Rigidbody2DComponent>(entity))
			{
				auto& rb = registry.get<Rigidbody2DComponent>(entity);
				writeCompHeader("Rigidbody2DComponent");
				const char* typeStr = rb.Type == Rigidbody2DComponent::BodyType::Static ? "Static"
					: rb.Type == Rigidbody2DComponent::BodyType::Dynamic ? "Dynamic" : "Kinematic";
				ss << "{ \"type\": \"" << typeStr << "\", ";
				ss << "\"fixed_rotation\": " << (rb.FixedRotation ? "true" : "false") << ", ";
				ss << "\"gravity_scale\": " << rb.GravityScale << " }";
			}

			if (registry.any_of<BoxCollider2DComponent>(entity))
			{
				auto& bc = registry.get<BoxCollider2DComponent>(entity);
				writeCompHeader("BoxCollider2DComponent");
				ss << "{ \"offset\": [" << bc.Offset.x << ", " << bc.Offset.y << "], ";
				ss << "\"size\": [" << bc.Size.x << ", " << bc.Size.y << "], ";
				ss << "\"density\": " << bc.Density << ", ";
				ss << "\"friction\": " << bc.Friction << ", ";
				ss << "\"restitution\": " << bc.Restitution << ", ";
				ss << "\"show_collider\": " << (bc.ShowCollider ? "true" : "false") << " }";
			}

			if (registry.any_of<UIEntityComponent>(entity))
			{
				auto& ui = registry.get<UIEntityComponent>(entity);
				writeCompHeader("UIEntityComponent");
				ss << "{ \"screen_id\": \"" << ui.ScreenId << "\", ";
				ss << "\"visible\": " << (ui.Visible ? "true" : "false") << ", ";
				ss << "\"blocking\": " << (ui.Blocking ? "true" : "false") << ", ";
				ss << "\"focusable\": " << (ui.Focusable ? "true" : "false") << ", ";
				ss << "\"z_order\": " << ui.ZOrder << ", ";
				ss << "\"input_context\": " << static_cast<int>(ui.InputContext) << " }";
			}

			if (registry.any_of<CameraFollowComponent>(entity))
			{
				auto& cf = registry.get<CameraFollowComponent>(entity);
				writeCompHeader("CameraFollowComponent");
				ss << "{ \"offset\": [" << cf.Offset.x << ", " << cf.Offset.y << "], ";
				ss << "\"smoothing\": " << cf.Smoothing << ", ";
				ss << "\"use_bounds\": " << (cf.UseBounds ? "true" : "false") << ", ";
				ss << "\"bounds_min\": [" << cf.BoundsMin.x << ", " << cf.BoundsMin.y << "], ";
				ss << "\"bounds_max\": [" << cf.BoundsMax.x << ", " << cf.BoundsMax.y << "], ";
				ss << "\"enabled\": " << (cf.Enabled ? "true" : "false") << " }";
			}

			if (registry.any_of<PrefabReferenceComponent>(entity))
			{
				auto& pr = registry.get<PrefabReferenceComponent>(entity);
				writeCompHeader("PrefabReferenceComponent");
				ss << "{ \"prefab_path\": \"" << pr.PrefabPath << "\", ";
				ss << "\"prefab_name\": \"" << pr.PrefabName << "\" }";
			}

			if (registry.any_of<AudioSourceComponent>(entity))
			{
				auto& au = registry.get<AudioSourceComponent>(entity);
				writeCompHeader("AudioSourceComponent");
				ss << "{ \"audio_path\": \"" << au.AudioPath << "\", ";
				ss << "\"volume\": " << au.Volume << ", ";
				ss << "\"pitch\": " << au.Pitch << ", ";
				ss << "\"loop\": " << (au.Loop ? "true" : "false") << ", ";
				ss << "\"play_on_awake\": " << (au.PlayOnAwake ? "true" : "false") << ", ";
				ss << "\"is_3d\": " << (au.Is3D ? "true" : "false") << ", ";
				ss << "\"min_distance\": " << au.MinDistance << ", ";
				ss << "\"max_distance\": " << au.MaxDistance << " }";
			}

			if (registry.any_of<AudioListenerComponent>(entity))
			{
				auto& al = registry.get<AudioListenerComponent>(entity);
				writeCompHeader("AudioListenerComponent");
				ss << "{ \"enabled\": " << (al.Enabled ? "true" : "false") << ", ";
				ss << "\"volume\": " << al.Volume << " }";
			}

			if (registry.any_of<CameraFollowComponent>(entity))
			{
				auto& cf = registry.get<CameraFollowComponent>(entity);
				writeCompHeader("CameraFollowComponent");
				ss << "{ \"offset\": [" << cf.Offset.x << ", " << cf.Offset.y << "], ";
				ss << "\"smoothing\": " << cf.Smoothing << ", ";
				ss << "\"use_bounds\": " << (cf.UseBounds ? "true" : "false") << ", ";
				ss << "\"bounds_min\": [" << cf.BoundsMin.x << ", " << cf.BoundsMin.y << "], ";
				ss << "\"bounds_max\": [" << cf.BoundsMax.x << ", " << cf.BoundsMax.y << "], ";
				ss << "\"enabled\": " << (cf.Enabled ? "true" : "false") << " }";
			}

			if (registry.any_of<CircleCollider2DComponent>(entity))
			{
				auto& cc = registry.get<CircleCollider2DComponent>(entity);
				writeCompHeader("CircleCollider2DComponent");
				ss << "{ \"offset\": [" << cc.Offset.x << ", " << cc.Offset.y << "], ";
				ss << "\"radius\": " << cc.Radius << ", ";
				ss << "\"density\": " << cc.Density << ", ";
				ss << "\"friction\": " << cc.Friction << ", ";
				ss << "\"restitution\": " << cc.Restitution << " }";
			}

			ss << "\n" << WriteIndent(3) << "}\n";
			ss << WriteIndent(2) << "}";
		}

		ss << "\n" << WriteIndent(1) << "],\n";
		ss << WriteIndent(1) << "\"meta\": {\n";
		ss << WriteIndent(2) << "\"version\": 1,\n";
		ss << WriteIndent(2) << "\"engine\": \"QuentlamEngine\"\n";
		ss << WriteIndent(1) << "}\n";
		ss << "}\n";

		return ss.str();
	}

	bool SceneSerializer::DeserializeFromString(const std::string& data)
	{
		if (data.empty())
			return false;

		size_t pos = 0;
		bool ok = ParseJSON(data, pos);
		if (!ok)
		{
			QL_CORE_ERROR("SceneSerializer: Failed to parse scene JSON");
			return false;
		}

		QL_CORE_TRACE("Scene deserialized ({0} chars)", data.size());
		return true;
	}

	bool SceneSerializer::ParseJSON(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (pos >= data.size() || data[pos] != '{')
			return false;
		pos++;

		while (true)
		{
			SkipWhitespace(data, pos);
			if (pos >= data.size() || data[pos] == '}')
			{
				if (pos < data.size()) pos++;
				return true;
			}
			if (data[pos] == ',')
			{
				pos++;
				SkipWhitespace(data, pos);
			}

			std::string key = ParseString(data, pos);
			SkipWhitespace(data, pos);
			if (data[pos] != ':') return false;
			pos++;
			SkipWhitespace(data, pos);

			if (key == "entities")
			{
				ParseArray(data, pos);
			}
			else
			{
				ParseObject(data, pos);
			}
		}
	}

	bool SceneSerializer::ParseObject(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] != '{')
			return false;
		pos++;

		while (true)
		{
			SkipWhitespace(data, pos);

			if (data[pos] == '}')
			{
				pos++;
				return true;
			}
			if (data[pos] == ',')
			{
				pos++;
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					return true;
				}
			}

			std::string key = ParseString(data, pos);
			SkipWhitespace(data, pos);
			if (data[pos] != ':') return false;
			pos++;
			SkipWhitespace(data, pos);

			if (key == "entity_id")
			{
				ParseNumber(data, pos);
				Entity entity = (m_DeserializationTarget ? m_DeserializationTarget : m_Scene.get())->CreateEntity();
				m_CurrentEntity = static_cast<entt::entity>(entity);
			}
			else if (key == "tag")
			{
				std::string tag = ParseString(data, pos);
				if (m_CurrentEntity != entt::null)
				{
					auto& tc = (m_DeserializationTarget ? m_DeserializationTarget : m_Scene.get())->GetRegistry().get<TagComponent>(m_CurrentEntity);
					tc.Tag = tag;
				}
			}
			else if (key == "components")
			{
				ParseComponents(data, pos);
			}
			else
			{
				ParseValue(data, pos);
			}
		}
	}

	bool SceneSerializer::ParseArray(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] != '[') return false;
		pos++;

		while (true)
		{
			SkipWhitespace(data, pos);
			if (pos >= data.size() || data[pos] == ']')
			{
				if (pos < data.size()) pos++;
				return true;
			}
			if (data[pos] == ',')
			{
				pos++;
				SkipWhitespace(data, pos);
			}
			if (pos >= data.size() || data[pos] == ']')
			{
				if (pos < data.size()) pos++;
				return true;
			}

			ParseObject(data, pos);
		}
	}

	void SceneSerializer::ParseComponents(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] != '{') return;
		pos++;

		while (true)
		{
			SkipWhitespace(data, pos);

			if (data[pos] == '}')
			{
				pos++;
				return;
			}
			if (data[pos] == ',')
			{
				pos++;
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					return;
				}
			}

			std::string compName = ParseString(data, pos);
			SkipWhitespace(data, pos);
			if (data[pos] != ':') { ParseValue(data, pos); continue; }
			pos++;
			SkipWhitespace(data, pos);

			ParseComponentData(data, pos, compName);
		}
	}

	void SceneSerializer::ParseComponentData(const std::string& data, size_t& pos, const std::string& compName)
	{
		if (m_CurrentEntity == entt::null)
		{
			SkipObject(data, pos);
			return;
		}

		auto& registry = (m_DeserializationTarget ? m_DeserializationTarget : m_Scene.get())->GetRegistry();

		if (compName == "LuaScriptComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			std::string scriptPath;
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "script_path")
				{
					scriptPath = ParseString(data, pos);
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!scriptPath.empty())
			{
				if (!registry.any_of<LuaScriptComponent>(m_CurrentEntity))
					registry.emplace<LuaScriptComponent>(m_CurrentEntity, scriptPath);
				else
					registry.get<LuaScriptComponent>(m_CurrentEntity).ScriptPath = scriptPath;
				QL_CORE_TRACE("Deserialized LuaScriptComponent: {0}", scriptPath);
			}
		}
		else if (compName == "DirectionalLightComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::vec3 color(1.0f);
			float intensity = 1.0f;
			glm::vec3 direction(-0.5f, -1.0f, -0.3f);
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "color")
				{
					color = ParseVec3(data, pos);
				}
				else if (field == "intensity")
				{
					intensity = (float)ParseNumber(data, pos);
				}
				else if (field == "direction")
				{
					direction = ParseVec3(data, pos);
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!registry.any_of<DirectionalLightComponent>(m_CurrentEntity))
				registry.emplace<DirectionalLightComponent>(m_CurrentEntity);
			auto& dlc = registry.get<DirectionalLightComponent>(m_CurrentEntity);
			dlc.Color = color;
			dlc.Intensity = intensity;
			dlc.Direction = direction;
		}
		else if (compName == "WeatherSystemComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			float timeOfDay = 0.4f;
			float weatherIntensity = 0.0f;
			float rainIntensity = 0.0f;
			bool enabled = true;
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "time_of_day")
				{
					timeOfDay = (float)ParseNumber(data, pos);
				}
				else if (field == "weather_intensity")
				{
					weatherIntensity = (float)ParseNumber(data, pos);
				}
				else if (field == "rain_intensity")
				{
					rainIntensity = (float)ParseNumber(data, pos);
				}
				else if (field == "enabled")
				{
					enabled = ParseBool(data, pos);
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!registry.any_of<WeatherSystemComponent>(m_CurrentEntity))
				registry.emplace<WeatherSystemComponent>(m_CurrentEntity);
			auto& wsc = registry.get<WeatherSystemComponent>(m_CurrentEntity);
			wsc.TimeOfDay = timeOfDay;
			wsc.WeatherIntensity = weatherIntensity;
			wsc.RainIntensity = rainIntensity;
			wsc.Enabled = enabled;
		}
		else if (compName == "SkyboxComponent")
		{
			bool visible = true;
			int mode = 0;
			while (data[pos] && data[pos] != '}')
			{
				SkipWhitespace(data, pos);
				std::string key = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] == ':') { pos++; SkipWhitespace(data, pos); }
				if (key == "visible") visible = ParseBool(data, pos);
				else if (key == "mode") mode = static_cast<int>(ParseNumber(data, pos));
				else ParseValue(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] == ',') pos++;
			}
			if (!registry.any_of<SkyboxComponent>(m_CurrentEntity))
				registry.emplace<SkyboxComponent>(m_CurrentEntity);
			auto& sky = registry.get<SkyboxComponent>(m_CurrentEntity);
			sky.Visible = visible;
			sky.Mode = static_cast<SkyboxComponent::SkyMode>(mode);
		}
		else if (compName == "SceneGroupComponent")
		{
			bool expanded = true;
			while (data[pos] && data[pos] != '}')
			{
				SkipWhitespace(data, pos);
				std::string key = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] == ':') { pos++; SkipWhitespace(data, pos); }
				if (key == "expanded") expanded = ParseBool(data, pos);
				else ParseValue(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] == ',') pos++;
			}
			if (!registry.any_of<SceneGroupComponent>(m_CurrentEntity))
				registry.emplace<SceneGroupComponent>(m_CurrentEntity);
			auto& grp = registry.get<SceneGroupComponent>(m_CurrentEntity);
			grp.Expanded = expanded;
		}
		else if (compName == "TransformComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::mat4 mat(1.0f);
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "matrix")
				{
					if (data[pos] == '[') pos++;
					for (int c = 0; c < 4; c++)
					{
						SkipWhitespace(data, pos);
						if (data[pos] == '[') pos++;
						for (int r = 0; r < 4; r++)
						{
							mat[c][r] = ParseNumber(data, pos);
							SkipWhitespace(data, pos);
						}
						SkipWhitespace(data, pos);
						if (data[pos] == ']') pos++;
						SkipWhitespace(data, pos);
						if (data[pos] == ',') pos++;
					}
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!registry.any_of<TransformComponent>(m_CurrentEntity))
				registry.emplace<TransformComponent>(m_CurrentEntity, mat);
			else
				registry.get<TransformComponent>(m_CurrentEntity).Transform = mat;
		}
		else if (compName == "SpriteRendererComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::vec4 color(1.0f);
			glm::vec2 size(1.0f);
			float tilingFactor = 1.0f, sortingOrder = 0.0f;
			bool flipX = false, flipY = false;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "color")
				{
					if (data[pos] == '[') pos++;
					color.r = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.g = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.b = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.a = ParseNumber(data, pos);
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else if (field == "size")
				{
					if (data[pos] == '[') pos++;
					size.x = ParseNumber(data, pos);
					SkipComma(data, pos);
					size.y = ParseNumber(data, pos);
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else if (field == "tiling_factor")
				{
					tilingFactor = ParseNumber(data, pos);
				}
				else if (field == "sorting_order")
				{
					sortingOrder = ParseNumber(data, pos);
				}
				else if (field == "flip_x")
				{
					flipX = ParseBool(data, pos);
				}
				else if (field == "flip_y")
				{
					flipY = ParseBool(data, pos);
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!registry.any_of<SpriteRendererComponent>(m_CurrentEntity))
			{
				auto& src = registry.emplace<SpriteRendererComponent>(m_CurrentEntity);
				src.Color = color; src.Size = size;
				src.TilingFactor = tilingFactor; src.SortingOrder = sortingOrder;
				src.FlipX = flipX; src.FlipY = flipY;
			}
			else
			{
				auto& src = registry.get<SpriteRendererComponent>(m_CurrentEntity);
				src.Color = color; src.Size = size;
				src.TilingFactor = tilingFactor; src.SortingOrder = sortingOrder;
				src.FlipX = flipX; src.FlipY = flipY;
			}
		}
		else if (compName == "SpriteTransformComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::vec4 color(1.0f);
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "color")
				{
					if (data[pos] == '[') pos++;
					color.r = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.g = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.b = ParseNumber(data, pos);
					SkipComma(data, pos);
					color.a = ParseNumber(data, pos);
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else
				{
					ParseValue(data, pos);
				}
			}

			if (!registry.any_of<SpriteTransformComponent>(m_CurrentEntity))
				registry.emplace<SpriteTransformComponent>(m_CurrentEntity, color);
			else
				registry.get<SpriteTransformComponent>(m_CurrentEntity).Color = color;
		}
		else if (compName == "UIEntityComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			std::string screenId;
			bool visible = true, blocking = false, focusable = true;
			int32_t zOrder = 0;
			int inputContext = 0;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "screen_id")
					screenId = ParseString(data, pos);
				else if (field == "visible")
					visible = ParseBool(data, pos);
				else if (field == "blocking")
					blocking = ParseBool(data, pos);
				else if (field == "focusable")
					focusable = ParseBool(data, pos);
				else if (field == "z_order")
					zOrder = static_cast<int32_t>(ParseNumber(data, pos));
				else if (field == "input_context")
					inputContext = static_cast<int>(ParseNumber(data, pos));
				else
					ParseValue(data, pos);
			}

			if (!registry.any_of<UIEntityComponent>(m_CurrentEntity))
				registry.emplace<UIEntityComponent>(m_CurrentEntity);
			auto& ui = registry.get<UIEntityComponent>(m_CurrentEntity);
			ui.ScreenId = screenId;
			ui.Visible = visible;
			ui.Blocking = blocking;
			ui.Focusable = focusable;
			ui.ZOrder = zOrder;
			ui.InputContext = static_cast<EInputContext>(inputContext);
		}
		else if (compName == "CameraFollowComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::vec2 offset(0.0f), boundsMin(0.0f), boundsMax(100.0f);
			float smoothing = 5.0f;
			bool useBounds = false, enabled = true;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "offset" || field == "bounds_min" || field == "bounds_max")
				{
					glm::vec2* target = (field == "offset") ? &offset : (field == "bounds_min") ? &boundsMin : &boundsMax;
					if (data[pos] == '[') pos++;
					target->x = ParseNumber(data, pos);
					SkipComma(data, pos);
					target->y = ParseNumber(data, pos);
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else if (field == "smoothing")
					smoothing = ParseNumber(data, pos);
				else if (field == "use_bounds")
					useBounds = ParseBool(data, pos);
				else if (field == "enabled")
					enabled = ParseBool(data, pos);
				else
					ParseValue(data, pos);
			}

			if (!registry.any_of<CameraFollowComponent>(m_CurrentEntity))
				registry.emplace<CameraFollowComponent>(m_CurrentEntity);
			auto& cf = registry.get<CameraFollowComponent>(m_CurrentEntity);
			cf.Offset = offset;
			cf.Smoothing = smoothing;
			cf.UseBounds = useBounds;
			cf.BoundsMin = boundsMin;
			cf.BoundsMax = boundsMax;
			cf.Enabled = enabled;
		}
		else if (compName == "PrefabReferenceComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			std::string prefabPath, prefabName;
			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}')
				{
					pos++;
					break;
				}
				if (data[pos] == ',')
				{
					pos++;
					SkipWhitespace(data, pos);
				}

				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "prefab_path")
					prefabPath = ParseString(data, pos);
				else if (field == "prefab_name")
					prefabName = ParseString(data, pos);
				else
					ParseValue(data, pos);
			}

			if (!registry.any_of<PrefabReferenceComponent>(m_CurrentEntity))
				registry.emplace<PrefabReferenceComponent>(m_CurrentEntity);
			auto& pr = registry.get<PrefabReferenceComponent>(m_CurrentEntity);
			pr.PrefabPath = prefabPath;
			pr.PrefabName = prefabName;
			pr.IsValid = !prefabPath.empty();
		}
		else if (compName == "AudioSourceComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			std::string audioPath;
			float volume = 1.0f, pitch = 1.0f;
			float minDist = 1.0f, maxDist = 100.0f;
			bool loop = false, playOnAwake = false, is3d = false;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}') { pos++; break; }
				if (data[pos] == ',') { pos++; SkipWhitespace(data, pos); }
				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "audio_path") audioPath = ParseString(data, pos);
				else if (field == "volume") volume = ParseNumber(data, pos);
				else if (field == "pitch") pitch = ParseNumber(data, pos);
				else if (field == "loop") loop = ParseBool(data, pos);
				else if (field == "play_on_awake") playOnAwake = ParseBool(data, pos);
				else if (field == "is_3d") is3d = ParseBool(data, pos);
				else if (field == "min_distance") minDist = ParseNumber(data, pos);
				else if (field == "max_distance") maxDist = ParseNumber(data, pos);
				else ParseValue(data, pos);
			}

			if (!registry.any_of<AudioSourceComponent>(m_CurrentEntity))
				registry.emplace<AudioSourceComponent>(m_CurrentEntity);
			auto& au = registry.get<AudioSourceComponent>(m_CurrentEntity);
			au.AudioPath = audioPath;
			au.Volume = volume;
			au.Pitch = pitch;
			au.Loop = loop;
			au.PlayOnAwake = playOnAwake;
			au.Is3D = is3d;
			au.MinDistance = minDist;
			au.MaxDistance = maxDist;
		}
		else if (compName == "AudioListenerComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			bool enabled = true;
			float volume = 1.0f;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}') { pos++; break; }
				if (data[pos] == ',') { pos++; SkipWhitespace(data, pos); }
				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "enabled") enabled = ParseBool(data, pos);
				else if (field == "volume") volume = ParseNumber(data, pos);
				else ParseValue(data, pos);
			}

			if (!registry.any_of<AudioListenerComponent>(m_CurrentEntity))
				registry.emplace<AudioListenerComponent>(m_CurrentEntity);
			auto& al = registry.get<AudioListenerComponent>(m_CurrentEntity);
			al.Enabled = enabled;
			al.Volume = volume;
		}
		else if (compName == "CircleCollider2DComponent")
		{
			SkipWhitespace(data, pos);
			if (data[pos] != '{') { ParseValue(data, pos); return; }
			pos++;

			glm::vec2 offset(0.0f);
			float radius = 0.5f, density = 1.0f, friction = 0.5f, restitution = 0.0f;

			while (true)
			{
				SkipWhitespace(data, pos);
				if (data[pos] == '}') { pos++; break; }
				if (data[pos] == ',') { pos++; SkipWhitespace(data, pos); }
				std::string field = ParseString(data, pos);
				SkipWhitespace(data, pos);
				if (data[pos] != ':') { ParseValue(data, pos); break; }
				pos++;
				SkipWhitespace(data, pos);

				if (field == "offset")
				{
					if (data[pos] == '[') pos++;
					offset.x = ParseNumber(data, pos);
					SkipComma(data, pos);
					offset.y = ParseNumber(data, pos);
					SkipWhitespace(data, pos);
					if (data[pos] == ']') pos++;
				}
				else if (field == "radius") radius = ParseNumber(data, pos);
				else if (field == "density") density = ParseNumber(data, pos);
				else if (field == "friction") friction = ParseNumber(data, pos);
				else if (field == "restitution") restitution = ParseNumber(data, pos);
				else ParseValue(data, pos);
			}

			if (!registry.any_of<CircleCollider2DComponent>(m_CurrentEntity))
				registry.emplace<CircleCollider2DComponent>(m_CurrentEntity);
			auto& cc = registry.get<CircleCollider2DComponent>(m_CurrentEntity);
			cc.Offset = offset;
			cc.Radius = radius;
			cc.Density = density;
			cc.Friction = friction;
			cc.Restitution = restitution;
		}
		else
		{
			SkipObject(data, pos);
		}
	}

	std::string SceneSerializer::ParseString(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (pos >= data.size() || data[pos] != '"')
			return "";
		pos++;
		std::string result;
		while (pos < data.size() && data[pos] != '"')
		{
			if (data[pos] == '\\' && pos + 1 < data.size())
				pos++;
			result += data[pos];
			pos++;
		}
		if (pos < data.size()) pos++;
		return result;
	}

	float SceneSerializer::ParseNumber(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		std::string num;
		while (pos < data.size())
		{
			char c = data[pos];
			if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E')
			{
				num += c;
				pos++;
			}
			else
			{
				break;
			}
		}
		return num.empty() ? 0.0f : static_cast<float>(std::atof(num.c_str()));
	}

	glm::vec3 SceneSerializer::ParseVec3(const std::string& data, size_t& pos)
	{
		glm::vec3 result(0.0f);
		SkipWhitespace(data, pos);
		if (data[pos] == '[') pos++;
		result.x = ParseNumber(data, pos);
		SkipComma(data, pos);
		result.y = ParseNumber(data, pos);
		SkipComma(data, pos);
		result.z = ParseNumber(data, pos);
		SkipWhitespace(data, pos);
		if (data[pos] == ']') pos++;
		return result;
	}

	bool SceneSerializer::ParseBool(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data.compare(pos, 4, "true") == 0)
		{
			pos += 4;
			return true;
		}
		if (data.compare(pos, 5, "false") == 0)
		{
			pos += 5;
			return false;
		}
		return false;
	}

	bool SceneSerializer::ParseValue(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (pos >= data.size()) return false;
		if (data[pos] == '{')
			SkipObject(data, pos);
		else if (data[pos] == '[')
			SkipArray(data, pos);
		else if (data[pos] == '"')
			ParseString(data, pos);
		else
			ParseNumber(data, pos);
		return true;
	}

	void SceneSerializer::SkipObject(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] != '{') return;
		pos++;
		int depth = 1;
		while (depth > 0 && pos < data.size())
		{
			if (data[pos] == '{' || data[pos] == '[') depth++;
			else if (data[pos] == '}' || data[pos] == ']') depth--;
			pos++;
		}
	}

	void SceneSerializer::SkipArray(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] != '[') return;
		pos++;
		int depth = 1;
		while (depth > 0 && pos < data.size())
		{
			if (data[pos] == '{' || data[pos] == '[') depth++;
			else if (data[pos] == '}' || data[pos] == ']') depth--;
			pos++;
		}
	}

	void SceneSerializer::SkipComma(const std::string& data, size_t& pos)
	{
		SkipWhitespace(data, pos);
		if (data[pos] == ',') pos++;
	}

	void SceneSerializer::SkipWhitespace(const std::string& data, size_t& pos)
	{
		while (pos < data.size() && std::isspace(static_cast<unsigned char>(data[pos])))
			pos++;
	}

	Entity SceneSerializer::DeserializeEntity(Scene* scene, const std::string& entityJson)
	{
		if (!scene || entityJson.empty())
			return Entity{ entt::null, nullptr };

		// Wrap entity JSON in minimal scene JSON
		std::string sceneJson = "{ \"entities\": [ " + entityJson + " ] }";
		size_t pos = 0;
		SkipWhitespace(sceneJson, pos);
		if (sceneJson[pos] != '{') return Entity{ entt::null, nullptr };
		pos++;

		// Find "entities" key
		while (pos < sceneJson.size())
		{
			SkipWhitespace(sceneJson, pos);
			if (sceneJson[pos] == '}') break;

			if (sceneJson[pos] == ',') { pos++; SkipWhitespace(sceneJson, pos); }

			if (sceneJson.substr(pos, 10) == "\"entities\"")
			{
				pos += 10;
				SkipWhitespace(sceneJson, pos);
				if (sceneJson[pos] != ':') break;
				pos++;
				SkipWhitespace(sceneJson, pos);

				// Parse entities array and first entity
				if (sceneJson[pos] == '[') pos++;
				SkipWhitespace(sceneJson, pos);

				// Parse entity object manually
				Entity newEntity = scene->CreateEntity("");
				m_CurrentEntity = static_cast<entt::entity>(newEntity);
				if (sceneJson[pos] == '{')
				{
					pos++;
					while (pos < sceneJson.size())
					{
						SkipWhitespace(sceneJson, pos);
						if (sceneJson[pos] == '}') { pos++; break; }
						if (sceneJson[pos] == ',') { pos++; SkipWhitespace(sceneJson, pos); }

						std::string key = ParseString(sceneJson, pos);
						SkipWhitespace(sceneJson, pos);
						if (sceneJson[pos] != ':') break;
						pos++;
						SkipWhitespace(sceneJson, pos);

						if (key == "tag")
						{
							std::string tag = ParseString(sceneJson, pos);
							auto& tc = newEntity.GetComponent<TagComponent>();
							tc.Tag = tag;
						}
						else if (key == "components")
						{
							ParseComponentData(sceneJson, pos, "");
						}
						else
						{
							ParseValue(sceneJson, pos);
						}
					}
				}

				return newEntity;
			}
			else
			{
				std::string key = ParseString(sceneJson, pos);
				ParseValue(sceneJson, pos);
				if (pos < sceneJson.size() && sceneJson[pos] == ',') pos++;
			}
		}

		return Entity{ entt::null, nullptr };
	}
}