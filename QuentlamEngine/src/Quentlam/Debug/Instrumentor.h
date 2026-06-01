#pragma once

#include <string>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <sstream>

// Must include before any engine headers to avoid circular PCH dependency
#include "Quentlam/Core/Base.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam {

	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;
		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	class Instrumentor
	{
	public:
		Instrumentor() = default;

		Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

		static spdlog::logger* GetLoggerSafe()
		{
			// Access Log::GetBaseLogger without including Log.h
			extern std::shared_ptr<spdlog::logger>& GetBaseLoggerRef();
			return GetBaseLoggerRef().get();
		}

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead.  That's better than having badly formatted
				// profiling output.
				if (spdlog::logger* logger = GetLoggerSafe())
				{
					logger->error("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
				}
				InternalEndSession();
			}
			m_OutputStream.open(filepath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({ name });
				WriteHeader();
			}
			else
			{
				if (spdlog::logger* logger = GetLoggerSafe())
				{
					logger->error("Instrumentor could not open results file '{0}'.", filepath);
				}
			}
		}

		void EndSession()
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard<std::mutex> lock(m_Mutex);
			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}

	private:
		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
			m_OutputStream.flush();
		}

		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				m_OutputStream << "]}";
				m_OutputStream.flush();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}

		InstrumentationSession* m_CurrentSession = nullptr;
		std::ofstream m_OutputStream;
		std::mutex m_Mutex;
	};

	struct InstrumentationTimer
	{
		InstrumentationTimer(const char* name)
			: m_Name(name), m_StartTimepoint(std::chrono::steady_clock::now())
		{
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::time_point_cast<FloatingPointMicroseconds>(endTimepoint).time_since_epoch() - highResStart);

			Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });
			m_Stopped = true;
		}

	private:
		const char* m_Name = nullptr;
		std::chrono::steady_clock::time_point m_StartTimepoint;
		bool m_Stopped = false;
	};

}

#define QL_PROFILE 0
#if QL_PROFILE
	#define QL_PROFILE_SCOPE_LINE(name, line) constexpr auto fixed_name_##line = name; InstrumentationTimer timer_##line(fixed_name_##line)
	#define QL_PROFILE_SCOPE(name) QL_PROFILE_SCOPE_LINE(name, __LINE__)
	#define QL_PROFILE_FUNCTION() QL_PROFILE_SCOPE(__FUNCSIG__)
#else
	#define QL_PROFILE_SCOPE(name)
	#define QL_PROFILE_FUNCTION()
#endif
#define QL_PROFILE_BEGIN_SESSION(name, filepath) Quentlam::Instrumentor::Get().BeginSession(name, filepath)
#define QL_PROFILE_END_SESSION() Quentlam::Instrumentor::Get().EndSession()
