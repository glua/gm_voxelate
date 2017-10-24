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
	void updateMinMax(const VoxelCoordXYZ & chunkPos);
	void fullUpdateMinMax();
	VoxelChunk* getChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z);

	bool isChunkLoaded(VoxelCoordXYZ pos);

	const int getChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z, char * out);
	bool setChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z, const char* data_compressed, int data_len);

	bool loadFromString(std::string contents);

	std::tuple<char*, size_t> writeToString();

	void initialize();

	void forceUpdateAllChunks();

	std::tuple<VoxelCoordXYZ, VoxelCoordXYZ> getExtents();
	double getBlockScale();

	std::vector<VoxelCoordXYZ> getAllChunkPositions(Vector origin);

	VoxelCoordXYZ minChunk = { 0,0,0 };
	VoxelCoordXYZ maxChunk = { 0,0,0 };
	VoxelCoordXYZ minPos = { 0,0,0 };
	VoxelCoordXYZ maxPos = { 0,0,0 };

#ifdef VOXELATE_SERVER
	bool sendChunk(int peerID, VoxelCoordXYZ pos);
	bool sendChunksAround(int peerID, VoxelCoordXYZ pos, VoxelCoord radius = 10);
#endif

	void doUpdates(int count, CBaseEntity * ent, float curTime);

	bool isPositionInside(Vector pos);

	VoxelTraceRes doTrace(Vector startPos, Vector delta);
	VoxelTraceRes doTrace(btVector3 startPos, btVector3 delta);
	VoxelTraceRes doTraceHull(Vector startPos, Vector delta, Vector extents);
	VoxelTraceRes doTraceHull(btVector3 startPos, btVector3 delta, btVector3 extents);

	VoxelTraceRes iTrace(Vector startPos, Vector delta, Vector defNormal);
	VoxelTraceRes iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal);

#ifdef VOXELATE_CLIENT
	btVector3 viewPos = btAdjVector3(0, 0, 0);
	void draw();
#endif

	BlockData get(PreciseVoxelCoord x, PreciseVoxelCoord y, PreciseVoxelCoord z);
	BlockData get(VoxelCoord x, VoxelCoord y, VoxelCoord z);
	bool set(VoxelCoord x, VoxelCoord y, VoxelCoord z, BlockData d, bool flagChunks = true);
	bool set(PreciseVoxelCoord x, PreciseVoxelCoord y, PreciseVoxelCoord z, BlockData d,bool flagChunks=true);

	//bool trackUpdates = false;
	//std::vector<XYZCoordinate> queued_block_updates;

	VoxelConfig config;
private:
	//bool initialised = false;

	std::unordered_map<VoxelCoordXYZ, VoxelChunk*> chunks_map; // ok zerf lmao

	void flagChunk(VoxelCoordXYZ chunk_pos, bool high_priority);

	std::deque<VoxelCoordXYZ> dirty_chunk_queue;
	std::set<VoxelCoordXYZ> dirty_chunk_set;

	// physics
	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
};
