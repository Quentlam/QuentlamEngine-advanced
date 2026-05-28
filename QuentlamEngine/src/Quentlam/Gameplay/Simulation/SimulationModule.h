#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>
#include <variant>
#include <unordered_map>

namespace Quentlam
{
	enum class ESeason : uint8_t
	{
		Spring = 0,
		Summer = 1,
		Autumn = 2,
		Winter = 3
	};

	enum class EWeather : uint8_t
	{
		Sunny = 0,
		Cloudy = 1,
		Rainy = 2,
		Storm = 3,
		Snow = 4,
		Windy = 5
	};

	enum class ETimeOfDay : uint8_t
	{
		Night = 0,
		Morning = 1,
		Midday = 2,
		Afternoon = 3,
		Evening = 4,
		Midnight = 5
	};

	class GameClock
	{
	public:
		GameClock() = default;
		GameClock(int32_t startHour, int32_t startMinute);

		int32_t GetHour() const { return m_Hour; }
		int32_t GetMinute() const { return m_Minute; }
		float GetTimeOfDay() const { return m_Hour + m_Minute / 60.0f; }

		void SetTime(int32_t hour, int32_t minute);
		void AddTime(float hours);
		void AdvanceDay();

		float GetDayFraction() const { return GetTimeOfDay() / 24.0f; }
		ETimeOfDay GetTimeOfDayEnum() const;

		int32_t GetTotalMinutes() const { return m_Hour * 60 + m_Minute; }
		int32_t GetDayOfYear() const { return m_DayOfYear; }
		int32_t GetYear() const { return m_Year; }

		void SetDayOfYear(int32_t day) { m_DayOfYear = day; }
		void SetYear(int32_t year) { m_Year = year; }

		float GetTimeSpeed() const { return m_TimeSpeed; }
		void SetTimeSpeed(float speed) { m_TimeSpeed = speed; }

		bool IsPaused() const { return m_Paused; }
		void SetPaused(bool paused) { m_Paused = paused; }

		std::string GetTimeString(bool showSeconds = false) const;
		std::string GetDateString() const;

	private:
		int32_t m_Hour = 6;
		int32_t m_Minute = 0;
		int32_t m_DayOfYear = 1;
		int32_t m_Year = 1;
		float m_TimeSpeed = 1.0f;
		bool m_Paused = false;
	};

	class Calendar
	{
	public:
		Calendar() = default;

		static constexpr int32_t DaysPerSeason = 28;
		static constexpr int32_t DaysPerYear = DaysPerSeason * 4;

		ESeason GetCurrentSeason() const;
		int32_t GetDayOfSeason() const;
		std::string GetSeasonName() const;
		std::string GetSeasonName(ESeason season) const;

		bool IsHoliday(int32_t day, ESeason season) const;
		const std::string& GetHolidayName(int32_t day, ESeason season) const;
		void RegisterHoliday(ESeason season, int32_t day, const std::string& name);
		void ClearHolidays();

		int32_t GetFestivalDay(ESeason season) const;
		const std::string& GetFestivalName(ESeason season) const;
		void SetFestival(ESeason season, int32_t day, const std::string& name);

		bool CanSleep(ESeason currentSeason, int32_t currentDay, EWeather currentWeather) const;

		std::string GetDateString(int32_t day, ESeason season, int32_t year) const;

	private:
		int32_t m_FestivalDays[4] = { 0, 7, 14, 21 };
		std::string m_FestivalNames[4] = { "Egg Festival", "Luau", "Spirits Eve", "Festival of Ice" };

		struct HolidayHash {
			size_t operator()(const std::pair<int32_t, ESeason>& p) const {
				return std::hash<int>()(p.first) ^ (static_cast<size_t>(p.second) << 4);
			}
		};
		std::unordered_map<std::pair<int32_t, ESeason>, std::string, HolidayHash> m_Holidays;
	};

