#pragma once

#include <array>
#include <list>
#include <functional>
#include <tuple>

#include "glua.h"

#include "vox_util.h"
#include "vox_engine.h"
// #include "vox_lua_net.h"
#include "vox_network.h"

#include "fastlz.h"

#include "collisionutils.h"

#include "materialsystem/imesh.h"

#include "btBulletDynamicsCommon.h"

const int VOXEL_VERT_FMT = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_VERTEX_SHADER | VERTEX_USERDATA_SIZE(4) | VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2);

#define DIR_X_POS 1
#define DIR_Y_POS 2
#define DIR_Z_POS 3

#define DIR_X_NEG 4
#define DIR_Y_NEG 5
#define DIR_Z_NEG 6

// TODO re-calibrate this for greedy meshing
#define BUILD_MAX_VERTS (VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*4*2)

// #ifdef VOXELATE_SERVER
// #define VOXELATE_SERVER_VPHYSICS 1
// #endif

// Goddamn types.
typedef uint16 BlockData;
typedef std::int32_t VoxelCoord;
typedef double PreciseVoxelCoord;
typedef std::array<VoxelCoord, 3> VoxelCoordXYZ;

// Size of a chunk. Don't change this.
const VoxelCoord VOXEL_CHUNK_SIZE = 16;

const VoxelCoord VOXEL_INIT_X = 16;
const VoxelCoord VOXEL_INIT_Y = 16;
const VoxelCoord VOXEL_INIT_Z = 16;

extern Vector BulletPositionToSource(btVector3 vec);
extern btVector3 SourcePositionToBullet(Vector vec);

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
	double scale = 32;

	bool buildPhysicsMesh = false;

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

enum ELevelOfDetail {
	FULL = 0,
	TWO_MERGE,
	FOUR_MERGE,
	EIGHT_MERGE,
	CHUNK_MERGE,
};

int div_floor(int x, int y);
