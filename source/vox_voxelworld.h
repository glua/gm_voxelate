#pragma once

#include <cmath>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <deque>

#include "vox_world_common.h"

class VoxelChunk;

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
	double getBlockScale();

	std::vector<VoxelCoordXYZ> getAllChunkPositions(Vector origin);

#ifdef VOXELATE_SERVER
	bool sendChunk(int peerID, VoxelCoordXYZ pos);
	bool sendChunksAround(int peerID, VoxelCoordXYZ pos, VoxelCoord radius = 10);
#endif

	void doUpdates(int count, CBaseEntity * ent, float curTime);

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

	// physics
	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
};