	struct SeasonRule
	{
		ESeason Season = ESeason::Spring;
		float DayLengthMultiplier = 1.0f;
		float NightLengthMultiplier = 1.0f;
		float CropGrowthMultiplier = 1.0f;
		float FishSpawnMultiplier = 1.0f;
		float ForageMultiplier = 1.0f;
		std::vector<EWeather> AllowedWeathers;
		float WeatherChances[6] = { 0.5f, 0.2f, 0.2f, 0.05f, 0.0f, 0.05f };
	};

	class SimulationModule
	{
	public:
		SimulationModule();
		static SimulationModule& Get();

		void Update(float deltaTime);

		GameClock& GetClock() { return m_Clock; }
		const GameClock& GetClock() const { return m_Clock; }
		Calendar& GetCalendar() { return m_Calendar; }
		const Calendar& GetCalendar() const { return m_Calendar; }

		EWeather GetCurrentWeather() const { return m_CurrentWeather; }
		void SetWeather(EWeather weather);
		void ForceWeatherChange(EWeather weather);

		ESeason GetCurrentSeason() const;
		int32_t GetCurrentDay() const;
		int32_t GetCurrentYear() const;

		void OnDayStart();
		void OnDayEnd();
		void OnSeasonChange(ESeason newSeason);
		void OnYearChange();

		float GetWeatherDuration() const { return m_WeatherDuration; }
		void SetWeatherDuration(float hours) { m_WeatherDuration = hours; }

		const SeasonRule* GetSeasonRule(ESeason season) const;
		void SetSeasonRule(ESeason season, const SeasonRule& rule);
		const SeasonRule& GetCurrentSeasonRule() const;

		float GetDayProgress() const;
		float GetSeasonProgress() const;

		void OnNewGame();
		void AdvanceDayInternal();

		std::function<void()> OnDayStartCallback;
		std::function<void()> OnDayEndCallback;
		std::function<void(ESeason, ESeason)> OnSeasonChangeCallback;
		std::function<void(int32_t, int32_t)> OnYearChangeCallback;
		std::function<void(EWeather, EWeather)> OnWeatherChangeCallback;
		std::function<void()> OnMidnightCallback;
		std::function<void(float hoursRemaining)> OnApproachingMidnightCallback;

	private:
		void AdvanceTime(float deltaTime);
		void EvaluateWeatherChange();
		EWeather RollWeather() const;

		GameClock m_Clock;
		Calendar m_Calendar;
		SeasonRule m_SeasonRules[4];
		EWeather m_CurrentWeather = EWeather::Sunny;
		EWeather m_NextWeather = EWeather::Sunny;
		float m_WeatherDuration = 24.0f;
		float m_WeatherTimer = 0.0f;
		bool m_WeatherLocked = false;

		float m_MidnightWarningThreshold = 0.5f;
		bool m_MidnightWarningFired = false;

		float m_TimeAccumulator = 0.0f;
		float m_LastDayProgress = 0.0f;
	};

	enum class EDailyEventType : uint8_t
	{
		None = 0,
		Holiday = 1,
		Festival = 2,
		Wedding = 3,
		Shipment = 4,
		Birthday = 5,
		Custom = 100
	};

	struct DailyEvent
	{
		int32_t Day = 1;
		ESeason Season = ESeason::Spring;
		EDailyEventType Type = EDailyEventType::None;
		std::string Name;
		std::string Description;
		std::string TriggeredFlag;
		std::vector<std::string> RequiredFlags;
		std::unordered_map<std::string, int32_t> NpcAttendance;
	};

	class DailyEventBus
	{
	public:
		DailyEventBus() = default;
		static DailyEventBus& Get();

		void RegisterEvent(const DailyEvent& event);
		void UnregisterEvent(const std::string& eventName);
		void ClearEvents();

		const DailyEvent* GetEventForDay(int32_t day, ESeason season) const;
		std::vector<const DailyEvent*> GetEventsForSeason(ESeason season) const;

