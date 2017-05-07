#include "vox_lua_bridge.h"
#include "vox_lua_src.h"

#include "vox_engine.h"
#include "vox_util.h"
#include "vox_Voxels.h"

using namespace GarrysMod::Lua;

//Utility functions for pulling ents, vectors directly from the lua with limited amounts of fuckery.

CBaseEntity* elua_getEntity(lua_State* state, int index) {
	if (!IS_SERVERSIDE)
		return nullptr;

	GarrysMod::Lua::UserData* ud = reinterpret_cast<GarrysMod::Lua::UserData*>(LUA->GetUserdata(index));

	int e_index = reinterpret_cast<CBaseHandle*>(ud->data)->GetEntryIndex();

	auto edict = IFACE_SV_ENGINE->PEntityOfEntIndex(e_index);

	if (edict == nullptr)
		return nullptr;
	return edict->GetUnknown()->GetBaseEntity();
}

Vector elua_getVector(lua_State* state, int index) {
	GarrysMod::Lua::UserData* ud = reinterpret_cast<GarrysMod::Lua::UserData*>(LUA->GetUserdata(index));
	Vector v = *reinterpret_cast<Vector*>(ud->data);
	return v;
}


//Can't figure out how to push vectors without crashing the game when they get GC'd
void elua_pushVector(lua_State* state, Vector v) {
	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->GetField(-1, "Vector");
	LUA->Remove(-2);
	LUA->PushNumber(v.x);
	LUA->PushNumber(v.y);
	LUA->PushNumber(v.z);
	LUA->Call(3, 1);
	
	/*
	GarrysMod::Lua::UserData* ud = (UserData*)(LUA->NewUserdata(sizeof(UserData)));
	ud->type = Type::VECTOR;
	ud->data = new Vector(v);
	
	LUA->CreateMetaTableType("Vector", Type::VECTOR);
	LUA->SetMetaTable(-2);
	*/
}



int luaf_voxNew(lua_State* state) {
	LUA->PushNumber(newIndexedVoxels(LUA->GetNumber(1)));

	return 1;
}

//Friend function of Voxels, handles all configuration.
//Todo use some sort of struct for config and shove it into the voxels class
int luaf_voxInit(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr) {
		VoxelConfig* config = new VoxelConfig();
		if (IS_SERVERSIDE) {
			LUA->GetField(2, "useMeshCollisions");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL))
				config->sv_useMeshCollisions = LUA->GetBool();
			LUA->Pop();
		}
		else {
			LUA->GetField(2, "drawExterior");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL))
				config->cl_drawExterior = LUA->GetBool();
			LUA->Pop();

			LUA->GetField(2, "atlasWidth");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
				config->cl_atlasWidth = LUA->GetNumber();
			}
			LUA->Pop();

			LUA->GetField(2, "atlasHeight");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER))
				config->cl_atlasHeight = LUA->GetNumber();
			LUA->Pop();

			LUA->GetField(2, "atlasMaterial");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::STRING))
				config->cl_atlasMaterial = IFACE_CL_MATERIALS->FindMaterial(LUA->GetString(-1), nullptr);
			else
				config->cl_atlasMaterial = IFACE_CL_MATERIALS->FindMaterial("models/debug/debugwhite", nullptr);
			LUA->Pop();

			config->cl_atlasMaterial->IncrementReferenceCount();

			LUA->GetField(2, "atlasIsPadded");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL) && LUA->GetBool()) {
				config->cl_pixel_bias_x = (config->cl_atlasMaterial->GetMappingWidth() / config->cl_atlasWidth / 4.0) / config->cl_atlasMaterial->GetMappingWidth();
				config->cl_pixel_bias_y = (config->cl_atlasMaterial->GetMappingHeight() / config->cl_atlasHeight / 4.0) / config->cl_atlasMaterial->GetMappingHeight();
			}
			LUA->Pop();

			//vox_print("%f <-> %f", config->cl_pixel_bias_x, config->cl_pixel_bias_y);
		}

		LUA->GetField(2, "dimensions");
		if (LUA->IsType(-1, GarrysMod::Lua::Type::VECTOR)) {
			Vector dims = elua_getVector(state, -1);
			if (dims.x >= 1 && dims.y >= 1 && dims.z >= 1) {
				config->dimX = dims.x;
				config->dimY = dims.y;
				config->dimZ = dims.z;
			}
		}
		LUA->Pop();

		LUA->GetField(2, "scale");
		if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
			double n = LUA->GetNumber();
			if (n > 0) {
				config->scale = n;
			}
		}
		LUA->Pop();

		LUA->GetField(2, "voxelTypes");
		if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
			LUA->PushNil();
			while (LUA->Next(-2)) {
				if (LUA->IsType(-2, GarrysMod::Lua::Type::NUMBER)) {
					VoxelType& vt = config->voxelTypes[static_cast<int>(LUA->GetNumber(-2))];
					vt.form = VFORM_CUBE;
					if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
						LUA->GetField(-1, "atlasIndex");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							AtlasPos atlasPos = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
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
							vt.side_xPos = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

						LUA->GetField(-1, "atlasIndex_xNeg");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							vt.side_xNeg = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

						LUA->GetField(-1, "atlasIndex_yPos");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							vt.side_yPos = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

						LUA->GetField(-1, "atlasIndex_yNeg");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							vt.side_yNeg = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

						LUA->GetField(-1, "atlasIndex_zPos");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							vt.side_zPos = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

						LUA->GetField(-1, "atlasIndex_zNeg");
						if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
							int atlasIndex = LUA->GetNumber(-1);
							vt.side_zNeg = AtlasPos(atlasIndex % config->cl_atlasWidth, atlasIndex / config->cl_atlasWidth);
						}
						LUA->Pop();

					}
				}
				LUA->Pop();
			}
		}
		LUA->Pop();

		v->initialize(config);

		LUA->GetField(2, "_ent");
		LUA->GetField(-1, "_initMisc");
		LUA->Push(-2);
		Vector extents = v->getExtents();
		LUA->PushNumber(extents.x);
		LUA->PushNumber(extents.y);
		LUA->PushNumber(extents.z);
		LUA->Call(4, 0);
		LUA->Pop();
	}

	return 0;
}

