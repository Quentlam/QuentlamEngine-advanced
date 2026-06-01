#pragma once
#include "Quentlam/Core/Base.h"
#include <string>
#include <cstdint>

namespace Quentlam
{

enum class ENetworkRole
{
	None = 0,
	Authority = 1,
	Proxy = 2
};

struct NetworkIdentityComponent
{
	uint64_t NetworkId = 0;
	ENetworkRole LocalRole = ENetworkRole::None;
	bool IsLocalPlayer = false;
	bool IsSpawned = false;
};

}
