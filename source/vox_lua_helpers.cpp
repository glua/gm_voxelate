#include "vox_lua_helpers.h"

inline VoxelWorld* luaL_checkvoxelworld(lua_State* state, int loc) {
	int index = luaL_checknumber(state, loc);

	auto world = getIndexedVoxelWorld(index);

	if (world != nullptr) {
		return world;
	}
	else {
		lua_pushfstring(state, "Invalid voxel world index [%d]!", index);
		lua_error(state);

		return NULL; // to keep the compiler happy
	}
}

inline VoxelCoordXYZ luaL_checkvoxelxyz(lua_State* state, int loc) {
	luaL_checktype(state, loc, LUA_TTABLE);

	lua_rawgeti(state, loc, 1);
	VoxelCoord x = preciseToNormal(luaL_checknumber(state, -1));
	lua_pop(state, 1);

	lua_rawgeti(state, loc, 2);
	VoxelCoord y = preciseToNormal(luaL_checknumber(state, -1));
	lua_pop(state, 1);

	lua_rawgeti(state, loc, 3);
	VoxelCoord z = preciseToNormal(luaL_checknumber(state, -1));
	lua_pop(state, 1);

	return { x,y,z };
}

inline PreciseVoxelCoordXYZ luaL_checkprecisevoxelxyz(lua_State* state, int loc) {
	luaL_checktype(state, loc, LUA_TTABLE);

	lua_rawgeti(state, loc, 1);
	PreciseVoxelCoord x = luaL_checknumber(state, -1);
	lua_pop(state, 1);

	lua_rawgeti(state, loc, 2);
	PreciseVoxelCoord y = luaL_checknumber(state, -1);
	lua_pop(state, 1);

	lua_rawgeti(state, loc, 3);
	PreciseVoxelCoord z = luaL_checknumber(state, -1);
	lua_pop(state, 1);

	return { x,y,z };
}

inline Vector luaL_checkvector(lua_State* state, int loc) {
	return state->luabase->GetVector(loc);
}

inline void luaL_pushvector(lua_State* state, float x, float y, float z) {
	lua_getglobal(state, "Vector");

	lua_pushnumber(state, x);
	lua_pushnumber(state, y);
	lua_pushnumber(state, z);

	lua_call(state, 3, 1);
}

inline void luaL_pushvector(lua_State* state, Vector v) {
	lua_getglobal(state, "Vector");

	lua_pushnumber(state, v.x);
	lua_pushnumber(state, v.y);
	lua_pushnumber(state, v.z);

	lua_call(state, 3, 1);
}

inline void luaL_pushvector(lua_State* state, VoxelCoordXYZ p) {
	lua_getglobal(state, "Vector");

	lua_pushnumber(state, p[0]);
	lua_pushnumber(state, p[1]);
	lua_pushnumber(state, p[2]);

	lua_call(state, 3, 1);
}
