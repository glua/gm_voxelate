#include <stdio.h>

#include "glua.h"

#include "vox_util.h"
#include "vox_engine.h"
#include "vox_lua_api.h"
#include "vox_voxelworld.h"
#include "vox_network.h"
#include "vox_shaders.h"

#include "sn_bf_read.hpp"
#include "sn_bf_write.hpp"
#include "sn_ucharptr.hpp"

// Bump version on releases, not at random.
// Major: Increase on major protocol or API changes. Try to avoid this.
// Minor: Increase on added features.
// Patch: Increase on any other release. (Bug fixes, performance improvements...)

const char* VERSION = "0.3.0";

GMOD_MODULE_OPEN() {

	// Load the engine interfaces we need.
	if (!init_interfaces()) {
		vox_print("Fatal! Failed to load engine interfaces!");
		return 1;
	}

	// Sets up ENet.
	if (!vox_network_startup()) {
		vox_print("Fatal! Failed to setup networking!");
		return 1;
	}

	// Detours shader lookup function so our shader works.
	if (!installShaders()) {
		vox_print("Fatal! Failed to setup shaders!");
		return 1;
	}

	voxelworld_initialise_networking_static();

	// Expose buffers to lua.
	// TODO what is UCHARPTR and what is it used for?
	UCHARPTR::Initialize(LUA);
	sn_bf_read::Initialize(LUA);
	sn_bf_write::Initialize(LUA);

	vox_init_lua_api(LUA, VERSION);

	vox_print("Module initialization complete. [Version %s]",VERSION);

	return 0;
}

GMOD_MODULE_CLOSE() {

	checkAllVoxelWorldsDeleted();

	uninstallShaders();

	vox_network_shutdown();

	sn_bf_write::Deinitialize(LUA);
	sn_bf_read::Deinitialize(LUA);
	UCHARPTR::Deinitialize(LUA);

	vox_print("Module unloaded.");
	return 0;
}