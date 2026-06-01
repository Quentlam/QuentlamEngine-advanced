#include "qlpch.h"
#include "Quentlam/Modding/LuaScriptInstance.h"
#include "Quentlam/Scene/Scene.h"

namespace Quentlam
{

LuaScriptInstance::LuaScriptInstance(Scene* scene, uint32_t entityID, const std::string& scriptPath)
	: m_Scene(scene), m_EntityID(entityID), m_ScriptPath(scriptPath), m_Valid(false)
{
	CreateEnvironment();
	SetupGlobals();
}

LuaScriptInstance::~LuaScriptInstance()
{
}

void LuaScriptInstance::CreateEnvironment()
{
}

void LuaScriptInstance::SetupGlobals()
{
}

void LuaScriptInstance::SetupInstanceAPI()
{
}

bool LuaScriptInstance::Instantiate()
{
	if (m_ScriptPath.empty())
	{
		QL_CORE_WARN("LuaScriptInstance: script path is empty for entity {}", m_EntityID);
		return false;
	}
	m_Valid = true;
	return true;
}

void LuaScriptInstance::OnLoad()
{
}

void LuaScriptInstance::OnUpdate(float dt)
{
}

void LuaScriptInstance::OnDestroy()
{
}

bool LuaScriptInstance::CallFunction(const std::string& name)
{
	return false;
}

void LuaScriptInstance::SetEntityPosition(const glm::vec3& pos)
{
}

void LuaScriptInstance::SetEntityColor(const glm::vec4& color)
{
}

void LuaScriptInstance::SetEntityVisible(bool visible)
{
}

std::unordered_map<uint32_t, glm::vec3> LuaScriptInstance::GetAllEntityPositions() const
{
	return {};
}

std::unordered_map<uint32_t, glm::vec4> LuaScriptInstance::GetAllEntityColors() const
{
	return {};
}

std::unordered_map<uint32_t, bool> LuaScriptInstance::GetAllEntityVisibility() const
{
	return {};
}

std::string LuaScriptInstance::GetGameState() const
{
	return "Playing";
}

int LuaScriptInstance::GetGameScore() const
{
	return 0;
}

}
