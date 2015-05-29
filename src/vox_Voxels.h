#pragma once

#include <map>
#include <set>
#include <list>

#include "materialsystem/imesh.h"
#include "GarrysMod/Lua/Interface.h"

#include "vox_util.h"

#define VOXEL_CHUNK_SIZE 16

class CPhysPolysoup;
class IPhysicsObject;
class CPhysCollide;

enum VoxelForm {
	VFORM_NULL,
	VFORM_CUBE
};

struct AtlasPos {
	AtlasPos(int x, int y) { this->x = x; this->y = y; }
	int x;
	int y;
};

struct VoxelType {
	VoxelForm form = VFORM_NULL;
	AtlasPos side_xPos = AtlasPos(0, 0);
	AtlasPos side_xNeg = AtlasPos(0, 0);
	AtlasPos side_yPos = AtlasPos(0, 0);
	AtlasPos side_yNeg = AtlasPos(0, 0);
	AtlasPos side_zPos = AtlasPos(0, 0);
	AtlasPos side_zNeg = AtlasPos(0, 0);
};

struct VoxelTraceRes {
	double fraction = -1;
	Vector hitPos;
	Vector hitNormal = Vector(0,0,0);
	VoxelTraceRes& operator*(double n) { hitPos *= n; return *this; }
};

struct VoxelConfigClient;
struct VoxelConfigServer;

struct VoxelConfig {
	~VoxelConfig() {
		if (cl_atlasMaterial)
			cl_atlasMaterial->DecrementReferenceCount();
	}
	
	int dimX = 1;
	int dimY = 1;
	int dimZ = 1;
	double scale = 1;

	bool sv_useMeshCollisions = false;

	IMaterial* cl_atlasMaterial = nullptr;
	bool cl_drawExterior = true;
	int cl_atlasWidth = 1;
	int cl_atlasHeight = 1;

	double cl_pixel_bias_x = 0;
	double cl_pixel_bias_y = 0;

	VoxelType voxelTypes[256];
};

class Voxels;
class VoxelChunk;

int newIndexedVoxels(int index = -1);
Voxels* getIndexedVoxels(int index);
void deleteIndexedVoxels(int index);
void deleteAllIndexedVoxels();

class Voxels {
	friend class VoxelChunk;
public:
	~Voxels();
	
	uint16 generate(int x, int y, int z);

	VoxelChunk* addChunk(int chunk_num, const char* data_compressed, int data_len);
	VoxelChunk* getChunk(int x, int y, int z);

	void flagAllChunksForUpdate();

	const int getChunkData(int chunk_num, char* out);

	void initialize(VoxelConfig* config);
	bool isInitialized();

	Vector getExtents();

	void doUpdates(int count, CBaseEntity* ent);

	VoxelTraceRes doTrace(Vector startPos, Vector delta);
	VoxelTraceRes doTraceHull(Vector startPos, Vector delta, Vector extents);

	VoxelTraceRes iTrace(Vector startPos, Vector delta, Vector defNormal);
	VoxelTraceRes iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal);

	void draw();

	uint16 get(int x, int y, int z);
	bool set(int x, int y, int z, uint16 d);
private:
	VoxelChunk** chunks = nullptr;
	std::set<VoxelChunk*> chunks_flagged_for_update;

	VoxelConfig* config = nullptr;
};


class VoxelChunk {
public:
	VoxelChunk(Voxels* sys, int x, int y, int z, bool generate);
	~VoxelChunk();
	void build(CBaseEntity* ent);
	void draw(CMatRenderContextPtr& pRenderContext);

	uint16 get(int x, int y, int z);
	void set(int x, int y, int z, uint16 d);

	uint16 voxel_data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = {};
private:
	void meshClearAll();

	void meshStart();
	void meshStop(CBaseEntity* ent);

	void addFullVoxelFace(int x,int y,int z,int tx, int ty, byte dir);

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