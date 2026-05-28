#pragma once
#include "ModdingModule.h"
#include <sol/sol.hpp>
#include <fstream>
#include <sstream>

namespace Quentlam
{
	class LuaScriptBridge : public ScriptBridge
	{
	public:
		LuaScriptBridge();
		~LuaScriptBridge() override;

		bool LoadScript(const std::string& scriptPath) override;
		bool CallFunction(const std::string& funcName, const std::vector<std::string>& args, std::string* result = nullptr) override;
		bool HasFunction(const std::string& funcName) const override;
		void UnloadScript() override;
		bool IsValid() const override;

		bool CallLoad();
		bool CallUpdate(float dt);
		bool CallOnSave(const std::string& context);
		bool CallOnLoad(const std::string& context);

		sol::state& GetLuaState() { return m_Lua; }
		const sol::state& GetLuaState() const { return m_Lua; }

	private:
		void SetupBindings();

		sol::state m_Lua;
		std::string m_CurrentScriptPath;
		std::unordered_set<std::string> m_RegisteredFunctions;
	};
}
