#pragma once
#include "qlpch.h"
#include "spdlog/spdlog.h"
#include "Base.h"

namespace Quentlam
{
	 class QUENTLAM_API Log
	{

	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetBaseLogger() { return s_BaseLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }


	private:
		static std::shared_ptr<spdlog::logger> s_BaseLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};

	// Provided for Instrumentor.h to safely access the logger without including Log.h
	QUENTLAM_API std::shared_ptr<spdlog::logger>& GetBaseLoggerRef();


}




//Base Logger macros
#define QL_CORE_TRACE(...)   ::Quentlam::Log::GetBaseLogger()->trace(__VA_ARGS__)
#define QL_CORE_INFO(...)    ::Quentlam::Log::GetBaseLogger()->info(__VA_ARGS__)
#define QL_CORE_WARN(...)    ::Quentlam::Log::GetBaseLogger()->warn(__VA_ARGS__)
#define QL_CORE_ERROR(...)   ::Quentlam::Log::GetBaseLogger()->error(__VA_ARGS__)
#define QL_CORE_FATAL(...)   ::Quentlam::Log::GetBaseLogger()->critical(__VA_ARGS__)


//Client Logger macros
#define QL_TRACE(...)   ::Quentlam::Log::GetClientLogger()->trace(__VA_ARGS__)
#define QL_INFO(...)    ::Quentlam::Log::GetClientLogger()->info(__VA_ARGS__)
#define QL_WARN(...)    ::Quentlam::Log::GetClientLogger()->warn(__VA_ARGS__)
#define QL_ERROR(...)   ::Quentlam::Log::GetClientLogger()->error(__VA_ARGS__)
#define QL_FATAL(...)   ::Quentlam::Log::GetClientLogger()->critical(__VA_ARGS__)
