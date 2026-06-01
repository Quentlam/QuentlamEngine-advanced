#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <functional>

namespace Quentlam
{

struct BuildConfiguration
{
	std::string Name = "Release";
	std::string Platform = "Win64";
	std::string OutputDirectory = "Build/";
	bool GeneratePDB = true;
	bool Optimize = true;
	bool StripDebugInfo = false;
};

struct BuildTarget
{
	std::string Name;
	std::string ProjectPath;
	std::string OutputPath;
	std::vector<std::string> AdditionalDependencies;
	bool IsEngineTarget = false;
};

enum class EBuildStep
{
	PreBuild,
	Compile,
	Link,
	PostBuild,
	Package,
	Deploy
};

struct BuildProgress
{
	EBuildStep Step = EBuildStep::PreBuild;
	std::string CurrentTask;
	float Progress = 0.0f;
	int CompletedTasks = 0;
	int TotalTasks = 0;
	bool Success = false;
	std::string ErrorMessage;
};

using BuildProgressCallback = std::function<void(const BuildProgress&)>;

class BuildAutomation
{
public:
	static BuildAutomation& Get();

	void AddTarget(const BuildTarget& target);
	void RemoveTarget(const std::string& name);
	BuildTarget* GetTarget(const std::string& name);
	const std::vector<BuildTarget>& GetTargets() const { return m_Targets; }

	void SetConfiguration(const BuildConfiguration& config);
	const BuildConfiguration& GetConfiguration() const { return m_Configuration; }

	bool Build(const std::string& targetName, BuildProgressCallback callback = nullptr);
	bool BuildAll(BuildProgressCallback callback = nullptr);

	bool Clean(const std::string& targetName);
	bool CleanAll();

	std::string GetBuildLog() const { return m_BuildLog; }
	void ClearLog() { m_BuildLog.clear(); }

private:
	BuildAutomation() = default;
	~BuildAutomation() = default;

	bool RunMSBuild(const BuildTarget& target, const BuildConfiguration& config, BuildProgressCallback callback);
	bool RunBuildCommand(const std::string& command, const std::string& args, BuildProgressCallback callback);

	std::vector<BuildTarget> m_Targets;
	BuildConfiguration m_Configuration;
	std::string m_BuildLog;

	static void AppendLog(const std::string& msg);
};

}
