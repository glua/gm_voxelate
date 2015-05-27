#include "vox_lua_bridge.h"
#include "vox_lua_src.h"

#include "vox_engine.h"
#include "vox_util.h"
#include "vox_Voxels.h"

using namespace GarrysMod::Lua;

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
		if (STATE_SERVER) {
			LUA->GetField(2, "useMeshCollisions");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL))
				v->sv_useMeshCollisions = LUA->GetBool(-1);
			LUA->Pop();
		}
		else {
			LUA->GetField(2, "drawExterior");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL))
				v->cl_drawExterior = LUA->GetBool(-1);
			LUA->Pop();

			LUA->GetField(2, "atlasMaterial");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::STRING))
				v->cl_atlasMaterial = iface_materials->FindMaterial(LUA->GetString(-1), nullptr);
			else
				v->cl_atlasMaterial = iface_materials->FindMaterial("models/debug/debugwhite", nullptr);
			v->cl_atlasMaterial->IncrementReferenceCount();
			v->cl_pixel_bias_x = 1.0 / v->cl_atlasMaterial->GetMappingWidth();
			v->cl_pixel_bias_y = 1.0 / v->cl_atlasMaterial->GetMappingHeight();
			LUA->Pop();

			LUA->GetField(2, "atlasWidth");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER))
				v->cl_atlasWidth = LUA->GetNumber();
			LUA->Pop();

			LUA->GetField(2, "atlasHeight");
			if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER))
				v->cl_atlasHeight = LUA->GetNumber();
			LUA->Pop();
		}
	}

	LUA->GetField(2, "dimensions");
	if (LUA->IsType(-1, GarrysMod::Lua::Type::VECTOR)) {
		LUA->GetField(-1, "x");
		v->_dimX = LUA->GetNumber(-1);
		LUA->GetField(-2, "y");
		v->_dimY = LUA->GetNumber(-1);
		LUA->GetField(-3, "z");
		v->_dimZ = LUA->GetNumber(-1);
		LUA->Pop(3);
	}
	LUA->Pop();

	LUA->GetField(2, "scale");
	if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
		double n = LUA->GetNumber();
		if (n>0) {
			v->_scale = n;
		}
	}
	LUA->Pop();

	LUA->GetField(2, "voxelTypes");
	if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
		LUA->PushNil();
		while (LUA->Next(-2)) {
			if (LUA->IsType(-2, GarrysMod::Lua::Type::NUMBER)) {
				VoxelType& vt = v->voxelTypes[(int)LUA->GetNumber(-2)];
				vt.form = VFORM_CUBE;
				if (LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
					LUA->GetField(-1, "atlasIndex");
					if (LUA->IsType(-1, GarrysMod::Lua::Type::NUMBER)) {
						int atlasIndex = LUA->GetNumber(-1);
						vt.atlasX = atlasIndex % v->cl_atlasWidth;
						vt.atlasY = atlasIndex / v->cl_atlasWidth;
					}
					LUA->Pop();
				}
			}
			LUA->Pop();
		}
	}
	LUA->Pop();

	LUA->GetField(2, "_ent");
	LUA->GetField(-1, "_initMisc");
	LUA->Push(-2);
	double x, y, z;
	v->getRealSize(x,y,z);
	LUA->PushNumber(x);
	LUA->PushNumber(y);
	LUA->PushNumber(z);
	LUA->Call(4, 0);
	LUA->Pop();

	v->chunks = new VoxelChunk*[v->_dimX*v->_dimY*v->_dimZ]();

	if (STATE_SERVER) {
		for (int x = 0; x < v->_dimX; x++) {
			for (int y = 0; y < v->_dimY; y++) {
				for (int z = 0; z < v->_dimZ; z++) {
					VoxelChunk* newChunk = new VoxelChunk(v, x, y, z);
					v->chunks[x + y*v->_dimX + z* v->_dimX* v->_dimY] = newChunk;
				}
			}
		}
		v->flagAllChunksForUpdate();
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
		return 0;
	}

	int chunk_n = LUA->GetNumber(2);

	char data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2];

	int out_len = v->getChunkData(chunk_n, data);
	if (out_len == 0) {
		return 0;
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
		
		v->addChunk(LUA->GetNumber(2), data, len);
	}
	return 0;
}

