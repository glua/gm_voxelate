#include <stdio.h>

#include "glua.h"

#include "vox_util.h"
#include "vox_engine.h"
#include "vox_lua_bridge.h"
#include "vox_voxelworld.h"
#include "vox_network.h"

#include "sn_bf_read.hpp"
#include "sn_bf_write.hpp"
#include "sn_ucharptr.hpp"

const char* VERSION = "0.2.0+";

GMOD_MODULE_OPEN() {

	if (!init_interfaces()) {
		vox_print("Fatal! Failed to load engine interfaces!");
		return 1;
	}

	if (!network_startup()) {
		return 0;
	}

	voxelworld_initialise_networking_static();

	UCHARPTR::Initialize(LUA);
	sn_bf_read::Initialize(LUA);
	sn_bf_write::Initialize(LUA);

	init_lua(state, VERSION);

	vox_print("Loaded module. [V%s]",VERSION);

	return 0;
}

GMOD_MODULE_CLOSE() {

	if (!IS_SERVERSIDE)
		deleteAllIndexedVoxelWorlds();

	network_shutdown();

	sn_bf_write::Deinitialize(LUA);
	sn_bf_read::Deinitialize(LUA);
	UCHARPTR::Deinitialize(LUA);

	vox_print("Unloaded module.");
	return 0;
}