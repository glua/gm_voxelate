#include "vox_voxelworld.h"
#include "vox_chunk.h"

std::unordered_map<int,VoxelWorld*> indexedVoxelWorldRegistry;

int newIndexedVoxelWorld(int index, VoxelConfig& config) {
	if (index == -1) {
		auto idx = indexedVoxelWorldRegistry.size();

		while (indexedVoxelWorldRegistry[idx] != nullptr) {
			idx++;
		}

		auto world = new VoxelWorld(config);

		world->worldID = idx;

		indexedVoxelWorldRegistry[idx] = world;

		return idx;
	}
	else {
		indexedVoxelWorldRegistry[index] = new VoxelWorld(config);
		return index;
	}
}

VoxelWorld* getIndexedVoxelWorld(int index) {
	auto it = indexedVoxelWorldRegistry.find(index);

	if (it != indexedVoxelWorldRegistry.end()) {
		return it->second;
	}

	return nullptr;
}

void deleteIndexedVoxelWorld(int index) {
	auto it = indexedVoxelWorldRegistry.find(index);

	if (it != indexedVoxelWorldRegistry.end()) {
		VoxelWorld* ptr = it->second;
		delete ptr;

		indexedVoxelWorldRegistry.erase(it);
	}
}

void checkAllVoxelWorldsDeleted() {
	for (auto it : indexedVoxelWorldRegistry) {
		if (it.second != nullptr) {
			vox_print("LEEK: VOXELWORLD EXISTS AT MODULE UNLOAD!");
		}
	}

	indexedVoxelWorldRegistry.clear();
}

VoxelWorld::VoxelWorld(VoxelConfig& config) {
	this->config = config;

	if (config.atlasMaterial != nullptr)
		config.atlasMaterial->IncrementReferenceCount();

	// physics

	collisionConfiguration = new btDefaultCollisionConfiguration();

	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	overlappingPairCache = new btDbvtBroadphase();

	solver = new btSequentialImpulseConstraintSolver;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	initialize();
}

VoxelWorld::~VoxelWorld() {
	for (auto it : chunks_map) {
		// Pretty sure chunk pointers should never be null but I guess it can't hurt to check
		if (it.second != nullptr) {
			delete it.second;
		}
	}

	// Don't think this is needed but W/E
	chunks_map.clear();

	if (config.atlasMaterial != nullptr)
		config.atlasMaterial->DecrementReferenceCount();
}

VoxelChunk* VoxelWorld::initChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	VoxelCoordXYZ pos = { x, y, z };
	auto iter = chunks_map.find(pos);

	if (iter != chunks_map.end())
		return iter->second;

	VoxelChunk* chunk = new VoxelChunk(this, x, y, z);

	chunks_map.insert({ pos, chunk });

	flagChunk(pos, false);

	// Flag our three neighbors that share faces

	VoxelChunk* neighbor;

	flagChunk({ x - 1, y, z }, false);
	flagChunk({ x, y - 1, z }, false);
	flagChunk({ x, y, z - 1 }, false);

	updateMinMax(pos);

	return chunk;
}

void VoxelWorld::updateMinMax(const VoxelCoordXYZ & chunkPos) {
	if (minChunk[0] > chunkPos[0]) {
		// x min
		minChunk[0] = chunkPos[0];
		minPos[0] = chunkPos[0] * VOXEL_CHUNK_SIZE;
	}
	else if (maxChunk[0] < chunkPos[0]) {
		// x max
		maxChunk[0] = chunkPos[0];
		maxPos[0] = (chunkPos[0] + 1) * VOXEL_CHUNK_SIZE - 1;
	}

	if (minChunk[1] > chunkPos[1]) {
		// y min
		minChunk[1] = chunkPos[1];
		minPos[1] = chunkPos[1] * VOXEL_CHUNK_SIZE;
	}
	else if (maxChunk[1] < chunkPos[1]) {
		// y max
		maxChunk[1] = chunkPos[1];
		maxPos[1] = (chunkPos[1] + 1) * VOXEL_CHUNK_SIZE - 1;
	}

	if (minChunk[2] > chunkPos[2]) {
		// z min
		minChunk[2] = chunkPos[2];
		minPos[2] = chunkPos[2] * VOXEL_CHUNK_SIZE;
	}
	else if (maxChunk[2] < chunkPos[2]) {
		// z max
		maxChunk[2] = chunkPos[2];
		maxPos[2] = (chunkPos[2] + 1) * VOXEL_CHUNK_SIZE - 1;
	}
}