int luaf_voxDelete(lua_State* state) {
	int index = LUA->GetNumber(1);
	deleteIndexedVoxels(index);
	return 0;
}

int luaf_voxDraw(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr) {
		v->draw();
	}

	return 0;
}

int luaf_voxData(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	
	if (v == nullptr) {
		LUA->PushBool(false);
		return 1;
	}

	int chunk_n = LUA->GetNumber(2);

	char data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2];

	int out_len = v->getChunkData(chunk_n, data);
	if (out_len == 0) {
		LUA->PushBool(true);
		return 1;
	}

	LUA->PushString(data,out_len);
	return 1;
}

int luaf_voxInitChunk(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);

	if (v != nullptr) {
		unsigned int len;
		const char* data = LUA->GetString(3, &len);
		
		v->setChunkData(LUA->GetNumber(2),data,len);
	}
	return 0;
}

int luaf_voxEnableMeshGeneration(lua_State* state) {
	int index = LUA->GetNumber(1);
	bool enable = LUA->GetBool(2);

	Voxels* v = getIndexedVoxels(index);
	if (v) {
		v->enableUpdates(enable);
	}

	return 0;
}

int luaf_voxFlagAllChunksForUpdate(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v) {
		v->flagAllChunksForUpdate();
	}

	return 0;
}

int luaf_voxGenerate(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v) {
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
	}

	return 0;
}

int luaf_voxGet(lua_State* state) {
	int index = LUA->GetNumber(1);

	int x = LUA->CheckNumber(2);
	int y = LUA->CheckNumber(3);
	int z = LUA->CheckNumber(4);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr)
		LUA->PushNumber(v->get(x, y, z));
	else
		LUA->PushNumber(0);
	return 0;
}

int luaf_voxSet(lua_State* state) {
	int index = LUA->GetNumber(1);

	int x = LUA->CheckNumber(2);
	int y = LUA->CheckNumber(3);
	int z = LUA->CheckNumber(4);
	int d = LUA->CheckNumber(5);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr) {
		if (v->set(x, y, z, d)) {
			LUA->PushBool(true);
			return 1;
		}
	}
	return 0;
}

int luaf_voxUpdate(lua_State* state) {
	int index = LUA->GetNumber(1);
	int chunk_count = LUA->GetNumber(2);

	CBaseEntity* ent = elua_getEntity(state, 3);

	Voxels* v = getIndexedVoxels(index);

	if (v != nullptr) {
		v->doUpdates(10, ent);
	}
	
	return 0;
}

int luaf_voxTrace(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr && v->isInitialized()) {
		Vector start = elua_getVector(state, 2);

		Vector delta = elua_getVector(state, 3);

		VoxelTraceRes r;
		if (LUA->GetBool(4)) {
			Vector extents = elua_getVector(state,5);

			r = v->doTraceHull(start, delta, extents);
		}
		else {
			r = v->doTrace(start, delta);
		}

		if (r.fraction != -1) {
			LUA->PushNumber(r.fraction);
			elua_pushVector(state, r.hitPos);
			elua_pushVector(state, r.hitNormal);
			return 3;
		}
	}

	return 0;
}

void init_lua(lua_State* state, const char* version_string) {
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();

	LUA->PushCFunction(luaf_voxNew);
	LUA->SetField(-2, "voxNew");

	LUA->PushCFunction(luaf_voxInit);
	LUA->SetField(-2, "voxInit");

	LUA->PushCFunction(luaf_voxDelete);
	LUA->SetField(-2, "voxDelete");

	LUA->PushCFunction(luaf_voxDraw);
	LUA->SetField(-2, "voxDraw");

	LUA->PushCFunction(luaf_voxData);
	LUA->SetField(-2, "voxData");

	LUA->PushCFunction(luaf_voxInitChunk);
	LUA->SetField(-2, "voxInitChunk");

	LUA->PushCFunction(luaf_voxEnableMeshGeneration);
	LUA->SetField(-2, "voxEnableMeshGeneration");

	LUA->PushCFunction(luaf_voxFlagAllChunksForUpdate);
	LUA->SetField(-2, "voxFlagAllChunksForUpdate");

	LUA->PushCFunction(luaf_voxUpdate);
	LUA->SetField(-2, "voxUpdate");

	LUA->PushCFunction(luaf_voxTrace);
	LUA->SetField(-2, "voxTrace");

	LUA->PushCFunction(luaf_voxGenerate);
	LUA->SetField(-2, "voxGenerate");

	LUA->PushCFunction(luaf_voxGet);
	LUA->SetField(-2, "voxGet");

	LUA->PushCFunction(luaf_voxSet);
	LUA->SetField(-2, "voxSet");

	LUA->PushString(version_string);
	LUA->SetField(-2, "VERSION");

	LUA->SetField(-2, "G_VOX_IMPORTS");

	LUA->GetField(-1, "RunString");
	LUA->PushString(LUA_SRC);
	LUA->Call(1, 0);

	LUA->Pop();
}