#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include <array>
#include <functional>
#include <vector>
#include <string>
#include <deque>

#include "materialsystem/imesh.h"

#include "glua.h"

#include "vox_util.h"

// Goddamn types.
typedef uint16 BlockData;
typedef std::int32_t VoxelCoord;
typedef std::array<VoxelCoord, 3> VoxelCoordXYZ;

// custom specialization of std::hash can be injected in namespace std
// thanks zerf
namespace std {
	template<> struct hash<VoxelCoordXYZ> {
		std::size_t operator()(VoxelCoordXYZ const& a) const {
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

// Size of a chunk. Don't change this.
const VoxelCoord VOXEL_CHUNK_SIZE = 16;

class CPhysPolysoup;
class IPhysicsObject;
class CPhysCollide;

// The physical form of a block. The only options right now are nothing, and a cube.
enum VoxelForm {
	VFORM_NULL,
	VFORM_CUBE
};

// 2D Location in an atlas.
struct AtlasPos {
	AtlasPos() {};
	AtlasPos(int x, int y) { this->x = x; this->y = y; }
	int x;
	int y;
};

// A single block type. Sides are valid only if VFORM_CUBE is used. Probably subject to change soon.
struct VoxelType {
	VoxelForm form = VFORM_NULL;
	AtlasPos side_xPos = AtlasPos(0, 0);
	AtlasPos side_xNeg = AtlasPos(0, 0);
	AtlasPos side_yPos = AtlasPos(0, 0);
	AtlasPos side_yNeg = AtlasPos(0, 0);
	AtlasPos side_zPos = AtlasPos(0, 0);
	AtlasPos side_zNeg = AtlasPos(0, 0);
};

// Simplified traceres struct.
struct VoxelTraceRes {
	double fraction = -1;
	Vector hitPos;
	Vector hitNormal = Vector(0,0,0);
	VoxelTraceRes& operator*(double n) { hitPos *= n; return *this; }
};

struct VoxelConfig {

	int dims_x = VOXEL_CHUNK_SIZE;
	int dims_y = VOXEL_CHUNK_SIZE;
	int dims_z = VOXEL_CHUNK_SIZE;

	bool huge = false;

	double scale = 32;

	bool buildPhysicsMesh = false;
	bool buildExterior = false;

	IMaterial* atlasMaterial = nullptr;

	int atlasWidth = 1;
	int atlasHeight = 1;

	VoxelType voxelTypes[256];
};

class VoxelWorld;
class VoxelChunk;

// Voxel worlds have indices. This is used for networking and stuff.
int newIndexedVoxelWorld(int index, VoxelConfig& config);

VoxelWorld* getIndexedVoxelWorld(int index);

void deleteIndexedVoxelWorld(int index);
void checkAllVoxelWorldsDeleted();

// Sets up network handlers.
void voxelworld_initialise_networking_static();

class VoxelWorld {
	friend class VoxelChunk;
public:
	VoxelWorld(VoxelConfig& config);
	~VoxelWorld();

	int worldID = -1;

	VoxelChunk* initChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z);
	VoxelChunk* getChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z);

	const int getChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z, char * out);
	bool setChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z, const char* data_compressed, int data_len);

	bool loadFromString(std::string contents);

	std::tuple<char*, size_t> writeToString();

	void initialize();

	Vector getExtents();
	void getCellExtents(VoxelCoord& x, VoxelCoord &y, VoxelCoord &z);

	std::vector<VoxelCoordXYZ> getAllChunkPositions(Vector origin);

#ifdef VOXELATE_SERVER
	bool sendChunk(int peerID, VoxelCoordXYZ pos);
	bool sendChunksAround(int peerID, VoxelCoordXYZ pos, VoxelCoord radius = 10);
#endif

	void doUpdates(int count, CBaseEntity * ent);

	VoxelTraceRes doTrace(Vector startPos, Vector delta);
	VoxelTraceRes doTraceHull(Vector startPos, Vector delta, Vector extents);

	VoxelTraceRes iTrace(Vector startPos, Vector delta, Vector defNormal);
	VoxelTraceRes iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal);

	void draw();

	BlockData get(VoxelCoord x, VoxelCoord y, VoxelCoord z);
	bool set(VoxelCoord x, VoxelCoord y, VoxelCoord z, BlockData d,bool flagChunks=true);

	//bool trackUpdates = false;
	//std::vector<XYZCoordinate> queued_block_updates;
private:
	//bool initialised = false;

	std::unordered_map<VoxelCoordXYZ, VoxelChunk*> chunks_map; // ok zerf lmao

	void flagChunk(VoxelCoordXYZ chunk_pos, bool high_priority);

	std::deque<VoxelCoordXYZ> dirty_chunk_queue;
	std::set<VoxelCoordXYZ> dirty_chunk_set;

	VoxelConfig config;
};

// Used for building chunk slice meshes.
struct SliceFace {
	bool present;
#ifdef VOXELATE_CLIENT
	bool direction;
	AtlasPos texture;

	bool operator== (const SliceFace& other) const {
		return
			present == other.present &&
			direction == other.direction &&
			texture.x == other.texture.x &&
			texture.y == other.texture.y;
	}
#else
	bool operator== (const SliceFace& other) const {
		return present == other.present;
	}
#endif
};

class VoxelChunk {
public:
	VoxelChunk(VoxelWorld* sys, int x, int y, int z);
	~VoxelChunk();

	void generate();

	void build(CBaseEntity* ent);
	void draw(CMatRenderContextPtr& pRenderContext);

	VoxelCoordXYZ getWorldCoords();

	BlockData get(int x, int y, int z);
	void set(int x, int y, int z, BlockData d, bool flagChunks);

	int posX, posY, posZ;

	BlockData voxel_data[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = {};
private:
	void meshClearAll();

	void meshStart();
	void meshStop(CBaseEntity* ent);
	
	void buildSlice(int slice, byte dir, SliceFace faces[VOXEL_CHUNK_SIZE][VOXEL_CHUNK_SIZE], int upper_bound_x, int upper_bound_y);

	void addSliceFace(int slice, int x, int y, int w, int h, int tx, int ty, byte dir);

	VoxelWorld* world;
	CMeshBuilder meshBuilder;
	IMesh* current_mesh = nullptr;
	std::list<IMesh*> meshes;
	int verts_remaining = 0;

	CPhysPolysoup* phys_soup = nullptr;
	IPhysicsObject* phys_obj = nullptr;
	CPhysCollide* phys_collider = nullptr;
};