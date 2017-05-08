#ifndef VOX_NETWORK_HEADER
#define VOX_NETWORK_HEADER

#define VOX_NETWORK_PORT 42069

#include "enet/enet.h"
#include "glua.h"

ENetAddress address;

#ifdef VOXELATE_SERVER
ENetHost* server;
#else
ENetHost* client;
ENetPeer* peer;
#endif

bool network_startup();

void network_shutdown();

void setupLuaNetworking(lua_State* state);

#endif
