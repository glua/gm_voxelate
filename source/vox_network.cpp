#include "vox_engine.h"

#include "vox_network.h"

#include <unordered_map>
#include <algorithm>

#ifdef VOXELATE_SERVER
int nextPeerID = 0;
std::unordered_map<int,ENetPeer*> peers;

struct server_client {
	unsigned int peerID;
	std::string steamID;
	unsigned int get_uid() const { return peerID; }
};

enetpp::server<server_client> server;
#else
// ENetHost* client;
// ENetPeer* peer;

enetpp::client client;
#endif

#include <string>
#include <chrono>
#include <thread>
#include <mutex>

std::mutex enetMutex;
std::thread* eventLoopThread;
void networkEventLoop();

struct PeerData {
	int peerID;
	std::string steamID;
};

#include "GarrysMod\LuaHelpers.hpp"

bool network_startup() {
	enetpp::global_state::get().initialize();

#ifdef VOXELATE_SERVER
	auto init_client_func = [&](server_client& client, const char* ip) {
		client.peerID = nextPeerID;
		nextPeerID++;
	};

	server.start_listening(enetpp::server_listen_params<server_client>()
		.set_max_client_count(128)
		.set_channel_count(VOX_NETWORK_MAX_CHANNELS)
		.set_listen_port(VOX_NETWORK_PORT)
		.set_initialize_client_function(init_client_func));

	eventLoopThread = new std::thread(networkEventLoop);
#endif

	return true;
}

bool terminated = false;

