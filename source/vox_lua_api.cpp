
#include <materialsystem/imaterialvar.h>

#include "glua.h"

#include "vox_lua_advanced_vector.hpp"

#include "vox_lua_api.h"
#include "vox_lua_src.h"

#include "vox_engine.h"
#include "vox_util.h"
#include "vox_voxelworld.h"
#include "vox_chunk.h"

#include "vox_network.h"


#include "GarrysMod/LuaHelpers.hpp"

#include <tuple>

using namespace GarrysMod::Lua;

//Utility functions for pulling ents, vectors directly from the lua with limited amounts of fuckery.

CBaseEntity* elua_getEntity(GarrysMod::Lua::ILuaBase* LUA, int index) {
	if (!IS_SERVERSIDE)
		return nullptr;

	GarrysMod::Lua::ILuaBase::UserData* ud = LUA->GetUserdata(index);

	int e_index = reinterpret_cast<CBaseHandle*>(ud->data)->GetEntryIndex();

	auto edict = IFACE_SV_ENGINE->PEntityOfEntIndex(e_index);

	if (edict == nullptr)
		return nullptr;
	return edict->GetUnknown()->GetBaseEntity();
}

bool config_bool(GarrysMod::Lua::ILuaBase* LUA, const char* name, bool default_value) {
	LUA->GetField(1, name);
	if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL))
		default_value = LUA->GetBool();
	LUA->Pop();

	return default_value;
}

double config_num(GarrysMod::Lua::ILuaBase* LUA, const char* name, double default_value) {
	LUA->GetField(1, name);
	if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER))
		default_value = LUA->GetNumber();
	LUA->Pop();

	return default_value;
}

const char* config_string(GarrysMod::Lua::ILuaBase* LUA, const char* name, const char* default_value) {
	LUA->GetField(1, name);
	if (LUA->IsType(-1, GarrysMod::Lua::Type::STRING))
		default_value = LUA->GetString();
	LUA->Pop();

	return default_value;
}

Vector config_vector(GarrysMod::Lua::ILuaBase* LUA, const char* name, Vector default_value) {
	LUA->GetField(1, name);
	if (LUA->IsType(-1, GarrysMod::Lua::Type::VECTOR))
		default_value = LUA->GetVector();
	LUA->Pop();

	return default_value;
}


//Friend function of VoxelWorld, handles all configuration.
//Todo use some sort of struct for config and shove it into the voxels class
LUA_FUNCTION(luaf_voxNewWorld) {

	int index = -1;

	if (LUA->IsType(2, GarrysMod::Lua::Type::NUMBER))
		index = LUA->GetNumber(2);

	VoxelConfig config;

	// Dimensions
	config.scale = config_num(LUA, "scale", 32);

	// Atlas crap
	if (!IS_SERVERSIDE) {
		const char* temp_mat_name = config_string(LUA, "atlasMaterial", "models/debug/debugwhite");
		config.atlasMaterial = IFACE_CL_MATERIALS->FindMaterial(temp_mat_name, nullptr);

		bool var_found;
		IMaterialVar* var;
		
		var = config.atlasMaterial->FindVar("$atlas_w", &var_found);
		if (var_found && var->GetIntValue() > 0) {
			config.atlasWidth = var->GetIntValue();
		}
		else {
			config.atlasWidth = 1;
		}

		var = config.atlasMaterial->FindVar("$atlas_h", &var_found);
		if (var_found && var->GetIntValue() > 0) {
			config.atlasHeight = var->GetIntValue();
		}
		else {
			config.atlasHeight = 1;
		}
	}

	// Mesh building options
	config.buildPhysicsMesh = config_bool(LUA, "buildPhysicsMesh",false);

	// The rest of this is going to have to wait...
	LUA->GetField(1, "voxelTypes");
	if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
		LUA->PushNil();
		while (LUA->Next(-2)) {
			if (LUA->IsType(-2, GarrysMod::Lua::Type::NUMBER)) {
				VoxelType& vt = config.voxelTypes[static_cast<int>(LUA->GetNumber(-2))];
				vt.form = VFORM_CUBE;
				if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
					LUA->GetField(-1, "atlasIndex");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						AtlasPos atlasPos = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
						vt.side_xPos = atlasPos;
						vt.side_xNeg = atlasPos;
						vt.side_yPos = atlasPos;
						vt.side_yNeg = atlasPos;
						vt.side_zPos = atlasPos;
						vt.side_zNeg = atlasPos;
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_xPos");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_xPos = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_xNeg");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_xNeg = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_yPos");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_yPos = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_yNeg");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_yNeg = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_zPos");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_zPos = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

					LUA->GetField(-1, "atlasIndex_zNeg");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.side_zNeg = AtlasPos(atlasIndex % config.atlasWidth, atlasIndex / config.atlasWidth);
					}
					LUA->Pop();

				}
			}
			LUA->Pop();
		}
	}
	LUA->Pop();

	index = newIndexedVoxelWorld(index, config);

	LUA->PushNumber(index);

	return 1;
}

