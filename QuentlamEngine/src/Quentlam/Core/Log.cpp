#include "qlpch.h"

#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace Quentlam
{
	Quentlam::Ref<spdlog::logger> Log::s_BaseLogger;
	Quentlam::Ref<spdlog::logger> Log::s_ClientLogger;

	std::shared_ptr<spdlog::logger>& GetBaseLoggerRef()
	{
		return Log::GetBaseLogger();
	}

	void Log::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");//可以去关于spglog那里的github上的wiki的地方看关于pattern的设置
		s_BaseLogger = spdlog::stdout_color_mt("QUENTLAM");
		s_BaseLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

}