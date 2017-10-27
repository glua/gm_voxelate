#pragma once

#include "glua.h"

#include "GarrysMod/LuaHelpers.hpp"

#include "vox_lua_advanced_vector.hpp"

#include "vox_engine.h"
#include "vox_entity.h"
#include "vox_voxelworld.h"
#include "vox_chunk.h"

static uint8_t VoxelWorldTypeID = GarrysMod::Lua::Type::NONE;
static const char* VoxelWorldTypeName = "VoxelWorld";

#define VOXREG_META(name) \
	lua_pushcfunction(LUA->GetState(), voxF_meta_##name); \
	lua_setfield(LUA->GetState(), -2, #name)

#define VOXREG_LIB(name) \
	lua_pushcfunction(LUA->GetState(), voxF_lib_##name); \
	lua_setfield(LUA->GetState(), -2, #name)

#define VOXDEF_META(name) \
    int voxF_meta_##name(lua_State* state)

#define VOXDEF_LIB(name) \
    int voxF_lib_##name(lua_State* state)
    
extern inline VoxelWorld* luaL_checkvoxelworld(lua_State* state, int loc);
extern void luaL_pushvoxelworld(lua_State* state, VoxelWorld world);
extern void luaL_pushvoxelworld(lua_State* state, int worldID);
extern inline VoxelCoordXYZ luaL_checkvoxelxyz(lua_State* state, int loc);
extern inline PreciseVoxelCoordXYZ luaL_checkprecisevoxelxyz(lua_State* state, int loc);
extern inline Vector luaL_checkvector(lua_State* state, int loc);
extern inline void luaL_pushvector(lua_State* state, float x, float y, float z);
extern inline void luaL_pushvector(lua_State* state, Vector v);
extern inline void luaL_pushvector(lua_State* state, VoxelCoordXYZ p);
extern inline void luaL_pushbtVector3(lua_State* state, VoxelCoordXYZ p);
