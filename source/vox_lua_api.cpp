
#include <materialsystem/imaterialvar.h>

#include "vox_lua_api.h"
#include "vox_lua_src.h"

#include "vox_util.h"

#include "vox_network.h"

#include <tuple>

using namespace GarrysMod::Lua;

VOXDEF_LIB(PreciseToNormalCoords) {
	PreciseVoxelCoord coord = luaL_checknumber(state, 1);

	lua_pushnumber(state, preciseToNormal(coord));

	return 1;
}

//Friend function of VoxelWorld, handles all configuration.
//Todo use some sort of struct for config and shove it into the voxels class
VOXDEF_LIB(NewWorld) {
	int index = luaL_optint(state, 1, -1);

	// Initialise config
	VoxelConfig config;

	// but some macros first

#define CONF_BOOL(keyName,def) \
		lua_getfield(state, -1, #keyName); \
		if(!lua_isnil(state, -1)) { \
			config.##keyName = lua_toboolean(state, -1); \
		} \
		else { \
			config.##keyName = def; \
		} \
		lua_pop(state, 1)

#define CONF_NUM(keyName,def) \
		lua_getfield(state, -1, #keyName); \
		if(lua_isnumber(state, -1)) { \
			config.##keyName = lua_tonumber(state, -1); \
		} \
		else { \
			config.##keyName = def; \
		} \
		lua_pop(state, 1)

#define CONF_STR(keyName, def) \
		lua_getfield(state, -1, #keyName); \
		if(lua_isstring(state, -1)) { \
			config.##keyName = lua_tostring(state, -1); \
		} \
		else { \
			config.##keyName = def; \
		} \
		lua_pop(state, 1)

	// Dimensions
	CONF_NUM(scale, 40);

	// Atlas crap
#ifdef VOXELATE_CLIENT
	CONF_STR(atlasMaterialName, "models/debug/debugwhite");
	config.atlasMaterial = IFACE_CL_MATERIALS->FindMaterial(config.atlasMaterialName, nullptr);

	// Set atlas variables
	if(config.atlasMaterial != nullptr) {
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
#endif

	// Mesh building options
	CONF_BOOL(buildPhysicsMesh, false);

	// The rest of this is going to have to wait...
	/*
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
	*/

	// undef macros
#undef CONF_STR
#undef CONF_NUM
#undef CONF_BOOL

	luaL_pushvoxelworld(state, newIndexedVoxelWorld(index, config));

	return 1;
}

VOXDEF_META(DeleteWorld) {
	auto world = luaL_checkvoxelworld(state, 1);

	deleteIndexedVoxelWorld(world->worldID);

	return 0;
}

#ifdef VOXELATE_CLIENT
VOXDEF_META(Draw) {
	auto v = luaL_checkvoxelworld(state, 1);
	
	v->draw();

	return 0;
}
#endif

VOXDEF_META(GetBlockScale) {
	auto v = luaL_checkvoxelworld(state, 1);

	lua_pushnumber(state, v->getBlockScale());

	return 1;
}

VOXDEF_META(Update) {
	auto v = luaL_checkvoxelworld(state, 1);
	int chunk_count = luaL_checknumber(state, 2);
	float curTime = luaL_checknumber(state, 3);

	v->doUpdates(chunk_count, curTime);

	return 0;
}

VOXDEF_META(GetAllChunks) {
	auto v = luaL_checkvoxelworld(state, 1);

	auto origin = luaL_checkbtVector3(state, 2);

	auto chunk_positions = v->getAllChunkPositions(origin);
	lua_newtable(state);
	int i = 1;
	for (auto v : chunk_positions) {
		luaL_pushbtVector3(state, btAdjVector3(v[0], v[1], v[2]));

		lua_rawseti(state, -2, i);

		i++;
	}

	return 1;
}

#ifdef VOXELATE_SERVER
VOXDEF_META(SendChunk) {
	auto v = luaL_checkvoxelworld(state, 1);
	int peerID = luaL_checknumber(state, 2);
	int x = luaL_checknumber(state, 3);
	int y = luaL_checknumber(state, 4);
	int z = luaL_checknumber(state, 5);

	lua_pushboolean(state, v->sendChunk(peerID, { x, y, z }));

	return 1;
}
#endif

VOXDEF_META(IsChunkLoaded) {
	auto world = luaL_checkvoxelworld(state, 1);
	auto pos = luaL_checkvoxelxyz(state, 2);

	lua_pushboolean(state, world->isChunkLoaded(pos));

	return 1;
}

VOXDEF_META(LoadChunk) {
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

VOXDEF_META(GetBounds) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	auto bounds = v->getExtents();

	auto min = std::get<0>(bounds);
	min[0] *= v->config.scale;
	min[1] *= v->config.scale;
	min[2] *= v->config.scale;

	luaL_pushbtVector3(state, min);

	auto max = std::get<1>(bounds);
	max[0] *= v->config.scale;
	max[1] *= v->config.scale;
	max[2] *= v->config.scale;

	luaL_pushbtVector3(state, max);

	return 2;
}

#ifdef VOXELATE_CLIENT
VOXDEF_META(SetWorldViewPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	v->viewPos = luaL_checkbtVector3(state, 2);

	return 0;
}
VOXDEF_META(GetWorldViewPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	luaL_pushbtVector3(state, v->viewPos);

	return 1;
}
#endif

VOXDEF_META(SetSourceWorldPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	v->sourceWorldPos = luaL_checkvector(state, 2);

	return 0;
}

VOXDEF_META(GetSourceWorldPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	luaL_pushvector(state, v->sourceWorldPos.x, v->sourceWorldPos.y, v->sourceWorldPos.z);

	return 1;
}

VOXDEF_META(GetBlockID) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);

	auto x = luaL_checknumber(state, 2);
	auto y = luaL_checknumber(state, 3);
	auto z = luaL_checknumber(state, 4);

	lua_pushnumber(state, v->get(x, y, z));

	return 1;
}

VOXDEF_META(SetBlockID) {
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

VOXDEF_META(DoTrace) {
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

VOXDEF_META(SourceWorldToScaledBlockPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto worldPos = luaL_checkvector(state, 2);

	auto relative = worldPos - v->sourceWorldPos;
	relative /= v->config.scale;

	auto blockPos = SourcePositionToBullet(relative);

	luaL_pushbtVector3(state, blockPos);

	return 1;
}

VOXDEF_META(ScaledBlockPosToSourceWorld) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto blockPos = luaL_checkbtVector3(state, 2);

	auto relative = BulletPositionToSource(blockPos) * v->config.scale + v->sourceWorldPos;

	luaL_pushvector(state, relative);

	return 1;
}

VOXDEF_META(SourceWorldToBlockPos) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto worldPos = luaL_checkvector(state, 2);

	auto relative = worldPos - v->sourceWorldPos;

	auto blockPos = SourcePositionToBullet(relative);

	luaL_pushbtVector3(state, blockPos);

	return 1;
}

VOXDEF_META(BlockPosToSourceWorld) {
	VoxelWorld* v = luaL_checkvoxelworld(state, 1);
	auto blockPos = luaL_checkbtVector3(state, 2);

	auto relative = BulletPositionToSource(blockPos) + v->sourceWorldPos;

	luaL_pushvector(state, relative);

	return 1;
}

VOXDEF_META(IsValid) {
	int index = *state->luabase->GetUserType<int>(1, VoxelWorldTypeID);

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

VOXDEF_LIB(ReadFileUnrestricted) {
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
	VoxelWorldTypeID = LUA->CreateMetaTable(VoxelWorldTypeName);

	VOXREG_META(DeleteWorld);
	VOXREG_META(IsChunkLoaded);
	VOXREG_META(LoadChunk);
	VOXREG_META(GetBounds);
#ifdef VOXELATE_CLIENT
	VOXREG_META(SetWorldViewPos);
	VOXREG_META(GetWorldViewPos);
#endif
	VOXREG_META(SetSourceWorldPos);
	VOXREG_META(GetSourceWorldPos);
	VOXREG_META(GetBlockID);
	VOXREG_META(SetBlockID);
	VOXREG_META(DoTrace);
	VOXREG_META(SourceWorldToBlockPos);
	VOXREG_META(BlockPosToSourceWorld);
	VOXREG_META(SourceWorldToScaledBlockPos);
	VOXREG_META(ScaledBlockPosToSourceWorld);
	VOXREG_META(IsValid);

	LUA->Pop(1);

	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();

	VOXREG_LIB(NewWorld);
	VOXREG_LIB(PreciseToNormalCoords);

	LUA->PushString(version_string);
	LUA->SetField(-2, "VERSION");

#ifdef VOXELATE_LUA_HOTLOADING
	VOXREG_LIB(ReadFileUnrestricted);
#endif

	vox_setupLuaNetworkingAPI(LUA);

	LUA->SetField(-2, "G_VOX_IMPORTS");

	setupLuaAdvancedVectors(LUA->GetState());

	runBootstrap(LUA->GetState());

	LUA->Pop();
}