void VoxelWorld::fullUpdateMinMax() {
	minChunk = { 0,0,0 };
	maxChunk = { 0,0,0 };

	for (auto it : chunks_map) {
		updateMinMax(it.first);
	}
}

VoxelChunk* VoxelWorld::getChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	auto iter = chunks_map.find({ x, y, z });

	if (iter == chunks_map.end())
		return nullptr;

	return iter->second;
}

// takes in CHUNK coordinates
bool VoxelWorld::isChunkLoaded(VoxelCoordXYZ pos) {
	auto iter = chunks_map.find(pos);

	return iter != chunks_map.end();
}

// Fills a buffer at out with COMPRESSED chunk data, returns size.

const int VoxelWorld::getChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z,char* out) {
	auto iter = chunks_map.find({ x, y, z });

	if (iter == chunks_map.end())
		return 0;

	const char* input = reinterpret_cast<const char*>(iter->second->voxel_data);

	return fastlz_compress(input, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2, out);
}

bool VoxelWorld::setChunkData(VoxelCoord x, VoxelCoord y, VoxelCoord z, const char* data_compressed, int data_len) {
	if (data_compressed == nullptr) {
		vox_print("NULL DATA %i", data_len);
		return false;
	}

	VoxelChunk* chunk = initChunk(x, y, z);

	auto res = fastlz_decompress(data_compressed, data_len, chunk->voxel_data, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2);

	if (res == 0) {
		vox_print("VoxelWorld::setChunkData -> FastLZ decompression failed! [%i, %i, %i]", x, y, z);
		return false;
	}

	return true;
}

// DON'T USE THIS
bool VoxelWorld::loadFromString(std::string contents) {
	if (contents.size() == 0) {
		return 0;
	}

	bf_read reader;
	reader.StartReading(contents.c_str(), contents.size());

	auto magic = reader.ReadUBitLong(32);
	if (magic != 0xB00B1351) { // boobies v1
		// bad header
		return false;
	}

	auto totalChunks = reader.ReadUBitLong(32);

	if (reader.IsOverflowed()) {
		return false;
	}

	std::unordered_map<VoxelCoordXYZ, std::tuple<size_t, char*>> toLoad;

	for (unsigned int i = 0; i < totalChunks; i++) {
		VoxelCoord chunkX = reader.ReadUBitLong(32);
		VoxelCoord chunkY = reader.ReadUBitLong(32);
		VoxelCoord chunkZ = reader.ReadUBitLong(32);

		auto dataSize = reader.ReadUBitLong(14);

		if (reader.IsOverflowed()) {
			return false;
		}

		auto chunkData = new(std::nothrow) char[CHUNK_BUFFER_SIZE];
		reader.ReadBytes(chunkData, dataSize);

		if (reader.IsOverflowed()) {
			return false;
		}

		toLoad[{chunkX, chunkY, chunkZ}] = std::make_tuple(dataSize, chunkData);
	}

	// if we're here, the chunk read was successful

	auto success = true;

	for (auto it : toLoad) {
		if (!setChunkData(it.first[0], it.first[1], it.first[2], std::get<1>(it.second), std::get<0>(it.second))) {
			// well we're fucked

			success = false;
			break;
		}
	}

	for (auto it : toLoad) {
		// delete[std::get<0>(it.second)] std::get<1>(it.second);
		// TODO: ^ fucks up in linux builds
	}

	toLoad.clear();

	return success;
}

// DON'T USE THIS EITHER
std::tuple<char*,size_t> VoxelWorld::writeToString() {
	auto chunksToWrite = chunks_map.size();
	auto worldData = new(std::nothrow) char[CHUNK_BUFFER_SIZE * chunksToWrite + 32];

	bf_write writer;
	writer.StartWriting(worldData, CHUNK_BUFFER_SIZE * chunksToWrite + 32);

	writer.WriteUBitLong(0xB00B1351, 32);
	writer.WriteUBitLong(chunksToWrite, 32);

	for (auto it : chunks_map) {
		char buffer[CHUNK_BUFFER_SIZE];
		int compressed_size = getChunkData(it.first[0], it.first[1], it.first[2], buffer);

		writer.WriteUBitLong(compressed_size, 32);
		writer.WriteBytes(buffer, compressed_size);
	}

	return std::make_tuple( worldData,writer.GetNumBytesWritten() );
}

