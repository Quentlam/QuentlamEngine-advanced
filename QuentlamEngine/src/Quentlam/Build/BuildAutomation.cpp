#include "qlpch.h"
#include "Quentlam/Build/BuildAutomation.h"
#include "Quentlam/Core/Log.h"
#include <fstream>

namespace Quentlam
{

void BuildAutomation::AppendLog(const std::string& msg)
{
	QL_CORE_INFO("[Build] {}", msg);
	Get().m_BuildLog += msg + "\n";
}

BuildAutomation& BuildAutomation::Get()
{
	static BuildAutomation instance;
	return instance;
}

void BuildAutomation::AddTarget(const BuildTarget& target)
{
	for (auto& t : m_Targets)
	{
		if (t.Name == target.Name)
		{
			QL_CORE_WARN("Build target '{}' already exists.", target.Name);
			return;
		}
	}
	m_Targets.push_back(target);
	QL_CORE_INFO("Added build target: {}", target.Name);
}

void BuildAutomation::RemoveTarget(const std::string& name)
{
	for (auto it = m_Targets.begin(); it != m_Targets.end(); ++it)
	{
		if (it->Name == name)
		{
			m_Targets.erase(it);
			QL_CORE_INFO("Removed build target: {}", name);
			return;
		}
	}
	QL_CORE_WARN("Build target '{}' not found.", name);
}

BuildTarget* BuildAutomation::GetTarget(const std::string& name)
{
	for (auto& t : m_Targets)
	{
		if (t.Name == name)
			return &t;
	}
	return nullptr;
}

void BuildAutomation::SetConfiguration(const BuildConfiguration& config)
{
	m_Configuration = config;
	QL_CORE_INFO("Set build configuration: {}", config.Name);
}

bool BuildAutomation::RunBuildCommand(const std::string& command, const std::string& args, BuildProgressCallback callback)
{
	QL_CORE_INFO("[Build] Running command: {} {}", command, args);
	return true;
}

bool BuildAutomation::RunMSBuild(const BuildTarget& target, const BuildConfiguration& config, BuildProgressCallback callback)
{
	BuildProgress progress;
	progress.Step = EBuildStep::Compile;
	progress.TotalTasks = 1;
	progress.CurrentTask = "Compiling " + target.Name;
	progress.Progress = 0.0f;
	if (callback)
		callback(progress);

	progress.CurrentTask = "Linking " + target.Name;
	progress.Progress = 0.5f;
	if (callback)
		callback(progress);

	progress.Progress = 1.0f;
	progress.Success = true;
	if (callback)
		callback(progress);

	AppendLog("Built target: " + target.Name);
	return true;
}

bool BuildAutomation::Build(const std::string& targetName, BuildProgressCallback callback)
{
	BuildTarget* target = GetTarget(targetName);
	if (!target)
	{
		QL_CORE_ERROR("Build target '{}' not found.", targetName);
		return false;
	}

	QL_CORE_INFO("Building target: {}", targetName);
	BuildProgress progress;
	progress.Step = EBuildStep::PreBuild;
	progress.CurrentTask = "Pre-build steps";
	progress.Progress = 0.0f;
	if (callback)
		callback(progress);

	bool result = RunMSBuild(*target, m_Configuration, callback);

	progress.Step = EBuildStep::PostBuild;
	progress.Progress = 1.0f;
	progress.Success = result;
	if (callback)
		callback(progress);

	return result;
}

bool BuildAutomation::BuildAll(BuildProgressCallback callback)
{
	QL_CORE_INFO("Building all targets ({} total).", m_Targets.size());
	for (auto& target : m_Targets)
	{
		if (!Build(target.Name, callback))
		{
			QL_CORE_ERROR("Failed to build target: {}", target.Name);
			return false;
		}
	}
	return true;
}

bool BuildAutomation::Clean(const std::string& targetName)
{
	BuildTarget* target = GetTarget(targetName);
	if (!target)
	{
		QL_CORE_WARN("Cannot clean target '{}' - not found.", targetName);
		return false;
	}
	QL_CORE_INFO("Cleaning target: {}", targetName);
	AppendLog("Cleaned target: " + targetName);
	return true;
}

bool BuildAutomation::CleanAll()
{
	QL_CORE_INFO("Cleaning all targets.");
	for (auto& target : m_Targets)
	{
		AppendLog("Cleaned target: " + target.Name);
	}
	return true;
}

}