		bool TriggerEvent(const std::string& eventName);
		bool TriggerEventForDay(int32_t day, ESeason season);

		bool IsEventActive(const std::string& eventName) const;
		const DailyEvent* GetActiveEvent() const { return m_ActiveEvent; }

		void OnDayTick(int32_t day, ESeason season);

	private:
		std::vector<DailyEvent> m_Events;
		const DailyEvent* m_ActiveEvent = nullptr;
		std::unordered_set<std::string> m_TriggeredEvents;
	};

	inline GameClock::GameClock(int32_t startHour, int32_t startMinute)
		: m_Hour(startHour), m_Minute(startMinute) {}

	inline void GameClock::SetTime(int32_t hour, int32_t minute)
	{
		m_Hour = std::clamp(hour, 0, 23);
		m_Minute = std::clamp(minute, 0, 59);
	}

	inline void GameClock::AddTime(float hours)
	{
		int32_t totalMinutes = GetTotalMinutes() + static_cast<int32_t>(hours * 60.0f);
		if (totalMinutes >= 24 * 60)
		{
			int32_t days = totalMinutes / (24 * 60);
			totalMinutes %= (24 * 60);
			m_DayOfYear += days;
			if (m_DayOfYear > Calendar::DaysPerYear)
			{
				m_Year += (m_DayOfYear - 1) / Calendar::DaysPerYear;
				m_DayOfYear = ((m_DayOfYear - 1) % Calendar::DaysPerYear) + 1;
			}
		}
		m_Hour = totalMinutes / 60;
		m_Minute = totalMinutes % 60;
	}

	inline void GameClock::AdvanceDay()
	{
		m_DayOfYear++;
		if (m_DayOfYear > Calendar::DaysPerYear)
		{
			m_DayOfYear = 1;
			m_Year++;
		}
		SetTime(6, 0);
	}

	inline ETimeOfDay GameClock::GetTimeOfDayEnum() const
	{
		if (m_Hour >= 22 || m_Hour < 6) return ETimeOfDay::Night;
		if (m_Hour >= 6 && m_Hour < 10) return ETimeOfDay::Morning;
		if (m_Hour >= 10 && m_Hour < 14) return ETimeOfDay::Midday;
		if (m_Hour >= 14 && m_Hour < 18) return ETimeOfDay::Afternoon;
		if (m_Hour >= 18 && m_Hour < 22) return ETimeOfDay::Evening;
		return ETimeOfDay::Midnight;
	}

	inline std::string GameClock::GetTimeString(bool showSeconds) const
	{
		char buffer[32];
		if (showSeconds)
			snprintf(buffer, sizeof(buffer), "%02d:%02d:00", m_Hour, m_Minute);
		else
			snprintf(buffer, sizeof(buffer), "%02d:%02d", m_Hour, m_Minute);
		return buffer;
	}

	inline std::string GameClock::GetDateString() const
	{
		Calendar cal;
		ESeason season = cal.GetCurrentSeason();
		return cal.GetDateString(m_DayOfYear, season, m_Year);
	}

	inline int32_t Calendar::GetDayOfSeason() const
	{
		return 1;
	}

	inline std::string Calendar::GetSeasonName() const
	{
		return GetSeasonName(GetCurrentSeason());
	}

	inline std::string Calendar::GetSeasonName(ESeason season) const
	{
		switch (season)
		{
		case ESeason::Spring: return "Spring";
		case ESeason::Summer: return "Summer";
		case ESeason::Autumn: return "Autumn";
		case ESeason::Winter: return "Winter";
		default: return "Unknown";
		}
	}

	inline bool Calendar::IsHoliday(int32_t day, ESeason season) const
	{
		auto key = std::make_pair(day, season);
		return m_Holidays.find(key) != m_Holidays.end();
	}

	inline const std::string& Calendar::GetHolidayName(int32_t day, ESeason season) const
	{
		static std::string empty;
		auto key = std::make_pair(day, season);
		auto it = m_Holidays.find(key);
		return it != m_Holidays.end() ? it->second : empty;
	}