// used to be called by some stupid shit
// now always called by ctor
void VoxelWorld::initialize() {
#ifdef VOXELATE_SERVER
	// YO 3D LOOP TIME NIGGA
	// Only explicitly init the map on the server

	Msg("Init chunks...\n");

	for (VoxelCoord x = -VOXEL_INIT_X; x <= VOXEL_INIT_X; x++) {
		for (VoxelCoord y = -VOXEL_INIT_X; y <= VOXEL_INIT_Y; y++) {
			for (VoxelCoord z = -VOXEL_INIT_X; z <= VOXEL_INIT_Z; z++) {
				initChunk(x, y, z)->generate();
			}
		}
	}

	Msg("OK! forcing update...\n");

	forceUpdateAllChunks();
#endif
}

void VoxelWorld::forceUpdateAllChunks() {
	Msg("Updating...\n");
	while (!dirty_chunk_queue.empty()) {
		VoxelCoordXYZ pos = dirty_chunk_queue.front();

		VoxelChunk* chunk = getChunk(pos[0], pos[1], pos[2]);

		if (chunk != nullptr) {
			chunk->build(ELevelOfDetail::FULL);
		}

		dirty_chunk_queue.pop_front();
		dirty_chunk_set.erase(pos);
	}
}

// Warning: We don't give a shit about this with huge worlds!
std::tuple<VoxelCoordXYZ, VoxelCoordXYZ> VoxelWorld::getExtents() {
	return { minPos, maxPos };
}

double VoxelWorld::getBlockScale() {
	return config.scale;
}

/*void VoxelWorld::getCellExtents(VoxelCoord& x, VoxelCoord& y, VoxelCoord& z) {

	x = config.dims_x;
	y = config.dims_y;
	z = config.dims_z;
}*/

// Get all chunk positions
// order by distance to localized origin
// Can use something similar to get NEARBY chunks later...
std::vector<VoxelCoordXYZ> VoxelWorld::getAllChunkPositions(Vector origin) {
	return getAllChunkPositions(SourcePositionToBullet(origin));
}

std::vector<VoxelCoordXYZ> VoxelWorld::getAllChunkPositions(btVector3 origin) {
	// todo: make this return chunks via a fixed algorithm that radiates outward in a spiral

	// get them chunks
	std::vector<VoxelCoordXYZ> positions;

	for (auto pair : chunks_map) {
		positions.push_back( pair.first );
	}

	// translate origin to a chunk coordinate
	origin /= (VOXEL_CHUNK_SIZE * config.scale);


	VoxelCoordXYZ origin_coord = { origin.x(), origin.z(), -origin.y() };

	// sort them chunks
	auto sorter = [&](VoxelCoordXYZ c1, VoxelCoordXYZ c2) {
		int d1x = origin_coord[0] - c1[0];
		int d1y = origin_coord[1] - c1[1];
		int d1z = origin_coord[2] - c1[2];

		int d2x = origin_coord[0] - c2[0];
		int d2y = origin_coord[1] - c2[1];
		int d2z = origin_coord[2] - c2[2];

		int dist1 = d1x*d1x + d1y*d1y + d1z*d1z;
		int dist2 = d2x*d2x + d2y*d2y + d2z*d2z;

		return dist1 > dist2;
	};

	std::sort(positions.begin(), positions.end(), sorter);

	return positions;
}

