#include <stdio.h>

#include "glua.h"

#include "vox_util.h"
#include "vox_engine.h"
#include "vox_lua_bridge.h"
#include "vox_Voxels.h"

#include "vox_network.h"

const char* VERSION = "0.2.0+";

GMOD_MODULE_OPEN() {

	if (!init_interfaces()) {
		vox_print("Fatal! Failed to load engine interfaces!");
		return 1;
	}

	if (!network_startup()) {
		return 0;
	}

	init_lua(state, VERSION);

	vox_print("Loaded module. [V%s]",VERSION);

	return 0;
}

GMOD_MODULE_CLOSE() {

	if (!IS_SERVERSIDE)
		deleteAllIndexedVoxels();

	network_shutdown();

	vox_print("Unloaded module.");
	return 0;
}