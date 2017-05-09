#pragma once

#ifndef VOX_NETWORK_HEADER
#define VOX_NETWORK_HEADER

#define VOX_NETWORK_PORT 42069

#include "enet/enet.h"
#include "glua.h"

#include <functional>
#include <string>

#include <bitbuf.h>

typedef std::function<void(int peerID, bf_read reader)> networkCallback;

#define VOX_NETWORK_CHANNEL_CHUNKRADIUS_DATA 1

bool network_startup();

void network_shutdown();

void setupLuaNetworking(lua_State* state);

namespace networking {
#ifdef VOXELATE_SERVER
	bool channelSend(int peerID, uint16_t channelID, void* data, int size, bool unreliable = false);
#else
	bool channelSend(uint16_t channelID, void* data, int size, bool unreliable = false);
#endif
	void channelListen(uint16_t channelID, networkCallback callback);
};

#endif