LUA_FUNCTION(luaf_voxDeleteWorld) {
	int index = LUA->GetNumber(1);
	deleteIndexedVoxelWorld(index);
	return 0;
}

lua_State* lastState;

#ifdef VOXELATE_CLIENT
LUA_FUNCTION(luaf_voxDraw) {
	int index = LUA->GetNumber(1);

	lastState = LUA->GetState();

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v != nullptr) {
		v->draw();
	}

	return 0;
}
#endif

LUA_FUNCTION(luaf_voxGetBlockScale) {
	int index = LUA->CheckNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v != nullptr) {
		double scale = v->getBlockScale();
		LUA->PushNumber(scale);

		return 1;
	}

	return 0;
}

/*int luaf_voxGenerateDefault(lua_State* state) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	if (v) {
		//auto oldUpdatesEnabled = v->trackUpdates; wtf this doesnt even do anything
		//v->trackUpdates = false;

		int mx, my, mz;
		v->getCellExtents(mx, my, mz);

		for (int x = 0; x < mx; x++) {
			for (int y = 0; y < my; y++) {
				for (int z = 0; z < mz; z++) {
					auto block = vox_worldgen_basic(x,y,z);
					v->set(x, y, z, block, false);
				}
			}
		}

		//v->trackUpdates = oldUpdatesEnabled;
	}

	return 0;
}*/

/*int luaf_voxGetWorldUpdates(lua_State* state) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v) {
		for (auto pos : v->queued_block_updates) {
			LuaHelpers::PushHookRun(state->luabase, "VoxWorldBlockUpdate");

			lua_pushnumber(state, index);
			lua_pushnumber(state, pos[0]);
			lua_pushnumber(state, pos[1]);
			lua_pushnumber(state, pos[2]);

			LuaHelpers::CallHookRun(state->luabase, 4, 0);
		}

		v->queued_block_updates.clear();
	}

	return 0;
}

int luaf_voxSetWorldUpdatesEnabled(lua_State* state) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v) {
		bool enabled = LUA->GetBool(2);

		v->trackUpdates = enabled;
	}

	return 0;
}*/

/*int luaf_voxGenerate(lua_State* state) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v) {
		//auto oldUpdatesEnabled = v->trackUpdates;
		//v->trackUpdates = false;

		int mx, my, mz;
		v->getCellExtents(mx, my, mz);
		for (int x = 0; x < mx; x++) {
			for (int y = 0; y < my; y++) {
				for (int z = 0; z < mz; z++) {
					LUA->Push(2);
					LUA->PushNumber(x);
					LUA->PushNumber(y);
					LUA->PushNumber(z);
					LUA->Call(3, 1);
					int d = LUA->GetNumber();
					LUA->Pop();

					v->set(x, y, z, d, false);
				}
			}
		}

		//v->trackUpdates = oldUpdatesEnabled;
	}

	return 0;
}*/

