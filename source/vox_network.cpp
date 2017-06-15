//#include "vox_engine.h"

#include "vox_util.h"

#include "vox_network.h"

#include <unordered_map>
#include <algorithm>

#ifdef VOXELATE_SERVER
unsigned int nextPeerID = 0;
std::unordered_map<int,ENetPeer*> peers;

struct server_client {
	unsigned int peerID;
	std::string steamID;
	std::string ip;
	unsigned int get_id() const { return peerID; }
};

enetpp::server<server_client> server;
#define NW_CONTEXT server
#else
enetpp::client client;
#define NW_CONTEXT client
#endif

#include <string>
#include <chrono>
#include <thread>
#include <mutex>

#include "GarrysMod/LuaHelpers.hpp"

bool vox_network_startup() {
	enetpp::global_state::get().initialize();

	NW_CONTEXT.set_trace_handler([&](std::string err) {
		vox_print("[Net] %s", err.c_str());
	});

#ifdef VOXELATE_SERVER
	auto init_client_func = [&](server_client& client, const char* ip) {
		client.peerID = nextPeerID;
		client.ip = ip;
		nextPeerID++;
	};

	server.start_listening(enetpp::server_listen_params<server_client>()
		.set_max_client_count(128)
		.set_channel_count(VOX_NETWORK_MAX_CHANNELS)
		.set_listen_port(VOX_NETWORK_PORT)
		.set_initialize_client_function(init_client_func));
#endif


	return true;
}

//bool terminated = false;

void vox_network_shutdown() {
	//terminated = true;

#ifdef VOXELATE_SERVER
	server.stop_listening();
#else
	client.disconnect();
#endif

	enetpp::global_state::get().deinitialize();
}

int lua_network_sendpacket(lua_State* state) {
	size_t size;

	int channel = luaL_checknumber(state, 1);
	auto Ldata = luaL_checklstring(state, 2, &size);
	int unreliable = false; // im like a million percent positive we don't never want unreliable messages

	std::string* data = new std::string(Ldata, size);

	if (channel < 0 || channel >= VOX_NETWORK_CPP_CHANNEL_START) {
		lua_pushstring(state, "attempt to send packet on bad channel");
		lua_error(state);
	}

#ifdef VOXELATE_SERVER
	unsigned int peerID = luaL_checkinteger(state, 3);

	server.send_packet_to(peerID, channel, (const enet_uint8*)data->c_str(), size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);
#else
	client.send_packet(channel, (const enet_uint8*)data->c_str(), size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);
#endif

	return 0;
}

#ifdef VOXELATE_CLIENT
int lua_network_connect(lua_State* state) {
	std::string addrStr = luaL_checkstring(state, 1);

	auto params = enetpp::client_connect_params();

	params.set_channel_count(VOX_NETWORK_MAX_CHANNELS);
	params.set_server_host_name_and_port(addrStr.c_str(), VOX_NETWORK_PORT);
	params.set_incoming_bandwidth(0);
	params.set_outgoing_bandwidth(0);

	client.connect(params);

	return 0;
}
#endif

#ifdef VOXELATE_SERVER
int lua_network_disconnectPeer(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	server.disconnect_client(peerID, true);

	return 0;
}

/*int lua_network_resetPeer(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	server.disconnect_client(peerID, true);

	return 0;
}*/

/*int lua_network_getPeerSteamID(lua_State* state) {
	vox_print("get steamid?");
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto peer = server.get_client(peerID);

	if (peer == NULL) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	lua_pushstring(state, peer->steamID.c_str());

	return 1;
}

int lua_network_setPeerSteamID(lua_State* state) {
	vox_print("set steamid?");
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto peer = server.get_client(peerID);

	if (peer == NULL) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	peer->steamID = luaL_checkstring(state, 2);

	return 0;
}*/
#endif

std::unordered_map<int, networkCallback> cppChannelCallbacks;

