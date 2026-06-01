#include "qlpch.h"
#include "Quentlam/Network/NetworkManager.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{

NetworkManager& NetworkManager::Get()
{
	static NetworkManager instance;
	return instance;
}

bool NetworkManager::StartServer(int port)
{
	m_IsServer = true;
	m_Connected = true;
	QL_CORE_INFO("NetworkManager: Started server on port {}", port);
	return true;
}

bool NetworkManager::Connect(const std::string& host, int port)
{
	m_Connected = true;
	QL_CORE_INFO("NetworkManager: Connected to {}:{}", host, port);
	return true;
}

void NetworkManager::Disconnect()
{
	if (m_IsServer)
	{
		QL_CORE_INFO("NetworkManager: Server stopped.");
	}
	else
	{
		QL_CORE_INFO("NetworkManager: Disconnected from server.");
	}
	m_IsServer = false;
	m_Connected = false;
}

void NetworkManager::Send(const std::string& message, bool reliable)
{
	if (!m_Connected)
	{
		QL_CORE_WARN("NetworkManager::Send: not connected.");
		return;
	}
	QL_CORE_TRACE("NetworkManager::Send [reliable={}]: {}", reliable, message);
}

void NetworkManager::Broadcast(const std::string& message, bool reliable)
{
	if (!m_IsServer)
	{
		QL_CORE_WARN("NetworkManager::Broadcast: not a server.");
		return;
	}
	QL_CORE_TRACE("NetworkManager::Broadcast [reliable={}]: {}", reliable, message);
}

void NetworkManager::SetEventCallback(NetworkCallback callback)
{
	m_Callbacks.push_back(callback);
}

void NetworkManager::PollEvents()
{
}

bool NetworkManager::HasEvent() const
{
	return !m_EventQueue.empty();
}

NetworkEvent NetworkManager::PopEvent()
{
	if (!m_EventQueue.empty())
	{
		auto evt = m_EventQueue.front();
		m_EventQueue.pop();
		return evt;
	}
	return {};
}

}
