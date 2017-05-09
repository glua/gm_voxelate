#include "vox_engine.h"

#include "vox_network.h"

#include <unordered_map>
#include <algorithm>

#include <bitbuf.h>

ENetAddress address;

#define VOX_NETWORK_LUA_CHANNEL 0
#define VOX_NETWORK_CPP_CHANNEL 1

#ifdef VOXELATE_SERVER
ENetHost* server;
int nextPeerID = 0;
std::unordered_map<int,ENetPeer*> peers;

#define VOX_ENET_HOST server
#else
ENetHost* client;
ENetPeer* peer;

#define VOX_ENET_HOST client
#endif

#include <string>
#include <thread>
#include <mutex>

std::thread* eventLoopThread;
void networkEventLoop();

struct PeerData {
	int peerID;
	std::string steamID;
};

#include "GarrysMod\LuaHelpers.hpp"

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

	eventLoopThread = new std::thread(networkEventLoop);
#else
	client = enet_host_create(NULL, 1, 2, 0, 0);

	if (client == NULL) {
		Msg("ENet initialization failed at client creation...\n");

		return false;
	}
#endif

	return true;
}

bool terminated = false;

void network_shutdown() {
	terminated = true;

#ifdef VOXELATE_SERVER
	enet_host_destroy(server);
#else
	enet_peer_disconnect(peer, 0);

	bool disconnected = false;

	ENetEvent event;

	time_t startTime, endTime;
	
	startTime = time(NULL);

	do {
		endTime = time(NULL);
		if (enet_host_service(client, &event, 3000) > 0) {
			if (event.type == ENET_EVENT_TYPE_RECEIVE) {
				enet_packet_destroy(event.packet);
			}
			else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
				Msg("Successfully disconnected from ENet.\n");
				disconnected = true;

				break;
			}
		}
		else {
			break;
		}
	} while (difftime(endTime, startTime) <= 3.0); // loop for 3 seconds max

	if (!disconnected) {
		Msg("Forcefully disconnected from ENet...\n");
		enet_peer_reset(peer);
	}

	enet_host_destroy(client);
#endif

	enet_deinitialize();
}

// thank https://github.com/danielga/gm_enet/blob/master/source/main.cpp#L12
// modified to disable port numbers and ENET_HOST_ANY
static bool ParseAddress(lua_State *state, const char *addr, ENetAddress &outputAddress) {
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
	else if (enet_address_set_host(&outputAddress, host_str.c_str()) != 0)
	{
		LUA->PushNil();
		LUA->PushString("failed to resolve host name");
		return false;
	}

	return true;
}

int lua_network_sendpacket(lua_State* state) {
	auto Ldata = luaL_checkstring(state, 1);
	int size = luaL_checkinteger(state, 2);
	int unreliable = lua_toboolean(state, 3);

	std::string* data = new std::string(Ldata, size);

#ifdef VOXELATE_SERVER
	unsigned int peerID = luaL_checkinteger(state, 4);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;
#endif

	// this is probably exploitable :weary:

	ENetPacket* packet = enet_packet_create(data->c_str(), size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

	enet_peer_send(peer, VOX_NETWORK_LUA_CHANNEL, packet);

	enet_host_flush(VOX_ENET_HOST);

	return 0;
}

#ifdef VOXELATE_CLIENT
int lua_network_connect(lua_State* state) {
	ENetEvent event;

	std::string addrStr = luaL_checkstring(state, 1);
	if (!ParseAddress(state, addrStr.c_str(), address)) {
		return 2;
	}

	address.port = VOX_NETWORK_PORT;

	peer = enet_host_connect(client, &address, 2, 0);
	if (peer == NULL) {
		lua_pushnil(state);
		lua_pushstring(state, "No available peers for initiating an ENet connection.\n");
		return 2;
	}

	if (enet_host_service(client, &event, 15000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT) {

		auto peerData = new PeerData();

		peerData->steamID = "STEAM:0:0";
		peerData->peerID = 0;

		peer->data = peerData;

		eventLoopThread = new std::thread(networkEventLoop);

		eventLoopThread->detach();

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

std::vector<ENetEvent *> events;
std::mutex eventMutex;

void networkEventLoop() {
	/* Wait up to 1000 milliseconds for an event. */

	while (!terminated) {
		auto event = new ENetEvent();
		if (enet_host_service(VOX_ENET_HOST, event, 1000) > 0) {
			eventMutex.lock();
			events.push_back(event);
			eventMutex.unlock();
		}
		else {
			delete event;
		}
	}
}

#ifdef VOXELATE_SERVER
int lua_network_disconnectPeer(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	enet_peer_disconnect(peer, 0);

	return 0;
}

int lua_network_resetPeer(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
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

	return 0;
}

int lua_network_getPeerSteamID(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	auto peerData = (PeerData*)peer->data;

	lua_pushstring(state, peerData->steamID.c_str());

	return 1;
}

int lua_network_setPeerSteamID(lua_State* state) {
	unsigned int peerID = luaL_checkinteger(state, 1);

	auto it = peers.find(peerID);

	if (it == peers.end()) {
		lua_pushstring(state, "can't find peer");
		lua_error(state);
		return 0;
	}

	auto peer = it->second;

	auto peerData = (PeerData*)peer->data;

	peerData->steamID = luaL_checkstring(state, 2);

	return 0;
}
#endif

std::unordered_map<int, std::function<void(int peerID, void* data, int size)>> cppChannelCallbacks;

int lua_network_pollForEvents(lua_State* state) {
	eventMutex.lock();
	for(auto event : events) {
		PeerData* peerData;
		std::string data;

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
				data.assign((const char*)event->packet->data, event->packet->dataLength);

				if (event->channelID == VOX_NETWORK_LUA_CHANNEL) {
					LuaHelpers::PushHookRun(state->luabase, "VoxNetworkPacket");

					lua_pushnumber(state, peerData->peerID);
					lua_pushlstring(state, data.c_str(), event->packet->dataLength);
					lua_pushnumber(state, event->channelID);

					LuaHelpers::CallHookRun(state->luabase, 3, 0);
				}
				else if (event->channelID == VOX_NETWORK_CPP_CHANNEL) {
					bf_read reader;

					reader.StartReading(event->packet->data, event->packet->dataLength);

					int channelID = reader.ReadUBitLong(16);
					
					if (cppChannelCallbacks[channelID]) {
						cppChannelCallbacks[channelID](peerData->peerID, event->packet->data, event->packet->dataLength);
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
	void channelListen(uint16_t channelID, std::function<void(int peerID, void* data, int size)> callback) {
		cppChannelCallbacks[channelID] = callback;
	}

#ifdef VOXELATE_SERVER
	bool channelSend(int peerID,uint16_t channelID, void* data, int size, bool unreliable) {
#else
	bool channelSend(uint16_t channelID, void* data, int size, bool unreliable) {
#endif
		bf_write writer;

		writer.StartWriting(data, size + 2);

		writer.WriteUBitLong(channelID, 16);
		writer.WriteBytes(data, size);

#ifdef VOXELATE_SERVER
		auto it = peers.find(peerID);

		if (it == peers.end()) {
			return false;
		}

		auto peer = it->second;
#endif

		ENetPacket* packet = enet_packet_create(data, size, unreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);

		enet_peer_send(peer, VOX_NETWORK_CPP_CHANNEL, packet);

		enet_host_flush(VOX_ENET_HOST);

		return true;
	}
}
