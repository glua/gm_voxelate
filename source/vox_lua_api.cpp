
#include <materialsystem/imaterialvar.h>

#include "vox_lua_api.h"
#include "vox_lua_src.h"

#include "vox_util.h"

#include "vox_network.h"

#include <tuple>

using namespace GarrysMod::Lua;

static uint8_t metatype = GarrysMod::Lua::Type::NONE;
static const char *metaname = "VoxelWorld";

// Utility functions for pulling ents, vectors directly from the lua with limited amounts of fuckery.

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

#ifdef VOXELATE_CLIENT
LUA_FUNCTION(luaf_voxDraw) {
	int index = LUA->GetNumber(1);

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

#endif

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

VOXDEF(setSourceWorldPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	v->sourceWorldPos = luaL_checkvector(state, 2);

	return 0;
}

VOXDEF(getSourceWorldPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	luaL_pushvector(state, v->sourceWorldPos.x, v->sourceWorldPos.y, v->sourceWorldPos.z);

	return 1;
}

VOXDEF(preciseToNormalCoords) {
	PreciseVoxelCoord coord = luaL_checknumber(state, 1);

	lua_pushnumber(state, preciseToNormal(coord));

	return 1;
}

VOXDEF(voxGet) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	auto x = luaL_checknumber(state, 2);
	auto y = luaL_checknumber(state, 3);
	auto z = luaL_checknumber(state, 4);

	lua_pushnumber(state, v->get(x, y, z));

	return 1;
}

VOXDEF(voxSet) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	auto x = luaL_checknumber(state, 2);
	auto y = luaL_checknumber(state, 3);
	auto z = luaL_checknumber(state, 4);

	BlockData d = luaL_checknumber(state, 5);

	if (v->set(x, y, z, d)) {
		lua_pushboolean(state, true);

		return 1;
	}

	return 0;
}

VOXDEF(voxTrace) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto start = luaL_checkbtVector3(state, 2);
	auto delta = luaL_checkbtVector3(state, 3);

	VoxelTraceRes r;
	if (lua_toboolean(state, 4)) {
		auto extents = luaL_checkbtVector3(state, 5);
		r = v->doTraceHull(start, delta, extents);
	}
	else {
		r = v->doTrace(start, delta);
	}

	if ((r.fraction != 1) && (r.fraction != -1)) {
		lua_pushnumber(state, r.fraction);
		luaL_pushbtVector3(state, r.hitPos);
		luaL_pushbtVector3(state, r.hitNormal);

		return 3;
	}

	return 0;
}

VOXDEF(voxSourceWorldToBlockPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto worldPos = luaL_checkvector(state, 2);

	auto relative = worldPos - v->sourceWorldPos;
	relative /= v->config.scale;

	auto blockPos = SourcePositionToBullet(relative);

	luaL_pushbtVector3(state, blockPos);

	return 1;
}

VOXDEF(voxBlockPosToSourceWorld) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto blockPos = luaL_checkbtVector3(state, 2);

	auto relative = BulletPositionToSource(blockPos) * v->config.scale + v->sourceWorldPos;

	luaL_pushvector(state, relative);

	return 1;
}

VOXDEF(voxSourceWorldToVoxelatePos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto worldPos = luaL_checkvector(state, 2);

	auto relative = worldPos - v->sourceWorldPos;

	auto blockPos = SourcePositionToBullet(relative);

	luaL_pushbtVector3(state, blockPos);

	return 1;
}

VOXDEF(voxVoxelatePosToSourceWorld) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto blockPos = luaL_checkbtVector3(state, 2);

	auto relative = BulletPositionToSource(blockPos) + v->sourceWorldPos;

	luaL_pushvector(state, relative);

	return 1;
}

VOXDEF(voxReady) {
	int index = luaL_checknumber(state, 1);
	auto world = getIndexedVoxelWorld(index);

	lua_pushboolean(state, world != nullptr);

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
	VOXF(setSourceWorldPos);
	VOXF(getSourceWorldPos);
	VOXF(preciseToNormalCoords);
	VOXF(voxGet);
	VOXF(voxSet);
	VOXF(voxTrace);
	VOXF(voxSourceWorldToBlockPos);
	VOXF(voxBlockPosToSourceWorld);
	VOXF(voxSourceWorldToVoxelatePos);
	VOXF(voxVoxelatePosToSourceWorld);
	VOXF(voxReady);

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

	/*LUA->PushCFunction(luaf_voxGenerate);
	LUA->SetField(-2, "voxGenerate");

	LUA->PushCFunction(luaf_voxGenerateDefault);
	LUA->SetField(-2, "voxGenerateDefault");*/

	LUA->PushCFunction(luaf_voxGetBlockScale);
	LUA->SetField(-2, "voxGetBlockScale");

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