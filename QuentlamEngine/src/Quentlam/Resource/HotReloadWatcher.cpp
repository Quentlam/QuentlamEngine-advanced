#include "qlpch.h"
#include "Quentlam/Resource/HotReloadWatcher.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Resource/ResourceManager.h"
#include <chrono>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Quentlam
{
	HotReloadWatcher::HotReloadWatcher()
	{
	}

	HotReloadWatcher::~HotReloadWatcher()
	{
		Stop();
	}

	void HotReloadWatcher::Start(const std::string& watchDirectory)
	{
		if (m_IsRunning.load())
		{
			QL_CORE_WARN("[HotReload] Watcher is already running");
			return;
		}

		if (!std::filesystem::exists(watchDirectory))
		{
			QL_CORE_WARN("[HotReload] Watch directory does not exist: {0}", watchDirectory);
			return;
		}

		m_WatchDirectory = watchDirectory;
		m_IsRunning.store(true);

		m_WatchThread = std::thread([this]() { WatchLoop(); });
		QL_CORE_INFO("[HotReload] Started watching: {0}", watchDirectory);
	}

	void HotReloadWatcher::Stop()
	{
		if (!m_IsRunning.load())
			return;

		m_IsRunning.store(false);

		if (m_WatchThread.joinable())
			m_WatchThread.join();

		QL_CORE_INFO("[HotReload] Stopped watcher");
	}

	void HotReloadWatcher::SetPollInterval(int32_t milliseconds)
	{
		m_PollIntervalMs.store(std::max(100, milliseconds));
	}

	void HotReloadWatcher::RegisterCallback(const std::string& extension, std::function<void(const std::string&)> callback)
	{
		m_ExtensionCallbacks[extension] = callback;
		QL_CORE_TRACE("[HotReload] Registered callback for .{0} files", extension);
	}

	void HotReloadWatcher::UnregisterCallback(const std::string& extension)
	{
		m_ExtensionCallbacks.erase(extension);
	}

	void HotReloadWatcher::ClearCallbacks()
	{
		m_ExtensionCallbacks.clear();
	}

	void HotReloadWatcher::AddIgnorePattern(const std::string& pattern)
	{
		m_IgnorePatterns.push_back(pattern);
	}

	void HotReloadWatcher::ClearIgnorePatterns()
	{
		m_IgnorePatterns.clear();
	}

	void HotReloadWatcher::RegisterResource(const std::string& filepath, std::function<void()> reloadFunc)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_ResourceReloadFuncs[filepath] = reloadFunc;

		try
		{
			auto lastWrite = std::filesystem::last_write_time(filepath);
			m_LastKnownTimes[filepath] = std::chrono::duration_cast<std::chrono::milliseconds>(
				lastWrite.time_since_epoch()).count();
		}
		catch (...)
		{
			m_LastKnownTimes[filepath] = 0;
		}
	}

	void HotReloadWatcher::UnregisterResource(const std::string& filepath)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_ResourceReloadFuncs.erase(filepath);
		m_LastKnownTimes.erase(filepath);
	}

	void HotReloadWatcher::ClearResources()
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_ResourceReloadFuncs.clear();
		m_LastKnownTimes.clear();
	}

	void HotReloadWatcher::TriggerReload(const std::string& filepath)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);

		auto it = m_ResourceReloadFuncs.find(filepath);
		if (it != m_ResourceReloadFuncs.end())
		{
			QL_CORE_INFO("[HotReload] Triggering reload for: {0}", filepath);
			it->second();
		}

		auto extIt = m_ExtensionCallbacks.end();
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos != std::string::npos)
		{
			std::string ext = filepath.substr(dotPos + 1);
			extIt = m_ExtensionCallbacks.find(ext);
		}

		if (extIt != m_ExtensionCallbacks.end())
		{
			extIt->second(filepath);
		}

		if (OnReloadTriggered)
			OnReloadTriggered(filepath);
	}

	void HotReloadWatcher::TriggerReloadAll()
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		QL_CORE_INFO("[HotReload] Triggering reload for all {0} resources", m_ResourceReloadFuncs.size());
		for (auto& [filepath, func] : m_ResourceReloadFuncs)
		{
			func();
		}
	}

	size_t HotReloadWatcher::GetPendingReloadCount() const
	{
		std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_QueueMutex));
		return m_PendingReloadQueue.size();
	}

	size_t HotReloadWatcher::GetWatchedResourceCount() const
	{
		std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_QueueMutex));
		return m_ResourceReloadFuncs.size();
	}

	void HotReloadWatcher::WatchLoop()
	{
		while (m_IsRunning.load())
		{
			{
				std::vector<std::string> changedFiles;

				{
					std::lock_guard<std::mutex> lock(m_QueueMutex);
					for (auto& [filepath, lastTime] : m_LastKnownTimes)
					{
						if (!std::filesystem::exists(filepath))
							continue;

						try
						{
							auto lastWrite = std::filesystem::last_write_time(filepath);
							int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
								lastWrite.time_since_epoch()).count();

							if (currentTime > lastTime)
							{
								lastTime = currentTime;
								changedFiles.push_back(filepath);
							}
						}
						catch (...) {}
					}
				}

				for (const auto& filepath : changedFiles)
				{
					QL_CORE_INFO("[HotReload] File changed: {0}", filepath);
					if (OnFileChanged)
						OnFileChanged(filepath);
					TriggerReload(filepath);
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(m_PollIntervalMs.load()));
		}
	}

	bool HotReloadWatcher::CheckFileChanged(const std::string& filepath, int64_t lastModTime)
	{
		try
		{
			if (!std::filesystem::exists(filepath))
				return false;

			auto lastWrite = std::filesystem::last_write_time(filepath);
			int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
				lastWrite.time_since_epoch()).count();

			return currentTime > lastModTime;
		}
		catch (...)
		{
			return false;
		}
	}

	ResourceReloadGuard::~ResourceReloadGuard()
	{
		Unlock();
	}

	void ResourceReloadGuard::Lock()
	{
		m_Locked.store(true);
	}

	void ResourceReloadGuard::Unlock()
	{
		m_Locked.store(false);
	}

	TextureReloadManager& TextureReloadManager::Get()
	{
		static TextureReloadManager instance;
		return instance;
	}

	void TextureReloadManager::RegisterTexture(const std::string& path, std::function<void()> reloadFunc)
	{
		m_TextureReloadFuncs[path] = reloadFunc;
	}

	void TextureReloadManager::UnregisterTexture(const std::string& path)
	{
		m_TextureReloadFuncs.erase(path);
	}

	void TextureReloadManager::ReloadTexture(const std::string& path)
	{
		if (!m_Enabled.load())
			return;

		auto it = m_TextureReloadFuncs.find(path);
		if (it != m_TextureReloadFuncs.end())
		{
			QL_CORE_INFO("[TextureReload] Reloading: {0}", path);
			it->second();
		}
	}

	void TextureReloadManager::ReloadAllTextures()
	{
		if (!m_Enabled.load())
			return;

		QL_CORE_INFO("[TextureReload] Reloading all {0} textures", m_TextureReloadFuncs.size());
		for (auto& [path, func] : m_TextureReloadFuncs)
		{
			func();
		}
	}
}