// Updates up to n chunks
// Logic probably okay for huge worlds, although we may have to double check that the chunk still exists,
// or clean out chunks_flagged_for_update when we unload chunks
// TODO: convert Vector to AdvancedVector
void VoxelWorld::doUpdates(int count, float curTime) {
	// On the server, we -NEED- the entity. Not so important on the client
	
	if (!IS_SERVERSIDE || (config.buildPhysicsMesh)) {
		for (int i = 0; i < count && !dirty_chunk_queue.empty(); i++) {
			VoxelCoordXYZ pos = dirty_chunk_queue.front();
			dirty_chunk_queue.pop_front();
			dirty_chunk_set.erase(pos);

			VoxelChunk* chunk = getChunk(pos[0], pos[1], pos[2]);

			if (chunk != nullptr) {
				chunk->build(ELevelOfDetail::FULL);
			}
		}
	}

	// verbatim from http://www.reactphysics3d.com/usermanual.html
	// only updates the physics engine @ 60Hz

	static float previousFrameTime = 0;
	static float accumulator = 0;
	
	if (previousFrameTime == 0) {
		previousFrameTime = curTime;
	}

	// Constant physics time step 
	const float timeStep = 1.0 / 60.0;

	// Compute the time difference between the two frames 
	float deltaTime = curTime - previousFrameTime;

	// Update the previous time 
	previousFrameTime = curTime;

	// Add the time difference in the accumulator 
	accumulator += deltaTime;

	if (accumulator != 0) {
		// While there is enough accumulated time to take 
		// one or several physics steps 
		while (accumulator >= timeStep) {

			// Update the Dynamics world with a constant time step 
			dynamicsWorld->stepSimulation(timeStep, 10);

			// Decrease the accumulated time 
			accumulator -= timeStep;
		}
	}
}

