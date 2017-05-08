#include "vox_engine.h"

#include "vox_network.h"

#include <string>

bool network_startup() {
	if (enet_initialize() != 0) {
		Msg("ENet initialization failed...\n");

		return false;
	}

	address.host = ENET_HOST_ANY;
	address.port = VOX_NETWORK_PORT;

#ifdef VOXELATE_SERVER
	server = enet_host_create(&address, 128, 2, 0, 0);

	if (server == NULL) {
		Msg("ENet initialization failed at server creation...\n");

		return false;
}
#else
	client = enet_host_create(NULL, 1, 2, 0, 0);

	if (client == NULL) {
		Msg("ENet initialization failed at client creation...\n");

		return false;
	}
#endif

	return true;
}

void network_shutdown() {
	enet_deinitialize();
}

// thank https://github.com/danielga/gm_enet/blob/master/source/main.cpp#L12
// modified to disable port numbers and ENET_HOST_ANY
static bool ParseAddress(lua_State *state, const char *addr, ENetAddress &address) {
	std::string addr_str = addr, host_str;
	size_t pos = addr_str.find(':');
	if (pos != addr_str.npos)
	{
		LUA->PushNil();
		LUA->PushString("port number not allowed");
		return false;
	}
	else {
		host_str = addr_str;
	}

	if (host_str.empty())
	{
		LUA->PushNil();
		LUA->PushString("invalid host name");
		return false;
	}

	if (host_str == "*") {
		LUA->PushNil();
		LUA->PushString("wildcard host name disabled");
		return false;
	}
	else if (enet_address_set_host(&address, host_str.c_str()) != 0)
	{
		LUA->PushNil();
		LUA->PushString("failed to resolve host name");
		return false;
	}

	return true;
}

int lua_network_newpacket(lua_State* state) {

}

#ifdef VOXELATE_CLIENT
int lua_network_connect(lua_State* state) {
	ENetEvent event;

	std::string addrStr = luaL_checkstring(state, 1);
	if (!ParseAddress(state, addrStr.c_str(), address)) {
		return 2;
	}

	peer = enet_host_connect(client, &address, 2, 0);
	if (peer == NULL) {
		lua_pushnil(state);
		lua_pushstring(state, "No available peers for initiating an ENet connection.\n");
		return 2;
	}

	if (enet_host_service(client, &event, 5000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT) {

		lua_pushboolean(state, true);
		return 1;
	}
	else {
		enet_peer_reset(peer);

		lua_pushnil(state);
		lua_pushstring(state, "Connection to server failed.");

		return 2;
	}
}
#endif

void setupLuaNetworking(lua_State* state) {
#ifdef VOXELATE_CLIENT
	lua_pushcfunction(state, lua_network_connect);
#endif

}