	inline void Calendar::RegisterHoliday(ESeason season, int32_t day, const std::string& name)
	{
		m_Holidays[std::make_pair(day, season)] = name;
	}

	inline void Calendar::ClearHolidays()
	{
		m_Holidays.clear();
	}

	inline int32_t Calendar::GetFestivalDay(ESeason season) const
	{
		return m_FestivalDays[static_cast<int32_t>(season)];
	}

	inline const std::string& Calendar::GetFestivalName(ESeason season) const
	{
		return m_FestivalNames[static_cast<int32_t>(season)];
	}

	inline void Calendar::SetFestival(ESeason season, int32_t day, const std::string& name)
	{
		m_FestivalDays[static_cast<int32_t>(season)] = day;
		m_FestivalNames[static_cast<int32_t>(season)] = name;
	}

	inline bool Calendar::CanSleep(ESeason, int32_t, EWeather weather) const
	{
		return weather != EWeather::Storm;
	}

	inline std::string Calendar::GetDateString(int32_t day, ESeason season, int32_t year) const
	{
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "Year %d, %s Day %d",
			year, GetSeasonName(season).c_str(), day);
		return buffer;
	}

	inline SimulationModule::SimulationModule()
	{
		for (int i = 0; i < 4; ++i)
			m_SeasonRules[i].Season = (ESeason)i;

		m_SeasonRules[static_cast<int32_t>(ESeason::Spring)].AllowedWeathers = {
			EWeather::Sunny, EWeather::Cloudy, EWeather::Rainy, EWeather::Windy };
		m_SeasonRules[static_cast<int32_t>(ESeason::Summer)].WeatherChances[0] = 0.6f;
		m_SeasonRules[static_cast<int32_t>(ESeason::Summer)].WeatherChances[3] = 0.1f;
		m_SeasonRules[static_cast<int32_t>(ESeason::Winter)].WeatherChances[0] = 0.4f;
		m_SeasonRules[static_cast<int32_t>(ESeason::Winter)].WeatherChances[4] = 0.3f;
	}

	inline SimulationModule& SimulationModule::Get()
	{
		static SimulationModule instance;
		return instance;
	}

	inline void SimulationModule::Update(float deltaTime)
	{
		if (m_Clock.IsPaused()) return;
		AdvanceTime(deltaTime);
	}

	inline void SimulationModule::AdvanceTime(float deltaTime)
	{
		if (!m_Clock.IsPaused())
		{
			float scaledDelta = deltaTime * m_Clock.GetTimeSpeed();
			m_TimeAccumulator += scaledDelta;

			if (m_TimeAccumulator >= 1.0f / 60.0f)
			{
				float minutesToAdvance = m_TimeAccumulator * 10.0f;
				int32_t oldHour = m_Clock.GetHour();

				m_Clock.AddTime(minutesToAdvance / 60.0f);
				m_WeatherTimer += minutesToAdvance;

				if (m_Clock.GetHour() == 0 && oldHour != 0 && !m_Clock.IsPaused())
				{
					if (OnMidnightCallback)
						OnMidnightCallback();
				}

				float hoursUntilMidnight = (24.0f - m_Clock.GetTimeOfDay());
				if (hoursUntilMidnight < m_MidnightWarningThreshold && !m_MidnightWarningFired)
				{
					m_MidnightWarningFired = true;
					if (OnApproachingMidnightCallback)
						OnApproachingMidnightCallback(hoursUntilMidnight);
				}
				if (m_Clock.GetHour() >= 6)
					m_MidnightWarningFired = false;

				m_TimeAccumulator = 0.0f;
				EvaluateWeatherChange();
			}
		}
	}

	inline void SimulationModule::OnDayStart()
	{
		if (OnDayStartCallback)
			OnDayStartCallback();
	}

	inline void SimulationModule::OnDayEnd()
	{
		if (OnDayEndCallback)
			OnDayEndCallback();
		m_Clock.AdvanceDay();
	}

	inline void SimulationModule::OnSeasonChange(ESeason newSeason)
	{
		if (OnSeasonChangeCallback)
			OnSeasonChangeCallback(GetCurrentSeason(), newSeason);
	}

	inline void SimulationModule::OnYearChange()
	{
		if (OnYearChangeCallback)
			OnYearChangeCallback(GetCurrentYear() - 1, GetCurrentYear());
	}

	inline void SimulationModule::SetWeather(EWeather weather)
	{
		if (m_WeatherLocked) return;
		if (m_CurrentWeather != weather)
		{
			EWeather old = m_CurrentWeather;
			m_CurrentWeather = weather;
			if (OnWeatherChangeCallback)
				OnWeatherChangeCallback(old, weather);
		}
	}

	inline void SimulationModule::ForceWeatherChange(EWeather weather)
	{
		EWeather old = m_CurrentWeather;
		m_CurrentWeather = weather;
		m_WeatherTimer = 0.0f;
		if (OnWeatherChangeCallback)
			OnWeatherChangeCallback(old, weather);
	}

	inline ESeason SimulationModule::GetCurrentSeason() const
	{
		int32_t day = m_Clock.GetDayOfYear();
		int32_t seasonIndex = (day - 1) / Calendar::DaysPerSeason;
		return (ESeason)std::clamp(seasonIndex, 0, 3);
	}

	inline int32_t SimulationModule::GetCurrentDay() const
	{
		int32_t day = m_Clock.GetDayOfYear();
		return ((day - 1) % Calendar::DaysPerSeason) + 1;
	}

	inline int32_t SimulationModule::GetCurrentYear() const
	{
		return m_Clock.GetYear();
	}

	inline const SeasonRule* SimulationModule::GetSeasonRule(ESeason season) const
	{
		return &m_SeasonRules[static_cast<int32_t>(season)];
	}

	inline void SimulationModule::SetSeasonRule(ESeason season, const SeasonRule& rule)
	{
		m_SeasonRules[static_cast<int32_t>(season)] = rule;
	}

	inline const SeasonRule& SimulationModule::GetCurrentSeasonRule() const
	{
		return m_SeasonRules[static_cast<int32_t>(GetCurrentSeason())];
	}

	inline float SimulationModule::GetDayProgress() const
	{
		return m_Clock.GetDayFraction();
	}

	inline float SimulationModule::GetSeasonProgress() const
	{
		int32_t day = GetCurrentDay();
		return static_cast<float>(day) / Calendar::DaysPerSeason;
	}

	inline void SimulationModule::OnNewGame()
	{
		m_Clock = GameClock(6, 0);
		m_Clock.SetDayOfYear(1);
		m_Clock.SetYear(1);
		m_CurrentWeather = EWeather::Sunny;
		m_WeatherTimer = 0.0f;
		m_TimeAccumulator = 0.0f;
		m_MidnightWarningFired = false;
	}

	inline void SimulationModule::AdvanceDayInternal()
	{
		OnDayEnd();
		OnDayStart();
	}

	inline void SimulationModule::EvaluateWeatherChange()
	{
		if (m_WeatherLocked) return;
		if (m_WeatherTimer >= m_WeatherDuration)
		{
			EWeather newWeather = RollWeather();
			SetWeather(newWeather);
			m_WeatherTimer = 0.0f;
			m_WeatherDuration = 18.0f + static_cast<float>((rand() % 24));
		}
	}

	inline EWeather SimulationModule::RollWeather() const
	{
		const auto& rule = GetCurrentSeasonRule();
		float roll = static_cast<float>(rand()) / RAND_MAX;
		float cumulative = 0.0f;
		for (int i = 0; i < 6; ++i)
		{
			cumulative += rule.WeatherChances[i];
			if (roll < cumulative)
				return (EWeather)i;
		}
		return EWeather::Sunny;
	}
}
