#include <stdio.h>

#include "GarrysMod/Lua/Interface.h"

#include "vox_util.h"
#include "vox_engine.h"
#include "vox_lua_bridge.h"
#include "vox_Voxels.h"


using namespace std;

const char* VERSION = "0.1.0";

GMOD_MODULE_OPEN() {
	if (!determine_state(state)) {
		vox_print("Fatal! Could not determine the engine state!");
		return 1;
	}
	
	if (!init_interfaces()) {
		vox_print("Fatal! Failed to load engine interfaces!");
		return 1;
	}

	init_lua(state, VERSION);

	if (STATE_CLIENT)
		vox_print("Module [%s] loaded on Client.",VERSION);
	else
		vox_print("Module [%s] loaded on Server.",VERSION);

	return 0;
}

GMOD_MODULE_CLOSE() {
	if (STATE_CLIENT) {
		deleteAllIndexedVoxels();
	}
	vox_print("Module unloaded.");
	return 0;
}