LUA_FUNCTION(luaf_voxGet) {
	int index = LUA->GetNumber(1);

	PreciseVoxelCoord x = LUA->CheckNumber(2);
	PreciseVoxelCoord y = LUA->CheckNumber(3);
	PreciseVoxelCoord z = LUA->CheckNumber(4);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v != nullptr)
		LUA->PushNumber(v->get(x, y, z));
	else
		LUA->PushNumber(0);

	return 1;
}

LUA_FUNCTION(luaf_voxSet) {
	int index = LUA->GetNumber(1);

	PreciseVoxelCoord x = LUA->CheckNumber(2);
	PreciseVoxelCoord y = LUA->CheckNumber(3);
	PreciseVoxelCoord z = LUA->CheckNumber(4);
	BlockData d = LUA->CheckNumber(5);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v != nullptr) {
		if (v->set(x, y, z, d)) {
			LUA->PushBool(true);
			return 1;
		}
	}
	return 0;
}

LUA_FUNCTION(luaf_voxUpdate) {
	int index = LUA->GetNumber(1);
	int chunk_count = LUA->GetNumber(2);
	float curTime = LUA->GetNumber(3);

	CBaseEntity* ent = elua_getEntity(LUA, 3);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	if (v != nullptr) {
		v->doUpdates(chunk_count, ent, curTime);
	}

	return 0;
}

/*int luaf_voxSortUpdatesByDistance(lua_State* state) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	auto origin = LUA->GetVector(2);

	if (v != nullptr) {
		v->sortUpdatesByDistance(&origin);
	}

	return 0;
}*/

LUA_FUNCTION(luaf_voxGetAllChunks) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	Vector origin = LUA->GetVector(2);

	if (v == nullptr) {
		return 0;
	}

	auto chunk_positions = v->getAllChunkPositions(origin);
	LUA->CreateTable();
	int i = 1;
	for (auto v : chunk_positions) {
		LUA->PushNumber(i);
		LUA->PushVector(Vector(v[0], v[1], v[2]));

		LUA->SetTable(-3);
		i++;
	}

	return 1;
}

#ifdef VOXELATE_SERVER
LUA_FUNCTION(luaf_voxSendChunk) {
	int index = LUA->GetNumber(1);
	int peerID = LUA->GetNumber(2);
	int x = LUA->GetNumber(3);
	int y = LUA->GetNumber(4);
	int z = LUA->GetNumber(5);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	if (v != nullptr) {
		LUA->PushBool(v->sendChunk(peerID, { x, y, z }));
	}
	else {
		LUA->PushBool(false);
	}

	return 1;
}

/*LUA_FUNCTION(luaf_voxSendChunks) {
	int index = LUA->GetNumber(1);
	int peerID = LUA->GetNumber(2);
	int x = LUA->GetNumber(3);
	int y = LUA->GetNumber(4);
	int z = LUA->GetNumber(5);
	int radius = LUA->GetNumber(6);

	VoxelWorld* v = getIndexedVoxelWorld(index);

	if (v != nullptr) {
		LUA->PushBool(v->sendChunksAround(peerID, {x, y, z}, radius));
	}
	else {
		lua_pushboolean(state, false);
	}

	return 1;
}*/
#endif

/*int luaf_voxSaveToString1(lua_State* state) { // save with format 1
	// int index = LUA->GetNumber(1);

	// VoxelWorld* v = getIndexedVoxelWorld(index);
	// if (v != nullptr) {
	// 	auto ret = v->writeToString();
	// 	auto data = std::get<0>(ret);
	// 	auto size = std::get<1>(ret);

	// 	lua_pushlstring(state, data, size);

	// 	delete[size] data;

	// 	return 1;
	// }

	return 0;
}

int luaf_voxLoadFromString1(lua_State* state) { // save with format 1
	// int index = LUA->GetNumber(1);

	// VoxelWorld* v = getIndexedVoxelWorld(index);
	// if (v != nullptr) {
	// 	size_t size;

	// 	auto Ldata = luaL_checklstring(state, 2, &size);

	// 	std::string data(Ldata, size);

	// 	auto success = v->loadFromString(data);

	// 	lua_pushboolean(state, success);

	// 	return 1;
	// }

	return 0;
}*/

