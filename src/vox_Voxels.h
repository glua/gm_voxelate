#pragma once

#include <map>
#include <set>
#include <list>
#include "materialsystem/imesh.h"
#include "GarrysMod/Lua/Interface.h"


#define VOXEL_CHUNK_SIZE 16

class CPhysPolysoup;
class IPhysicsObject;
class CPhysCollide;

enum VoxelForm {
	VFORM_NULL,
	VFORM_CUBE
};


struct VoxelType {
	VoxelForm form = VFORM_NULL;
	int atlasX = 0;
	int atlasY = 0;
};

struct VoxelTraceRes {
	double fraction = -1;
	double hitX, hitY, hitZ;
	double normX, normY, normZ;
	VoxelTraceRes& operator*(double n) { hitX *= n; hitY *= n; hitZ *= n; return *this; }
};

class Voxels;
class VoxelChunk;

int newIndexedVoxels(int index = -1);
Voxels* getIndexedVoxels(int index);
void deleteIndexedVoxels(int index);
void deleteAllIndexedVoxels();

class Voxels {
	friend class VoxelChunk;
	friend int luaf_voxInit(lua_State* state);
public:
	~Voxels();
	
	//void init(lua_State* state);
	uint16 generate(int x, int y, int z);

	void pushConfig(lua_State* state);
	void deleteConfig(lua_State* state);

	VoxelChunk* addChunk(int chunk_num, const char* data_compressed, int data_len);
	VoxelChunk* getChunk(int x, int y, int z);

	void flagAllChunksForUpdate();

	const int getChunkData(int chunk_num, char* out);

	bool isInitialized();

	void getRealSize(double& x, double& y, double& z);

	//void beginTransmit(int id);
	//void doTransmit(int sys_index);

	void doUpdates(int count, CBaseEntity* ent, const Vector& pos);

	VoxelTraceRes doTrace(double startX, double startY, double startZ, double deltaX, double deltaY, double deltaZ);
	VoxelTraceRes doTraceHull(double startX, double startY, double startZ, double deltaX, double deltaY, double deltaZ, double extX, double extY, double extZ);

	VoxelTraceRes iTrace(double startX, double startY, double startZ, double deltaX, double deltaY, double deltaZ, double defNormX, double defNormY, double defNormZ);
	VoxelTraceRes iTraceHull(double startX, double startY, double startZ, double deltaX, double deltaY, double deltaZ, double extX, double extY, double extZ, double defNormX, double defNormY, double defNormZ);

	void draw();

	uint16 get(int x, int y, int z);
	bool set(int x, int y, int z, uint16 d);
private:
	VoxelChunk** chunks = nullptr;
	std::set<VoxelChunk*> chunks_flagged_for_update;

	VoxelType voxelTypes[256];

	std::map<int, int> sv_receivers;
	int sv_reg_config;

	IMaterial* cl_atlasMaterial = nullptr;
	bool cl_drawExterior = true;
	int cl_atlasWidth = 1;
	int cl_atlasHeight = 1;

	double cl_pixel_bias_x = 0;
	double cl_pixel_bias_y = 0;

	bool sv_useMeshCollisions = false;

	int _dimX = 1;
	int _dimY = 1;
	int _dimZ = 1;
	double _scale = 1;
};


class VoxelChunk {
public:
	VoxelChunk(Voxels* sys, int x, int y, int z);
	~VoxelChunk();
	void build(CBaseEntity* ent, const Vector& pos);
	void draw(CMatRenderContextPtr& pRenderContext);

	uint16 get(int x, int y, int z);
	void set(int x, int y, int z, uint16 d);

	//void send(int sys_index, int ply_id, bool init, int chunk_num);

	uint16 voxel_data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = {};
private:
	void meshClearAll();

	void meshStart();
	void meshStop(CBaseEntity* ent, const Vector& pos);

	void addFullVoxelFace(int x,int y,int z,int tx, int ty, byte dir);

	//IMaterial* tmp_mat;

	int posX, posY, posZ;

	Voxels* system;
	CMeshBuilder meshBuilder;
	IMesh* current_mesh;
	std::list<IMesh*> meshes;
	int verts_remaining;

	CPhysPolysoup* phys_soup;
	IPhysicsObject* phys_obj = nullptr;
	CPhysCollide* phys_collider = nullptr;
};