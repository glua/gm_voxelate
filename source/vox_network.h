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
static enet_uint8 VOX_NETWORK_MAX_CHANNELS = 1;

#include <functional>
#include <string>

#include <bitbuf.h>

typedef std::function<void(int peerID, const char* data, size_t data_len)> networkCallback;

enum vox_network_channel {
	chunk = 128
};

struct BaseNetworkMessage {
	uint8_t channelID;
};

bool vox_network_startup();

void vox_network_shutdown();

void vox_setupLuaNetworkingAPI(GarrysMod::Lua::ILuaBase *LUA);

namespace vox_networking {
#ifdef VOXELATE_SERVER
	bool channelSend(int peerID, void* data, int size, bool unreliable = false);
#else
	bool channelSend(void* data, int size, bool unreliable = false);
#endif
	void channelListen(uint16_t channelID, networkCallback callback);
};