int luaf_voxInitFinish(lua_State* state) {
	int index = LUA->GetNumber(1);

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr) {
		v->flagAllChunksForUpdate();
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

CBaseEntity* elua_getEntity(lua_State* state,int index) {
	if (STATE_CLIENT)
		return nullptr;

	GarrysMod::Lua::UserData* ud = (GarrysMod::Lua::UserData*)(LUA->GetUserdata(index));

	int e_index = ((CBaseHandle*)(ud->data))->GetEntryIndex();

	auto edict = iface_sv_ents->PEntityOfEntIndex(e_index);

	if (edict==nullptr)
		return nullptr;
	return edict->GetUnknown()->GetBaseEntity();
}

Vector elua_getVector(lua_State* state, int index) {
	GarrysMod::Lua::UserData* ud = (GarrysMod::Lua::UserData*)(LUA->GetUserdata(index));
	Vector v = *((Vector*)(ud->data));
	return v;
}

int luaf_voxUpdate(lua_State* state) {
	int index = LUA->GetNumber(1);
	int chunk_count = LUA->GetNumber(2);

	CBaseEntity* ent;
	Vector pos;

	ent = elua_getEntity(state, 3);
	pos = elua_getVector(state, 4);

	Voxels* v = getIndexedVoxels(index);

	if (v != nullptr) {
		v->doUpdates(10, ent, pos);
	}
	
	return 0;
}
/*
int luaf_ENT_Think(lua_State* state) {
	LUA->GetField(1, "GetInternalIndex");
	LUA->Push(1);
	LUA->Call(1, 1);
	int index = LUA->GetNumber();
	LUA->Pop();
	
	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr) {
		CBaseEntity* ent;
		Vector pos;
		
		if (STATE_SERVER) {
			//v->doTransmit(index);

			LUA->GetField(1, "EntIndex");
			LUA->Push(1);
			LUA->Call(1, 1);
			int e_index = LUA->GetNumber();
			LUA->Pop();

			ent = iface_sv_ents->PEntityOfEntIndex(e_index)->GetUnknown()->GetBaseEntity();

			LUA->GetField(1, "GetPos");
			LUA->Push(1);
			LUA->Call(1, 1);

			LUA->GetField(-1, "x");
			pos.x = LUA->GetNumber();
			LUA->GetField(-2, "y");
			pos.y = LUA->GetNumber();
			LUA->GetField(-3, "z");
			pos.z = LUA->GetNumber();
			LUA->Pop(4);

			vox_print("real-> %i", e_index);
		}

		v->doUpdates(10, ent, pos);
		elua_getEntity(state, 1);
	}

	if (STATE_SERVER) {
		LUA->PushSpecial(SPECIAL_GLOB);
		LUA->GetField(-1, "CurTime");
		LUA->Call(0, 1);
		double t = LUA->GetNumber();
		LUA->Pop(2);

		LUA->GetField(1, "NextThink");
		LUA->Push(1);
		LUA->PushNumber(t);
		LUA->Call(2, 0);

		LUA->PushBool(true);

		return 1;
	}
	else {
		return 0;
	}
}*/

int luaf_ENT_TestCollision(lua_State* state) {
	LUA->GetField(1, "GetInternalIndex");
	LUA->Push(1);
	LUA->Call(1, 1);
	int index = LUA->GetNumber();
	LUA->Pop();

	Voxels* v = getIndexedVoxels(index);
	if (v != nullptr && v->isInitialized()) {
		LUA->GetField(1, "GetPos");
		LUA->Push(1);
		LUA->Call(1, 1);

		LUA->GetField(-1, "x");
		double offX = LUA->GetNumber();
		LUA->GetField(-2, "y");
		double offY = LUA->GetNumber();
		LUA->GetField(-3, "z");
		double offZ = LUA->GetNumber();
		LUA->Pop(4);

		LUA->GetField(2, "x");
		double startX = LUA->GetNumber();
		LUA->GetField(2, "y");
		double startY = LUA->GetNumber();
		LUA->GetField(2, "z");
		double startZ = LUA->GetNumber();
		LUA->Pop(3);

		startX -= offX;
		startY -= offY;
		startZ -= offZ;

		LUA->GetField(3, "x");
		double deltaX = LUA->GetNumber();
		LUA->GetField(3, "y");
		double deltaY = LUA->GetNumber();
		LUA->GetField(3, "z");
		double deltaZ = LUA->GetNumber();
		LUA->Pop(3);

		VoxelTraceRes r;
		if (LUA->GetBool(4)) {
			LUA->GetField(5, "x");
			double extX = LUA->GetNumber();
			LUA->GetField(5, "y");
			double extY = LUA->GetNumber();
			LUA->GetField(5, "z");
			double extZ = LUA->GetNumber();
			LUA->Pop(3);

			r = v->doTraceHull(startX, startY, startZ, deltaX, deltaY, deltaZ, extX, extY, extZ);
		}
		else {
			r = v->doTrace(startX, startY, startZ, deltaX, deltaY, deltaZ);
		}

		if (r.fraction != -1) {
			LUA->CreateTable();

			LUA->PushNumber(r.fraction);
			LUA->SetField(-2, "Fraction");

			LUA->PushSpecial(SPECIAL_GLOB);
			
			LUA->GetField(-1, "Vector"); //HitPos, Fraction and Normal
			LUA->Push(-1);

			LUA->PushNumber(r.hitX+offX);
			LUA->PushNumber(r.hitY+offY);
			LUA->PushNumber(r.hitZ+offZ);
			LUA->Call(3, 1);
			LUA->SetField(-4, "HitPos");

			LUA->PushNumber(r.normX);
			LUA->PushNumber(r.normY);
			LUA->PushNumber(r.normZ);
			LUA->Call(3, 1);
			LUA->SetField(-3, "Normal");

			LUA->Pop();
		}
		return 1;
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

	LUA->PushCFunction(luaf_voxInitFinish);
	LUA->SetField(-2, "voxInitFinish");

	LUA->PushCFunction(luaf_voxUpdate);
	LUA->SetField(-2, "voxUpdate");

	//LUA->PushCFunction(luaf_ENT_Think);
	//LUA->SetField(-2, "ENT_Think");

	LUA->PushCFunction(luaf_ENT_TestCollision);
	LUA->SetField(-2, "ENT_TestCollision");

	LUA->PushCFunction(luaf_voxGet);
	LUA->SetField(-2, "voxGet");

	LUA->PushCFunction(luaf_voxSet);
	LUA->SetField(-2, "voxSet");

	LUA->PushString(version_string);
	LUA->SetField(-2, "VERSION");

	LUA->SetField(-2, "G_VOX_IMPORTS");

	//init_lua_net(state);

	LUA->GetField(-1, "RunString");
	LUA->PushString(LUA_SRC);
	LUA->Call(1, 0);

	LUA->Pop();
}