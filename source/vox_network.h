#pragma once

// TODO allow the server to specify a port.
#define VOX_NETWORK_PORT 42069

// This whole fuckin thing is gonna need to be changed.
// All world data should run over the same channel, so will probably just
// add my own channel/messge type field on top of ENet...

#include "enetpp/client.h"
#include "enetpp/server.h"

#include "glua.h"

// looks like we have a max of 255 chans
#define VOX_NETWORK_CPP_CHANNEL_START 128
static enet_uint8 VOX_NETWORK_MAX_CHANNELS = 255;

#include <functional>
#include <string>

#include <bitbuf.h>

typedef std::function<void(int peerID, const char* data, size_t data_len)> networkCallback;

#define VOX_NETWORK_CHANNEL_CHUNKDATA_SINGLE 1
#define VOX_NETWORK_CHANNEL_CHUNKDATA_RADIUS 2

bool network_startup();

void network_shutdown();

void setupLuaNetworking(GarrysMod::Lua::ILuaBase *LUA);

namespace networking {
#ifdef VOXELATE_SERVER
	bool channelSend(int peerID, uint16_t channelID, void* data, int size, bool unreliable = false);
#else
	bool channelSend(uint16_t channelID, void* data, int size, bool unreliable = false);
#endif
	void channelListen(uint16_t channelID, networkCallback callback);
};
