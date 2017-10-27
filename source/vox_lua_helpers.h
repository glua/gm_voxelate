#pragma once

#include "glua.h"

#include "GarrysMod/LuaHelpers.hpp"

#include "vox_lua_advanced_vector.hpp"

#include "vox_engine.h"
#include "vox_entity.h"
#include "vox_voxelworld.h"
#include "vox_chunk.h"

#define VOXF(name) \
	lua_pushcfunction(LUA->GetState(), voxF_##name); \
	lua_setfield(LUA->GetState(), -2, #name)

#define VOXDEF(name) \
    int voxF_##name(lua_State* state)
    
extern inline VoxelWorld* luaL_checkvoxelworld(lua_State* state, int loc);
extern inline VoxelCoordXYZ luaL_checkvoxelxyz(lua_State* state, int loc);
extern inline PreciseVoxelCoordXYZ luaL_checkprecisevoxelxyz(lua_State* state, int loc);
extern inline Vector luaL_checkvector(lua_State* state, int loc);
extern inline void luaL_pushvector(lua_State* state, float x, float y, float z);
extern inline void luaL_pushvector(lua_State* state, Vector v);
extern inline void luaL_pushvector(lua_State* state, VoxelCoordXYZ p);