LUA_FUNCTION(luaf_voxTrace) {
	int index = LUA->GetNumber(1);

	VoxelWorld* v = getIndexedVoxelWorld(index);
	if (v != nullptr) {
		Vector start = LUA->GetVector(2);

		Vector delta = LUA->GetVector(3);

		VoxelTraceRes r;
		if (LUA->GetBool(4)) {
			Vector extents = LUA->GetVector(5);
			r = v->doTraceHull(start, delta, extents);
		}
		else {
			r = v->doTrace(start, delta);
		}

		if (r.fraction != -1) {
			LUA->PushNumber(r.fraction);
			LUA->PushVector(r.hitPos);
			LUA->PushVector(r.hitNormal);
			return 3;
		}
	}

	return 0;
}

#define VOXF(name) \
	lua_pushcfunction(LUA->GetState(), voxF_##name); \
	lua_setfield(LUA->GetState(), -2, #name)

#define VOXDEF(name) \
	int voxF_##name(lua_State* state)

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

VOXDEF(isChunkLoaded) {
	auto world = luaL_checkvoxelworld(state, 1);
	auto pos = luaL_checkvoxelxyz(state, 2);

	lua_pushboolean(state, world->isChunkLoaded(pos));

	return 1;
}

VOXDEF(loadChunk) {
	auto world = luaL_checkvoxelworld(state, 1);
	auto pos = luaL_checkvoxelxyz(state, 2);

	if (!world->isChunkLoaded(pos)) {
		auto chunk = world->initChunk(pos[0], pos[1], pos[2]);
		chunk->generate();

		lua_pushboolean(state, true);
	}
	else {
		lua_pushboolean(state, false);
	}

	return 1;
}

VOXDEF(voxBounds) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	auto bounds = v->getExtents();

	auto min = std::get<0>(bounds);
	min[0] *= v->config.scale;
	min[1] *= v->config.scale;
	min[2] *= v->config.scale;

	luaL_pushvector(state, min);

	auto max = std::get<1>(bounds);
	max[0] *= v->config.scale;
	max[1] *= v->config.scale;
	max[2] *= v->config.scale;

	luaL_pushvector(state, max);

	return 2;
}

#ifdef VOXELATE_CLIENT
VOXDEF(setWorldViewPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	v->viewPos = luaL_checkbtVector3(state, 2);

	return 0;
}
VOXDEF(getWorldViewPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	luaL_pushbtVector3(state, v->viewPos);

	return 1;
}
#endif

VOXDEF(preciseToNormalCoords) {
	PreciseVoxelCoord coord = luaL_checknumber(state, 1);

	lua_pushnumber(state, preciseToNormal(coord));

	return 1;
}


// Bootstrap shit
void setupFiles();
const char* grabBootstrap();
int grabBootstrapLength();

lua_State* currentState;

void addFile(const unsigned char path[], int pathlen, const unsigned char contents[], int contlen) {
	if (currentState != 0) {
		// printf("SET %s\n", (char*)path);
		lua_pushlstring(currentState, (char*)path, pathlen);
		lua_pushlstring(currentState, (char*)contents, contlen);
		lua_settable(currentState, -3);
	}
}

void runBootstrap(lua_State* state) {
	currentState = state;

	bool success = false;
	const char* errmsg;

	lua_newtable(state);

	setupFiles();

	lua_setglobal(state, "FILETABLE");

	lua_pushcfunction(state, LuaHelpers::LuaErrorTraceback);
	int errFnLoc = lua_gettop(state);

	lua_pushlstring(currentState, grabBootstrap(), grabBootstrapLength());

	if (luaL_loadbuffer(state, grabBootstrap(), grabBootstrapLength(), "=vox_bootstrap") != 0) {
		errmsg = lua_tolstring(state, -1, NULL);
		lua_pop(state, 1);
	}
	else if (lua_pcall(state, 0, 0, errFnLoc) != 0) {
		int errLoc = lua_gettop(state);
		lua_getglobal(state, "tostring");
		lua_pushvalue(state, errLoc);
		if (lua_pcall(state, 1, 1, NULL) != 0) {
			errmsg = "something happened";
		}
		else {
			errmsg = lua_tolstring(state, lua_gettop(state), NULL);
		}
	}
	else {
		success = true;
	}
	lua_pop(state, 1);

	if (!success) {
		Msg("Bootstrap failed! (%s)\n", errmsg);
	}
}

#ifdef VOXELATE_LUA_HOTLOADING

#include <fstream>
#include <string>
#include <streambuf>

int luaf_voxReadFile(lua_State* state) {
	std::string path = luaL_checkstring(state, 1);

	std::ifstream t(path);
	if (t.fail())
		return 0;

	std::string contents((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());

	lua_pushlstring(state, contents.c_str(), contents.size());

	return 1;
}

#endif

void vox_init_lua_api(GarrysMod::Lua::ILuaBase *LUA, const char* version_string) {
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();

	VOXF(isChunkLoaded);
	VOXF(loadChunk);
	VOXF(voxBounds);
#ifdef VOXELATE_CLIENT
	VOXF(setWorldViewPos);
	VOXF(getWorldViewPos);
#endif
	VOXF(preciseToNormalCoords);

	LUA->PushCFunction(luaf_voxNewWorld);
	LUA->SetField(-2, "voxNewWorld");

	LUA->PushCFunction(luaf_voxDeleteWorld);
	LUA->SetField(-2, "voxDeleteWorld");

#ifdef VOXELATE_CLIENT
	LUA->PushCFunction(luaf_voxDraw);
	LUA->SetField(-2, "voxDraw");
#endif

	LUA->PushCFunction(luaf_voxUpdate);
	LUA->SetField(-2, "voxUpdate");

	LUA->PushCFunction(luaf_voxGetAllChunks);
	LUA->SetField(-2, "voxGetAllChunks");

	LUA->PushCFunction(luaf_voxTrace);
	LUA->SetField(-2, "voxTrace");

	/*LUA->PushCFunction(luaf_voxGenerate);
	LUA->SetField(-2, "voxGenerate");

	LUA->PushCFunction(luaf_voxGenerateDefault);
	LUA->SetField(-2, "voxGenerateDefault");*/

	LUA->PushCFunction(luaf_voxGetBlockScale);
	LUA->SetField(-2, "voxGetBlockScale");

	LUA->PushCFunction(luaf_voxGet);
	LUA->SetField(-2, "voxGet");

	LUA->PushCFunction(luaf_voxSet);
	LUA->SetField(-2, "voxSet");

	/*LUA->PushCFunction(luaf_voxGetWorldUpdates); for real? fuck off
	LUA->SetField(-2, "voxGetWorldUpdates");

	LUA->PushCFunction(luaf_voxSetWorldUpdatesEnabled);
	LUA->SetField(-2, "voxSetWorldUpdatesEnabled");*/

	/*LUA->PushCFunction(luaf_voxLoadFromString1);
	LUA->SetField(-2, "voxLoadFromString1");

	LUA->PushCFunction(luaf_voxSaveToString1);
	LUA->SetField(-2, "voxSaveToString1");*/

	LUA->PushString(version_string);
	LUA->SetField(-2, "VERSION");

#ifdef VOXELATE_SERVER
	LUA->PushCFunction(luaf_voxSendChunk);
	LUA->SetField(-2, "voxSendChunk");

	//LUA->PushCFunction(luaf_voxSendChunks);
	//LUA->SetField(-2, "voxSendChunks");
#endif

#ifdef VOXELATE_LUA_HOTLOADING
	LUA->PushCFunction(luaf_voxReadFile);
	LUA->SetField(-2, "readFileUnrestricted");
#endif

	vox_setupLuaNetworkingAPI(LUA);
	setupLuaAdvancedVectors(LUA->GetState());

	LUA->SetField(-2, "G_VOX_IMPORTS");

	runBootstrap(LUA->GetState());

	LUA->Pop();
}