#pragma once

#ifndef VOX_NETWORK_HEADER
#define VOX_NETWORK_HEADER

#define VOX_NETWORK_PORT 42069

#include "enet/enet.h"
#include "glua.h"

bool network_startup();

void network_shutdown();

void setupLuaNetworking(lua_State* state);

#endif