bool VoxelWorld::isPositionInside(Vector pos) {
	if (pos.x < maxPos[0]) {
		if (pos.x > minPos[0]) {
			if (pos.y < maxPos[1]) {
				if (pos.y > minPos[1]) {
					if (pos.z < maxPos[2]) {
						if (pos.z > minPos[2]) {
							// welcome to the great pyramids of giza
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

inline btVector3 SourcePositionToBullet(Vector pos) {
	return btVector3(pos.x, pos.z, -pos.y);
}

inline Vector BulletPositionToSource(btVector3 pos) {
	return Vector(pos.x(), -pos.z(), pos.y());
}

// Function for line traces. Re-scales vectors and moves the start to the beggining of the voxel entity,
// Then calls fast trace function
VoxelTraceRes VoxelWorld::doTrace(Vector startPos, Vector delta) {
	return doTrace(SourcePositionToBullet(startPos), SourcePositionToBullet(delta));
}

VoxelTraceRes VoxelWorld::doTrace(btVector3 startPos, btVector3 delta) {
	auto endPos = startPos + delta;

	btCollisionWorld::ClosestRayResultCallback RayCallback(startPos, endPos);

	dynamicsWorld->rayTest(startPos, endPos, RayCallback);

	VoxelTraceRes res;

	if (RayCallback.hasHit()) {
		res.fraction = RayCallback.m_closestHitFraction;
		res.hitPos = RayCallback.m_hitPointWorld;
		res.hitNormal = RayCallback.m_hitNormalWorld;
	}

	return res;
}

// Same as above for hull traces.
// TODO deal with assumption mentioned below...?
VoxelTraceRes VoxelWorld::doTraceHull(Vector startPos, Vector delta, Vector extents) {
	return doTraceHull(
		SourcePositionToBullet(startPos),
		SourcePositionToBullet(delta),
		SourcePositionToBullet(extents)
	);
}

VoxelTraceRes VoxelWorld::doTraceHull(btVector3 startPos, btVector3 delta, btVector3 extents) {
	auto endPos = startPos + delta;

	btBoxShape *box = new btBoxShape(extents.absolute());

	btCollisionWorld::ClosestConvexResultCallback RayCallback(startPos, endPos);

	btTransform startTransform(btMatrix3x3::getIdentity(), startPos);
	btTransform endTransform(btMatrix3x3::getIdentity(), endPos);

	dynamicsWorld->convexSweepTest(box, startTransform, endTransform, RayCallback);

	VoxelTraceRes res;

	if (RayCallback.hasHit()) {
		res.fraction = RayCallback.m_closestHitFraction;
		if (RayCallback.m_closestHitFraction == 0) {
			res.hitPos = startPos;
			res.hitNormal = RayCallback.m_hitNormalWorld;
			// res.hitNormal = btVector3(0,0,0);
		}
		else {
			res.hitPos = RayCallback.m_hitPointWorld;
			res.hitNormal = RayCallback.m_hitNormalWorld;
		}
	}

	delete box;

	return res;
}

// Render every single chunk.
// TODO for huge worlds, only render close chunks
#ifdef VOXELATE_CLIENT
void VoxelWorld::draw() {

	IMaterial* atlasMat = config.atlasMaterial;

	CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);

	//Bind material
	pRenderContext->Bind(atlasMat);

	//Set lighting
	//Vector4D lighting_cube[] = { Vector4D(.7, .7, .7, 1), Vector4D(.3, .3, .3, 1), Vector4D(.5, .5, .5, 1), Vector4D(.5, .5, .5, 1), Vector4D(1, 1, 1, 1), Vector4D(.1, .1, .1, 1) };
	//pRenderContext->SetAmbientLightCube(lighting_cube);
	//pRenderContext->SetAmbientLight(0, 0, 0);
	//pRenderContext->DisableAllLocalLights();

	// TODO: only draw nearby chunks
	for (auto pair : chunks_map) {
		pair.second->draw(pRenderContext);
	}
}
#endif

//floored division, credit http://www.microhowto.info/howto/round_towards_minus_infinity_when_dividing_integers_in_c_or_c++.html
int div_floor(int x, int y) {
	int q = x / y;
	int r = x%y;
	if ((r != 0) && ((r<0) != (y<0))) --q;
	return q;
}

VoxelCoord preciseToNormal(PreciseVoxelCoord coord) {
	if (coord >= 0) {
		return (VoxelCoord)coord; // cast to int and discard all decimal places
	}
	else {
		return (VoxelCoord)floor(coord); // floor to next "biggest" negative number
	}
}

VoxelCoord chunkRel(VoxelCoord coord, VoxelCoord chunkCoord) {
	if (coord >= 0) {
		return coord - chunkCoord * VOXEL_CHUNK_SIZE;
	}
	else {
		return coord + abs(chunkCoord) * VOXEL_CHUNK_SIZE;
	}
}

// Gets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
BlockData VoxelWorld::get(PreciseVoxelCoord x, PreciseVoxelCoord y, PreciseVoxelCoord z) {
	return get(preciseToNormal(x), preciseToNormal(y), preciseToNormal(z));
}

BlockData VoxelWorld::get(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	auto chunkX = div_floor(x, VOXEL_CHUNK_SIZE);
	auto chunkY = div_floor(y, VOXEL_CHUNK_SIZE);
	auto chunkZ = div_floor(z, VOXEL_CHUNK_SIZE);

	VoxelChunk* chunk = getChunk(chunkX, chunkY, chunkZ);
	if (chunk == nullptr) {
		return 0;
	}

	return chunk->get(chunkRel(x, chunkX), chunkRel(y, chunkY), chunkRel(z, chunkZ));
}

// Sets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
bool VoxelWorld::set(PreciseVoxelCoord x, PreciseVoxelCoord y, PreciseVoxelCoord z, BlockData d, bool flagChunks) {
	auto x2 = preciseToNormal(x);
	auto y2 = preciseToNormal(y);
	auto z2 = preciseToNormal(z);

	return set(x2, y2, z2, d, flagChunks);
}

// Sets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
bool VoxelWorld::set(VoxelCoord x, VoxelCoord y, VoxelCoord z, BlockData d, bool flagChunks) {
	auto chunkX = div_floor(x, VOXEL_CHUNK_SIZE);
	auto chunkY = div_floor(y, VOXEL_CHUNK_SIZE);
	auto chunkZ = div_floor(z, VOXEL_CHUNK_SIZE);

	VoxelChunk* chunk = getChunk(chunkX, chunkY, chunkZ);

	if (chunk == nullptr) {
		chunk = initChunk(chunkX, chunkY, chunkZ);
	}

	chunk->set(chunkRel(x, chunkX), chunkRel(y, chunkY), chunkRel(z, chunkZ), d, flagChunks);

	return true;
}

void VoxelWorld::flagChunk(VoxelCoordXYZ chunk_pos, bool high_priority) {
	if (!dirty_chunk_set.count(chunk_pos)) {
		dirty_chunk_set.insert(chunk_pos);
		if (high_priority) {
			dirty_chunk_queue.push_front(chunk_pos);
		}
		else {
			dirty_chunk_queue.push_back(chunk_pos);
		}
	}
}
