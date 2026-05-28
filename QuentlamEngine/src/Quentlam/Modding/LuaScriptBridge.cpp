#include "LuaScriptBridge.h"

namespace Quentlam
{
	LuaScriptBridge::LuaScriptBridge()
	{
		m_Lua.open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::table,
			sol::lib::string,
			sol::lib::math,
			sol::lib::io,
			sol::lib::os,
			sol::lib::debug
		);

		m_Lua["print"] = sol::nil;

		SetupBindings();
	}

	LuaScriptBridge::~LuaScriptBridge()
	{
		UnloadScript();
	}

	void LuaScriptBridge::SetupBindings()
	{
		m_Lua.new_enum("QL",
			"VERSION", "1.0.0",
			"ENGINE_NAME", "QuentlamEngine"
		);

		m_Lua.set_function("QL_Log", [](const std::string& msg) {
			QL_CORE_INFO("Lua: {0}", msg);
		});

		m_Lua.set_function("QL_LogWarn", [](const std::string& msg) {
			QL_CORE_WARN("Lua: {0}", msg);
		});

		m_Lua.set_function("QL_LogError", [](const std::string& msg) {
			QL_CORE_ERROR("Lua: {0}", msg);
		});
	}

	bool LuaScriptBridge::LoadScript(const std::string& scriptPath)
	{
		UnloadScript();

		std::ifstream file(scriptPath);
		if (!file.is_open())
		{
			QL_CORE_ERROR("LuaScriptBridge: Failed to open script: {0}", scriptPath);
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		sol::protected_function_result result = m_Lua.safe_script(buffer.str(),
			sol::script_pass_on_error);

		if (!result.valid())
		{
			sol::error err = result;
			QL_CORE_ERROR("LuaScriptBridge: Script load error: {0}", err.what());
			return false;
		}

		m_CurrentScriptPath = scriptPath;
		return true;
	}

	bool LuaScriptBridge::CallFunction(const std::string& funcName, const std::vector<std::string>& args, std::string* result)
	{
		sol::object func = m_Lua[funcName];
		if (!func.valid() || func.get_type() != sol::type::function)
			return false;

		sol::protected_function pf = func;
		sol::protected_function_result pfResult = pf();

		if (!pfResult.valid())
		{
			sol::error err = pfResult;
			QL_CORE_ERROR("LuaScriptBridge: Function '{0}' error: {1}", funcName, err.what());
			return false;
		}

		if (result && pfResult.get_type() == sol::type::string)
			*result = pfResult.get<std::string>();

		return true;
	}

	bool LuaScriptBridge::HasFunction(const std::string& funcName) const
	{
		sol::object func = m_Lua[funcName];
		return func.valid() && func.get_type() == sol::type::function;
	}

	void LuaScriptBridge::UnloadScript()
	{
		m_Lua.collect_garbage();
		m_CurrentScriptPath.clear();
		m_RegisteredFunctions.clear();
	}

	bool LuaScriptBridge::IsValid() const
	{
		return !m_CurrentScriptPath.empty();
	}

	bool LuaScriptBridge::CallLoad()
	{
		return CallFunction("load", {}, nullptr);
	}

	bool LuaScriptBridge::CallUpdate(float dt)
	{
		sol::object func = m_Lua["update"];
		if (!func.valid() || func.get_type() != sol::type::function)
			return true;

		sol::protected_function pf = func;
		sol::protected_function_result result = pf(dt);

		if (!result.valid())
		{
			sol::error err = result;
			QL_CORE_ERROR("LuaScriptBridge: update() error: {0}", err.what());
			return false;
		}
		return true;
	}

	bool LuaScriptBridge::CallOnSave(const std::string& context)
	{
		return CallFunction("onSave", {}, nullptr);
	}

	bool LuaScriptBridge::CallOnLoad(const std::string& context)
	{
		return CallFunction("onLoad", {}, nullptr);
	}
}
