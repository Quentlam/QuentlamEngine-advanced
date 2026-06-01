#pragma once
#include "Quentlam/Core/Base.h"
#include <functional>
#include <vector>
#include <string>
#include <queue>

namespace Quentlam
{

enum class ENetworkEventType
{
	Connect,
	Disconnect,
	Message
};

struct NetworkEvent
{
	ENetworkEventType Type;
	uint64_t ClientId = 0;
	std::string Message;
};

using NetworkCallback = std::function<void(const NetworkEvent&)>;

class NetworkManager
{
public:
	static NetworkManager& Get();

	bool IsServer() const { return m_IsServer; }
	bool IsClient() const { return !m_IsServer && m_Connected; }
	bool IsConnected() const { return m_Connected; }

	bool StartServer(int port = 7777);
	bool Connect(const std::string& host, int port = 7777);
	void Disconnect();

	void Send(const std::string& message, bool reliable = true);
	void Broadcast(const std::string& message, bool reliable = true);

	void SetEventCallback(NetworkCallback callback);

	void PollEvents();
	bool HasEvent() const;
	NetworkEvent PopEvent();

private:
	NetworkManager() = default;
	~NetworkManager() = default;

	bool m_IsServer = false;
	bool m_Connected = false;
	uint64_t m_ServerId = 0;
	std::vector<NetworkCallback> m_Callbacks;
	std::queue<NetworkEvent> m_EventQueue;
};

}
