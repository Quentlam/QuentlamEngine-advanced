#include "qlpch.h"
#include "Quentlam/Modding/LuaScriptBridge.h"
#include "Quentlam/Scene/Scene.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{

LuaScriptBridge::LuaScriptBridge()
	: m_Scene(nullptr)
{
	m_Lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::io, sol::lib::os, sol::lib::math, sol::lib::string, sol::lib::table);
	SetupBindings();
}

LuaScriptBridge::~LuaScriptBridge()
{
	UnloadScript();
}

void LuaScriptBridge::SetupBindings()
{
	SetupInputAPI();
	SetupEntityAPI();
	SetupGameAPI();
	SetupMathAPI();
	SetupAnimationAPI();
	SetupInteractionAPI();
	SetupParticleAPI();
	SetupNavigationAPI();
	SetupSceneAPI();
	SetupAtlasAPI();
	SetupCameraAPI();
	SetupAudioAPI();
	SetupPrefabAPI();
}

void LuaScriptBridge::SetupInputAPI()
{
	m_Lua["Key"] = sol::new_table();
	for (int i = 0; i < 512; ++i)
		m_Lua["Key"][i] = false;

	m_Lua.set_function("Input_IsKeyPressed", [](int key) { return false; });
	m_Lua.set_function("Input_IsMouseButtonPressed", [](int button) { return false; });
	m_Lua.set_function("Input_GetMouseX", [] { return 0.0f; });
	m_Lua.set_function("Input_GetMouseY", [] { return 0.0f; });
}

void LuaScriptBridge::SetupEntityAPI()
{
	m_Lua.set_function("Entity_GetPosition", [this](const std::string& id) {
		auto it = m_EntityPositions.find(id);
		if (it != m_EntityPositions.end()) return it->second;
		return glm::vec3(0.0f);
	});

	m_Lua.set_function("Entity_SetPosition", [this](const std::string& id, float x, float y, float z) {
		m_EntityPositions[id] = glm::vec3(x, y, z);
	});

	m_Lua.set_function("Entity_GetColor", [this](const std::string& id) {
		auto it = m_EntityColors.find(id);
		if (it != m_EntityColors.end()) return it->second;
		return glm::vec4(1.0f);
	});

	m_Lua.set_function("Entity_SetVisible", [this](const std::string& id, bool visible) {
		m_EntityVisibility[id] = visible;
	});

	m_Lua.set_function("Entity_SetTag", [this](const std::string& id, const std::string& tag) {
		m_EntityTags[id] = tag;
	});
}

void LuaScriptBridge::SetupGameAPI()
{
	m_Lua["GameState"] = "Playing";
	m_Lua["Score"] = 0;
	m_Lua["GameTime"] = 0.0f;

	m_Lua.set_function("Game_GetState", [this]() { return m_GameState; });
	m_Lua.set_function("Game_SetState", [this](const std::string& state) { m_GameState = state; });
	m_Lua.set_function("Game_GetScore", [this]() { return m_Score; });
	m_Lua.set_function("Game_AddScore", [this](int delta) { m_Score += delta; });
	m_Lua.set_function("Game_GetTime", [this]() { return m_GameTime; });
	m_Lua.set_function("Game_IsOver", [this]() { return m_GameOver; });
}

void LuaScriptBridge::SetupMathAPI()
{
	m_Lua.set_function("Math_Vec3", [](float x, float y, float z) { return glm::vec3(x, y, z); });
	m_Lua.set_function("Math_Distance", [](const glm::vec3& a, const glm::vec3& b) {
		return glm::distance(a, b);
	});
}

void LuaScriptBridge::SetupAnimationAPI()
{
}

void LuaScriptBridge::SetupInteractionAPI()
{
}

void LuaScriptBridge::SetupParticleAPI()
{
}

void LuaScriptBridge::SetupNavigationAPI()
{
}

void LuaScriptBridge::SetupSceneAPI()
{
	m_Lua.set_function("Scene_Load", [this](const std::string& path) {
		QL_CORE_INFO("Lua: Scene_Load({})", path);
		return true;
	});
	m_Lua.set_function("Scene_Save", [this](const std::string& path) {
		QL_CORE_INFO("Lua: Scene_Save({})", path);
		return true;
	});
}

void LuaScriptBridge::SetupAtlasAPI()
{
}

void LuaScriptBridge::SetupCameraAPI()
{
}

void LuaScriptBridge::SetupAudioAPI()
{
	m_Lua.set_function("Audio_Play", [](const std::string& sound) {
		QL_CORE_INFO("Lua: Audio_Play({})", sound);
	});
	m_Lua.set_function("Audio_Stop", [](const std::string& sound) {
		QL_CORE_INFO("Lua: Audio_Stop({})", sound);
	});
}

void LuaScriptBridge::SetupPrefabAPI()
{
}

bool LuaScriptBridge::LoadScript(const std::string& scriptPath)
{
	try
	{
		m_Lua.script_file(scriptPath);
		m_CurrentScriptPath = scriptPath;
		QL_CORE_INFO("Loaded Lua script: {}", scriptPath);
		return true;
	}
	catch (const sol::error& e)
	{
		QL_CORE_ERROR("Failed to load Lua script '{}': {}", scriptPath, e.what());
		return false;
	}
}

bool LuaScriptBridge::CallFunction(const std::string& funcName, const std::vector<std::string>& args, std::string* result)
{
	try
	{
		sol::function func = m_Lua[funcName];
		if (!func.valid())
			return false;
		if (args.empty())
			func();
		else if (args.size() == 1)
			func(args[0]);
		else if (args.size() == 2)
			func(args[0], args[1]);
		return true;
	}
	catch (const sol::error& e)
	{
		QL_CORE_ERROR("Lua function call failed '{}': {}", funcName, e.what());
		return false;
	}
}

bool LuaScriptBridge::HasFunction(const std::string& funcName) const
{
	return m_Lua[funcName].valid();
}

void LuaScriptBridge::UnloadScript()
{
	m_CurrentScriptPath.clear();
	m_Lua.collect_gc();
}

bool LuaScriptBridge::IsValid() const
{
	return !m_CurrentScriptPath.empty();
}

bool LuaScriptBridge::CallLoad()
{
	if (HasFunction("OnLoad"))
	{
		try { m_Lua["OnLoad"](); return true; }
		catch (const sol::error& e) { QL_CORE_ERROR("OnLoad: {}", e.what()); }
	}
	return false;
}

bool LuaScriptBridge::CallUpdate(float dt)
{
	m_DeltaTime = dt;
	m_GameTime += dt;
	if (HasFunction("OnUpdate"))
	{
		try { m_Lua["OnUpdate"](dt); return true; }
		catch (const sol::error& e) { QL_CORE_ERROR("OnUpdate: {}", e.what()); }
	}
	return false;
}

bool LuaScriptBridge::CallOnSave(const std::string& context)
{
	if (HasFunction("OnSave"))
	{
		try { m_Lua["OnSave"](context); return true; }
		catch (const sol::error& e) { QL_CORE_ERROR("OnSave: {}", e.what()); }
	}
	return false;
}

bool LuaScriptBridge::CallOnLoad(const std::string& context)
{
	if (HasFunction("OnLoad"))
	{
		try { m_Lua["OnLoad"](context); return true; }
		catch (const sol::error& e) { QL_CORE_ERROR("OnLoad: {}", e.what()); }
	}
	return false;
}

}
