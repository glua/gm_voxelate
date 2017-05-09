#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include <array>
#include <functional>

#include "materialsystem/imesh.h"

#include "glua.h"

#include "vox_util.h"

typedef std::int32_t Coord;
typedef std::array<Coord, 3> XYZCoordinate;

// custom specialization of std::hash can be injected in namespace std
// thanks zerf
namespace std {
	template<> struct hash<XYZCoordinate> {
		std::size_t operator()(XYZCoordinate const& a) const {
			std::size_t h = 2166136261;

			for (const std::int32_t& e : a) {
				h = (h ^ (e >> 24)) * 16777619;
				h = (h ^ ((e >> 16) & 0xff)) * 16777619;
				h = (h ^ ((e >> 8) & 0xff)) * 16777619;
				h = (h ^ (e & 0xff)) * 16777619;
			}

			return h;
		}
	};
};

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
	bool cl_drawExterior = false;
	int cl_atlasWidth = 1;
	int cl_atlasHeight = 1;

	double cl_pixel_bias_x = 0;
	double cl_pixel_bias_y = 0;

	VoxelType voxelTypes[256];
};

class VoxelWorld;
class VoxelChunk;

int newIndexedVoxels(int index = -1);
VoxelWorld* getIndexedVoxels(int index);
void deleteIndexedVoxels(int index);
void deleteAllIndexedVoxels();

class VoxelWorld {
	friend class VoxelChunk;
public:
	~VoxelWorld();

	VoxelChunk* addChunk(Coord x, Coord y, Coord z);
	VoxelChunk* getChunk(Coord x, Coord y, Coord z);

	const int getChunkData(Coord x, Coord y, Coord z, char * out);
	void setChunkData(Coord x, Coord y, Coord z, const char* data_compressed, int data_len);

	void initialize(VoxelConfig* config);
	bool isInitialized();

	Vector getExtents();
	void getCellExtents(Coord& x, Coord &y, Coord &z);

	void flagAllChunksForUpdate();

	void doUpdates(int count, CBaseEntity* ent);
	void enableUpdates(bool enable);

	VoxelTraceRes doTrace(Vector startPos, Vector delta);
	VoxelTraceRes doTraceHull(Vector startPos, Vector delta, Vector extents);

	VoxelTraceRes iTrace(Vector startPos, Vector delta, Vector defNormal);
	VoxelTraceRes iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal);

	void draw();

	uint16 get(Coord x, Coord y, Coord z);
	bool set(Coord x, Coord y, Coord z, uint16 d,bool flagChunks=true);
private:
	bool initialised = false;

	std::unordered_map<XYZCoordinate, VoxelChunk*> chunks_new; // ok zerf lmao
	std::set<VoxelChunk*> chunks_flagged_for_update;

	bool updates_enabled = false;

	VoxelConfig* config = nullptr;
};


class VoxelChunk {
public:
	VoxelChunk(VoxelWorld* sys, int x, int y, int z);
	~VoxelChunk();
	void build(CBaseEntity* ent);
	void draw(CMatRenderContextPtr& pRenderContext);

	uint16 get(int x, int y, int z);
	void set(int x, int y, int z, uint16 d, bool flagChunks);

	uint16 voxel_data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = {};
private:
	void meshClearAll();

	void meshStart();
	void meshStop(CBaseEntity* ent);

	void addFullVoxelFace(int x,int y,int z,int tx, int ty, byte dir);

	int posX, posY, posZ;

	VoxelWorld* system;
	CMeshBuilder meshBuilder;
	IMesh* current_mesh;
	std::list<IMesh*> meshes;
	int verts_remaining;

	CPhysPolysoup* phys_soup;
	IPhysicsObject* phys_obj = nullptr;
	CPhysCollide* phys_collider = nullptr;
};