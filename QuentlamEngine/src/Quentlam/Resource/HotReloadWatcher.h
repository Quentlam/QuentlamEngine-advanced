#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

namespace Quentlam
{
	class HotReloadWatcher
	{
	public:
		HotReloadWatcher();
		~HotReloadWatcher();

		void Start(const std::string& watchDirectory);
		void Stop();
		bool IsRunning() const { return m_IsRunning.load(); }

		void SetPollInterval(int32_t milliseconds);
		int32_t GetPollInterval() const { return m_PollIntervalMs.load(); }

		void RegisterCallback(const std::string& extension, std::function<void(const std::string& filepath)> callback);
		void UnregisterCallback(const std::string& extension);
		void ClearCallbacks();

		void AddIgnorePattern(const std::string& pattern);
		void ClearIgnorePatterns();

		void RegisterResource(const std::string& filepath, std::function<void()> reloadFunc);
		void UnregisterResource(const std::string& filepath);
		void ClearResources();

		void TriggerReload(const std::string& filepath);
		void TriggerReloadAll();

		size_t GetPendingReloadCount() const;
		size_t GetWatchedResourceCount() const;
		size_t GetWatchedDirectoryCount() const { return 1; }

		std::function<void(const std::string& filepath)> OnFileChanged;
		std::function<void(const std::string& filepath)> OnReloadTriggered;
		std::function<void(const std::string& error)> OnError;

	private:
		void WatchLoop();
		bool CheckFileChanged(const std::string& filepath, int64_t lastModTime);

		std::string m_WatchDirectory;
		std::atomic<bool> m_IsRunning{ false };
		std::atomic<int32_t> m_PollIntervalMs{ 1000 };

		std::thread m_WatchThread;

		std::unordered_map<std::string, std::function<void(const std::string&)>> m_ExtensionCallbacks;
		std::vector<std::string> m_IgnorePatterns;
		std::unordered_map<std::string, std::function<void()>> m_ResourceReloadFuncs;
		std::unordered_map<std::string, int64_t> m_LastKnownTimes;

		std::vector<std::string> m_PendingReloadQueue;
		mutable std::mutex m_QueueMutex;
	};

	class ResourceReloadGuard
	{
	public:
		ResourceReloadGuard() = default;
		~ResourceReloadGuard();

		void Lock();
		void Unlock();
		bool IsLocked() const { return m_Locked.load(); }

	private:
		std::atomic<bool> m_Locked{ false };
	};

	class TextureReloadManager
	{
	public:
		static TextureReloadManager& Get();

		void RegisterTexture(const std::string& path, std::function<void()> reloadFunc);
		void UnregisterTexture(const std::string& path);
		void ReloadTexture(const std::string& path);
		void ReloadAllTextures();

		void SetHotReloadEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsHotReloadEnabled() const { return m_Enabled; }

	private:
		TextureReloadManager() = default;

		std::unordered_map<std::string, std::function<void()>> m_TextureReloadFuncs;
		std::atomic<bool> m_Enabled{ true };
	};
}