void network_shutdown() {
	terminated = true;

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
	int unreliable = lua_toboolean(state, 3);

	std::string* data = new std::string(Ldata, size);

	if (channel < 0 || channel >= VOX_NETWORK_CPP_CHANNEL_START) {
		lua_pushstring(state, "attempt to send packet on bad channel");
		lua_error(state);
	}

	enetMutex.lock();
#ifdef VOXELATE_SERVER
	unsigned int peerID = luaL_checkinteger(state, 4);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		enetMutex.unlock();
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;
#endif

	if (peer == NULL) {
		enetMutex.unlock();
#ifdef VOXELATE_CLIENT
		lua_pushstring(state, "attempt to send packet before network init");
#else
		lua_pushstring(state, "attempt to send packet to null peer");
#endif
		lua_error(state);
		return 0;
	}

	// this is probably exploitable :weary:

	ENetPacket* packet = enet_packet_create(data->c_str(), size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

	if (enet_peer_send(peer, static_cast<enet_uint8>(channel), packet) != 0) {
		enet_packet_destroy(packet);

		enetMutex.unlock();

		lua_pushstring(state, "couldn't send packet");
		lua_error(state);
		return 0;
	}

	enet_host_flush(VOX_ENET_HOST);

	enetMutex.unlock();

	enet_packet_destroy(packet);

	return 0;
}

#ifdef VOXELATE_CLIENT
int lua_network_connect(lua_State* state) {
	ENetEvent event;

	std::string addrStr = luaL_checkstring(state, 1);

	auto params = enetpp::client_connect_params();

	params.set_channel_count(VOX_NETWORK_MAX_CHANNELS);
	params.set_server_host_name_and_port(addrStr.c_str(), VOX_NETWORK_PORT);
	params.set_incoming_bandwidth(0);
	params.set_outgoing_bandwidth(0);

	client.connect(params);
}
#endif

#ifdef VOXELATE_SERVER
int lua_network_disconnectPeer(lua_State* state) {
	enetMutex.lock();
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		enetMutex.unlock();

		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	enet_peer_disconnect(peer, 0);
	enetMutex.unlock();

	return 0;
}

int lua_network_resetPeer(lua_State* state) {
	enetMutex.lock();
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		enetMutex.unlock();

		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	enet_peer_reset(peer);

	for (auto it = peers.cbegin(); it != peers.cend(); ) {
		if (peer == it->second) {
			peers.erase(it++);
		}
		else {
			++it;
		}
	}

	enetMutex.unlock();

	return 0;
}

int lua_network_getPeerSteamID(lua_State* state) {
	enetMutex.lock();
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		enetMutex.unlock();
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	auto peerData = (PeerData*)peer->data;

	lua_pushstring(state, peerData->steamID.c_str());

	enetMutex.unlock();

	return 1;
}

int lua_network_setPeerSteamID(lua_State* state) {
	enetMutex.lock();
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		enetMutex.unlock();

		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	auto peerData = (PeerData*)peer->data;

	peerData->steamID = luaL_checkstring(state, 2);

	enetMutex.unlock();

	return 0;
}
#endif

std::unordered_map<int, networkCallback> cppChannelCallbacks;

int lua_network_pollForEvents(lua_State* state) {
	eventMutex.lock();
	for(auto event : events) {
		PeerData* peerData;

		switch (event->type) {
		case ENET_EVENT_TYPE_CONNECT:
			peerData = new PeerData();

#ifdef VOXELATE_SERVER
			nextPeerID++;
			peers[nextPeerID] = event->peer;

			peerData->steamID = "STEAM:0:0";
			peerData->peerID = nextPeerID;
#else
			// SHOULD NEVER BE CALLED CLIENTSIDE?!?!?!
			peerData->steamID = "STEAM:0:0";
			peerData->peerID = 0;
#endif

			event->peer->data = peerData;

			LuaHelpers::PushHookRun(state->luabase, "VoxNetworkConnect");

			lua_pushnumber(state, peerData->peerID);
			lua_pushnumber(state, event->peer->address.host);

			LuaHelpers::CallHookRun(state->luabase, 2, 0);
			
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			peerData = (PeerData*)event->peer->data;

			if (peerData != NULL) {

				if (event->channelID < VOX_NETWORK_CPP_CHANNEL_START) {
					LuaHelpers::PushHookRun(state->luabase, "VoxNetworkPacket");

					lua_pushnumber(state, peerData->peerID);
					lua_pushnumber(state, event->channelID);
					lua_pushlstring(state, reinterpret_cast<char*>(event->packet->data), event->packet->dataLength);

					LuaHelpers::CallHookRun(state->luabase, 3, 0);
				}
				else {

					int channelID = event->channelID - VOX_NETWORK_CPP_CHANNEL_START;
					
					if (cppChannelCallbacks.find(channelID) != cppChannelCallbacks.end()) {
						cppChannelCallbacks[channelID](peerData->peerID, reinterpret_cast<char*>(event->packet->data), event->packet->dataLength);
					}
				}

				enet_packet_destroy(event->packet);
			}
			else {
				Msg("ENET_EVENT_TYPE_RECEIVE: event->peer->data is null?????\n");
			}

			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			peerData = (PeerData*)event->peer->data;

			if (peerData != NULL) {
				LuaHelpers::PushHookRun(state->luabase, "VoxNetworkDisconnect");

				lua_pushnumber(state, peerData->peerID);

				LuaHelpers::CallHookRun(state->luabase, 1, 0);

				event->peer->data = NULL;
			}
			else {
				Msg("ENET_EVENT_TYPE_DISCONNECT: event->peer->data is null?????\n");
			}

#ifdef VOXELATE_SERVER
			for (auto it = peers.cbegin(); it != peers.cend(); ) {
				if (event->peer == it->second) {
					peers.erase(it++);
				}
				else {
					++it;
				}
			}
#endif
		}
	}

	events.clear();

	eventMutex.unlock();

	return 0;
}

void setupLuaNetworking(lua_State* state) {
#ifdef VOXELATE_CLIENT
	lua_pushcfunction(state, lua_network_connect);
	lua_setfield(state, -2, "networkConnect");
#endif

	lua_pushcfunction(state, lua_network_pollForEvents);
	lua_setfield(state, -2, "networkPoll");

#ifdef VOXELATE_SERVER
	lua_pushcfunction(state, lua_network_disconnectPeer);
	lua_setfield(state, -2, "networkDisconnectPeer");

	lua_pushcfunction(state, lua_network_resetPeer);
	lua_setfield(state, -2, "networkResetPeer");

	lua_pushcfunction(state, lua_network_setPeerSteamID);
	lua_setfield(state, -2, "networkSetPeerSteamID");

	lua_pushcfunction(state, lua_network_getPeerSteamID);
	lua_setfield(state, -2, "networkGetPeerSteamID");
#endif

	lua_pushcfunction(state, lua_network_sendpacket);
	lua_setfield(state, -2, "networkSendPacket");
}

namespace networking {
	void channelListen(uint16_t channelID, networkCallback callback) {
		cppChannelCallbacks[channelID] = callback;
	}

#ifdef VOXELATE_SERVER
	bool channelSend(int peerID,uint16_t channelID, void* data, int size, bool unreliable) {
#else
	bool channelSend(uint16_t channelID, void* data, int size, bool unreliable) {
#endif

		enetMutex.lock();

#ifdef VOXELATE_SERVER
		auto it = peers.find(peerID);

		if (it == peers.end()) {
			enetMutex.unlock();
			return false;
		}

		auto peer = it->second;
#else
		if (peer == NULL) {
			enetMutex.unlock();
			return false;
		}
#endif

		ENetPacket* packet = enet_packet_create(data, size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

		enet_peer_send(peer, VOX_NETWORK_CPP_CHANNEL_START + channelID, packet);

		enet_host_flush(VOX_ENET_HOST);

		enet_packet_destroy(packet);

		enetMutex.unlock();

		return true;
	}
}
