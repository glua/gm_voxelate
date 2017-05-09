#pragma once

#ifndef VOX_NETWORK_HEADER
#define VOX_NETWORK_HEADER

#define VOX_NETWORK_PORT 42069

#include "enet/enet.h"
#include "glua.h"

#include <functional>
#include <string>

bool network_startup();

void network_shutdown();

void setupLuaNetworking(lua_State* state);

namespace networking {
	void channelSend(uint16_t channelID, std::string data);
	void channelListen(uint16_t channelID, std::function<void(int peerID,std::string data)> callback);
};

#endif