int lua_network_pollForEvents(lua_State* state) {

#ifdef VOXELATE_SERVER
	auto on_connected = [&](server_client& client) {
		LuaHelpers::PushHookRun(state->luabase, "VoxNetworkConnect");

		lua_pushnumber(state, client.get_id());
		lua_pushstring(state, client.ip.c_str());

		LuaHelpers::CallHookRun(state->luabase, 2, 0);
	};
#else
	auto on_connected = [&]() {
		LuaHelpers::PushHookRun(state->luabase, "VoxNetworkConnect");

		LuaHelpers::CallHookRun(state->luabase, 0, 0);
	};
#endif


#ifdef VOXELATE_SERVER
	auto on_data_received = [&](server_client& client, enet_uint8 channelID, const enet_uint8* data, size_t data_size) {
		auto peerID = client.peerID;
#else
	auto on_data_received = [&](enet_uint8 channelID, const enet_uint8* data, size_t data_size) {
		unsigned int peerID = 0;
#endif
		if (channelID < VOX_NETWORK_CPP_CHANNEL_START) {
			LuaHelpers::PushHookRun(state->luabase, "VoxNetworkPacket");

			lua_pushnumber(state, peerID);
			lua_pushnumber(state, channelID);
			lua_pushlstring(state, (char*)data, data_size);

			LuaHelpers::CallHookRun(state->luabase, 3, 0);
		}
		else {

			int adjustedChannelID = channelID - VOX_NETWORK_CPP_CHANNEL_START;

			if (cppChannelCallbacks.find(adjustedChannelID) != cppChannelCallbacks.end()) {
				cppChannelCallbacks[adjustedChannelID](peerID, (char*)data, data_size);
			}
		}
	};


#ifdef VOXELATE_SERVER
	auto on_disconnected = [&](unsigned int peerID) {

		LuaHelpers::PushHookRun(state->luabase, "VoxNetworkDisconnect");

		lua_pushnumber(state, peerID);

		LuaHelpers::CallHookRun(state->luabase, 1, 0);
	};
#else
	auto on_disconnected = [&]() {

		LuaHelpers::PushHookRun(state->luabase, "VoxNetworkDisconnect");

		LuaHelpers::CallHookRun(state->luabase, 0, 0);
	};
#endif

	NW_CONTEXT.consume_events(on_connected, on_disconnected, on_data_received);

	return 0;
}

void vox_setupLuaNetworkingAPI(GarrysMod::Lua::ILuaBase *LUA) {
	lua_State* state = LUA->GetState();
#ifdef VOXELATE_CLIENT
	lua_pushcfunction(state, lua_network_connect);
	lua_setfield(state, -2, "networkConnect");
#endif

	lua_pushcfunction(state, lua_network_pollForEvents);
	lua_setfield(state, -2, "networkPoll");

#ifdef VOXELATE_SERVER
	lua_pushcfunction(state, lua_network_disconnectPeer);
	lua_setfield(state, -2, "networkDisconnectPeer");

	//lua_pushcfunction(state, lua_network_resetPeer);
	//lua_setfield(state, -2, "networkResetPeer");

	/*lua_pushcfunction(state, lua_network_setPeerSteamID);
	lua_setfield(state, -2, "networkSetPeerSteamID");

	lua_pushcfunction(state, lua_network_getPeerSteamID);
	lua_setfield(state, -2, "networkGetPeerSteamID");*/
#endif

	lua_pushcfunction(state, lua_network_sendpacket);
	lua_setfield(state, -2, "networkSendPacket");
}

namespace vox_networking {
	void channelListen(uint16_t channelID, networkCallback callback) {
		cppChannelCallbacks[channelID] = callback;
	}

#ifdef VOXELATE_SERVER
	bool channelSend(int peerID, uint16_t channelID, void* data, int size, bool unreliable) {

		channelID += VOX_NETWORK_CPP_CHANNEL_START;

		server.send_packet_to(peerID, channelID, (const enet_uint8*)data, size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

		return true;
	}
#else
	bool channelSend(uint16_t channelID, void* data, int size, bool unreliable) {

		channelID += VOX_NETWORK_CPP_CHANNEL_START;

		client.send_packet(channelID, (const enet_uint8*)data, size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

		return true;
	}
#endif

}
