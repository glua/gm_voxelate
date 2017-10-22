#include "vox_voxelworld.h"
#include "vox_chunk.h"

#include "GarrysMod/LuaHelpers.hpp"

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
	try {
		return indexedVoxelWorldRegistry.at(index);
	}
	catch (...) {
		return nullptr;
	}
}

void deleteIndexedVoxelWorld(int index) {
	try {
		VoxelWorld*& ptr = indexedVoxelWorldRegistry.at(index);
		if (ptr != nullptr) {
			delete ptr;
			ptr = nullptr;
		}
	}
	catch (...) {}
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

	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();

	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();

	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

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
	auto iter = chunks_map.find({ x, y, z });

	if (iter != chunks_map.end())
		return iter->second;

	VoxelChunk* chunk = new VoxelChunk(this, x, y, z);

	chunks_map.insert({ { x, y, z }, chunk });

	flagChunk({ x,y,z }, false);

	// Flag our three neighbors that share faces

	VoxelChunk* neighbor;

	flagChunk({ x - 1, y, z }, false);
	flagChunk({ x, y - 1, z }, false);
	flagChunk({ x, y, z - 1 }, false);

	return chunk;
}

VoxelChunk* VoxelWorld::getChunk(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	auto iter = chunks_map.find({ x, y, z });

	if (iter == chunks_map.end())
		return nullptr;

	return iter->second;
}

// Fills a buffer at out with COMPRESSED chunk data, returns size.

const int CHUNK_BUFFER_SIZE = 9000;

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

	// YO 3D LOOP TIME NIGGA
	// TODO: remove this and only add chunks when entities are nearby
	// TODO: remove this entirely actually, this only generates positive int chunks, but we're going arbitrary...
	if (IS_SERVERSIDE) {
		// Only explicitly init the map on the server
		if (!config.huge) {
			VoxelCoord max_chunk_x = (config.dims_x - 1) / VOXEL_CHUNK_SIZE;
			VoxelCoord max_chunk_y = (config.dims_y - 1) / VOXEL_CHUNK_SIZE;
			VoxelCoord max_chunk_z = (config.dims_z - 1) / VOXEL_CHUNK_SIZE;


			//vox_print("---> %i %i %i",max_chunk_x,max_chunk_y,max_chunk_z);

			for (VoxelCoord x = 0; x <= max_chunk_x; x++) {
				for (VoxelCoord y = 0; y <= max_chunk_y; y++) {
					for (VoxelCoord z = 0; z <= max_chunk_z; z++) {
						initChunk(x, y, z)->generate();
					}
				}
			}
			// todo do we do mapgen here?
		}
		else {
			vox_print("HUGE WORLDS NOT IMPLEMENTED YET!");
		}
	}
}

// Warning: We don't give a shit about this with huge worlds!
Vector VoxelWorld::getExtents() {

	return Vector(
		config.dims_x*config.scale,
		config.dims_y*config.scale,
		config.dims_z*config.scale
	);
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
	
	// get them chunks
	std::vector<VoxelCoordXYZ> positions;

	for (auto pair : chunks_map) {
		positions.push_back( pair.first );
	}

	// translate origin to a chunk coordinate
	origin /= (VOXEL_CHUNK_SIZE * config.scale);


	VoxelCoordXYZ origin_coord = { origin.x, origin.y, origin.z };

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

void voxelworld_initialise_networking_static() {
#ifdef VOXELATE_CLIENT
	vox_networking::channelListen(vox_network_channel::chunk, [&](int peerID, const char* data, size_t data_len) {
		//vox_print("chunk!");

		bf_read reader;
		reader.StartReading(data, data_len);

		int worldID = reader.ReadUBitLong(8);
		//vox_print("hello? %i",worldID);

		auto world = getIndexedVoxelWorld(worldID);

		if (world == NULL) {
			return;
		}

		VoxelCoordXYZ pos = {
			reader.ReadSBitLong(32),
			reader.ReadSBitLong(32),
			reader.ReadSBitLong(32)
		};

		//vox_print("Get chunk %i %i %i %i", worldID, pos[0], pos[1], pos[2]);

		auto dataSize = reader.GetNumBytesLeft();

		char chunkData[CHUNK_BUFFER_SIZE];
		reader.ReadBytes(chunkData, dataSize);

		world->setChunkData(pos[0], pos[1], pos[2], chunkData, dataSize);
	});

	//vox_networking::channelListen(VOX_NETWORK_CHANNEL_CHUNKDATA_RADIUS, [&](int peerID, const char* data, size_t data_len) {
		// not used.

		/*bf_read reader;
		reader.StartReading(data, data_len);

		int worldID = reader.ReadUBitLong(8);

		auto world = getIndexedVoxelWorld(worldID);

		if (world == NULL) {
			return;
		}

		VoxelCoord radius = reader.ReadUBitLong(8);

		XYZCoordinate origin = {
			reader.ReadSBitLong(32),
			reader.ReadSBitLong(32),
			reader.ReadSBitLong(32)
		};

		for (Coord x = 0; x < radius; x++) {
			for (Coord y = 0; y < radius; y++) {
				for (Coord z = 0; z < radius; z++) {
					auto dataSize = reader.ReadUBitLong(16);

					char chunkData[CHUNK_BUFFER_SIZE];
					reader.ReadBytes(chunkData, dataSize);

					world->setChunkData(origin[0] + x, origin[1] + y, origin[2] + z, chunkData, dataSize);
				}
			}
		}*/
	//});
#endif
}

#ifdef VOXELATE_SERVER

bool VoxelWorld::sendChunk(int peerID, VoxelCoordXYZ pos) {
	static char msg[CHUNK_BUFFER_SIZE + 16];

	bf_write writer;
	writer.StartWriting(msg, CHUNK_BUFFER_SIZE + 16);

	writer.WriteUBitLong(vox_network_channel::chunk, 8);
	writer.WriteUBitLong(worldID, 8);

	writer.WriteSBitLong(pos[0], 32);
	writer.WriteSBitLong(pos[1], 32);
	writer.WriteSBitLong(pos[2], 32);

	int compressed_size = getChunkData(pos[0], pos[1], pos[2], msg + writer.GetNumBytesWritten());

	if (compressed_size == 0)
		return false;

	return vox_networking::channelSend(peerID, msg, writer.GetNumBytesWritten() + compressed_size );
}


/*bool VoxelWorld::sendChunksAround(int peerID, VoxelCoordXYZ pos, VoxelCoord radius) {
	auto maxSize = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2 * radius * + 24;

	auto data = new(std::nothrow) uint8_t[maxSize];

	bf_write writer;

	writer.StartWriting(data, maxSize);

	if (data == NULL) {
		return false;
	}

	// write voxel ID

	writer.WriteUBitLong(worldID, 8);

	// write chunk num

	writer.WriteUBitLong(radius, 8);

	// write origin

	writer.WriteSBitLong(pos[0], 32);
	writer.WriteSBitLong(pos[1], 32);
	writer.WriteSBitLong(pos[2], 32);

	for (VoxelCoord x = 0; x < radius; x++) {
		for (VoxelCoord y = 0; y < radius; y++) {
			for (VoxelCoord z = 0; z < radius; z++) {
				char chunkData[CHUNK_BUFFER_SIZE];
				int len = getChunkData(pos[0] + x, pos[1] + y, pos[2] + z, chunkData);

				writer.WriteUBitLong(len, 16);
				writer.WriteBytes(chunkData, len);
			}
		}
	}

	writer.WriteOneBit(0); // null terminate for good measure

	return vox_networking::channelSend(peerID, VOX_NETWORK_CHANNEL_CHUNKDATA_RADIUS, data, writer.GetNumBytesWritten());
}*/
#endif
/*
void VoxelWorld::sortUpdatesByDistance(Vector* origin) {
	// std::function<bool(VoxelChunk*,VoxelChunk*)>

	if (origin != NULL) {
		auto sorter = [&](VoxelChunk* c1, VoxelChunk* c2) {
			auto c1pos = c1->getWorldCoords();
			auto c2pos = c2->getWorldCoords();

			if (c1pos[0] == c2pos[0]) {
				if (c1pos[1] == c2pos[1]) {
					auto Zdist1 = std::pow(c1pos[2] - origin->z, 2);
					auto Zdist2 = std::pow(c2pos[2] - origin->z, 2);

					return Zdist1 < Zdist2;
				}
			}

			auto dist1 = std::pow(c1pos[0] - origin->x, 2) + std::pow(c1pos[1] - origin->y, 2);
			auto dist2 = std::pow(c2pos[0] - origin->x, 2) + std::pow(c2pos[1] - origin->y, 2);

			return dist1 < dist2;
		};

		vox_print("no sorting allowed.");
		//std::sort(chunks_flagged_for_update.begin(), chunks_flagged_for_update.end(), sorter);
	}
}
*/

// Updates up to n chunks
// Logic probably okay for huge worlds, although we may have to double check that the chunk still exists,
// or clean out chunks_flagged_for_update when we unload chunks
// TODO: convert Vector to AdvancedVector
void VoxelWorld::doUpdates(int count, CBaseEntity* ent, float curTime) {
	// On the server, we -NEED- the entity. Not so important on the client
	
	if (!IS_SERVERSIDE || (ent != nullptr && config.buildPhysicsMesh)) {
		for (int i = 0; i < count && !dirty_chunk_queue.empty(); i++) {
			VoxelCoordXYZ pos = dirty_chunk_queue.front();
			dirty_chunk_queue.pop_front();
			dirty_chunk_set.erase(pos);

			VoxelChunk* chunk = getChunk(pos[0], pos[1], pos[2]);

			if (chunk != nullptr) {
				chunk->build(ent);
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

// Function for line traces. Re-scales vectors and moves the start to the beggining of the voxel entity,
// Then calls fast trace function
VoxelTraceRes VoxelWorld::doTrace(Vector startPos, Vector delta) {
	Vector voxel_extents = getExtents();

	if (startPos.WithinAABox(Vector(0,0,0), voxel_extents)) {
		return iTrace(startPos/config.scale , delta/config.scale, Vector(0,0,0)) * config.scale;
	}
	else {
		Ray_t ray;
		ray.Init(startPos,startPos+delta);
		CBaseTrace tr;
		IntersectRayWithBox(ray, Vector(0, 0, 0), voxel_extents, 0, &tr);

		startPos = tr.endpos;
		delta *= 1-tr.fraction;

		return iTrace(startPos / config.scale, delta / config.scale, tr.plane.normal) * config.scale;
	}
}

// Same as above for hull traces.
// TODO deal with assumption mentioned below...?
VoxelTraceRes VoxelWorld::doTraceHull(Vector startPos, Vector delta, Vector extents) {
	Vector voxel_extents = getExtents();

	//Calculate our bounds based on the offsets used by the player hull. This will not work for everything, but will preserve player movement.
	Vector box_lower = startPos - Vector(extents.x, extents.y, 0);

	Vector box_upper = startPos + Vector(extents.x, extents.y, extents.z*2);

	if (IsBoxIntersectingBox(box_lower,box_upper,Vector(0,0,0),voxel_extents)) {
		return iTraceHull(startPos/config.scale,delta/config.scale,extents/config.scale, Vector(0,0,0)) * config.scale;
	}
	else {
		Ray_t ray;
		ray.Init(startPos, startPos + delta, box_lower, box_upper);
		CBaseTrace tr;
		IntersectRayWithBox(ray, Vector(0, 0, 0), voxel_extents, 0, &tr);

		startPos = tr.endpos;
		delta *= 1 - tr.fraction;

		return iTraceHull(startPos / config.scale, delta / config.scale, extents / config.scale, tr.plane.normal) * config.scale;
	}
}

// Fast trace function, based on http://www.cse.chalmers.se/edu/year/2011/course/TDA361/Advanced%20Computer%20Graphics/grid.pdf
VoxelTraceRes VoxelWorld::iTrace(Vector startPos, Vector delta, Vector defNormal) {
	int vx = startPos.x;
	int vy = startPos.y;
	int vz = startPos.z;

	BlockData vdata = get(vx, vy, vz);
	VoxelType& vt = config.voxelTypes[vdata];
	if (vt.form == VFORM_CUBE) {
		VoxelTraceRes res;
		res.fraction = 0;
		res.hitPos = startPos;
		res.hitNormal = defNormal;
		return res;
	}

	int stepX, stepY, stepZ;
	double tMaxX, tMaxY, tMaxZ;

	if (delta.x >= 0) {
		stepX = 1;
		tMaxX = (1 - fmod(startPos.x, 1)) / delta.x;
	}
	else {
		stepX = -1;
		tMaxX = fmod(startPos.x, 1) / -delta.x;
	}

	if (delta.y >= 0) {
		stepY = 1;
		tMaxY = (1 - fmod(startPos.y, 1)) / delta.y;
	}
	else {
		stepY = -1;
		tMaxY = fmod(startPos.y, 1) / -delta.y;
	}

	if (delta.z >= 0) {
		stepZ = 1;
		tMaxZ = (1 - fmod(startPos.z, 1)) / delta.z;
	}
	else {
		stepZ = -1;
		tMaxZ = fmod(startPos.z, 1) / -delta.z;
	}

	double tDeltaX = fabs(1 / delta.x);
	double tDeltaY = fabs(1 / delta.y);
	double tDeltaZ = fabs(1 / delta.z);

	int failsafe = 0;
	while (failsafe++<10000) {
		byte dir = 0;
		if (tMaxX < tMaxY) {
			if (tMaxX < tMaxZ) {
				if (tMaxX>1)
					return VoxelTraceRes();
				vx += stepX;
				tMaxX += tDeltaX;
				if (vx < 0 || vx >= config.dims_x)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= config.dims_z)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		else {
			if (tMaxY < tMaxZ) {
				if (tMaxY>1)
					return VoxelTraceRes();
				vy += stepY;
				tMaxY += tDeltaY;
				if (vy < 0 || vy >= config.dims_y)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= config.dims_z)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		BlockData vdata = get(vx, vy, vz);
		VoxelType& vt = config.voxelTypes[vdata];
		if (vt.form == VFORM_CUBE) {
			VoxelTraceRes res;
			if (dir == DIR_X_POS) {
				res.fraction = tMaxX -tDeltaX;
				res.hitNormal = Vector(-1,0,0);
			}
			else if (dir == DIR_X_NEG) {
				res.fraction = tMaxX - tDeltaX;
				res.hitNormal = Vector(1, 0, 0);
			}
			else if (dir == DIR_Y_POS) {
				res.fraction = tMaxY - tDeltaY;
				res.hitNormal = Vector(0, -1, 0);
			}
			else if (dir == DIR_Y_NEG) {
				res.fraction = tMaxY - tDeltaY;
				res.hitNormal = Vector(0, 1, 0);
			}
			else if (dir == DIR_Z_POS) {
				res.fraction = tMaxZ - tDeltaZ;
				res.hitNormal = Vector(0, 0, -1);
			}
			else if (dir == DIR_Z_NEG) {
				res.fraction = tMaxZ - tDeltaZ;
				res.hitNormal = Vector(0, 0, 1);
			}
			res.hitPos = startPos + res.fraction*delta; // todo plz improve
			return res;
		}
	}

	vox_print("[bail] %f %f %f :: %f %f %f", startPos.x, startPos.y, startPos.z, delta.x, delta.y, delta.z);
	return VoxelTraceRes();
}

int floorCrazy(float f) {
	int floored = floor(f);
	if (floored == f)
		return f - 1;
	return f;
}

// Same as above, for hull traces.
// Has a lot of sketchy shit to prevent players getting stuck :(
VoxelTraceRes VoxelWorld::iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal) {
	double epsilon = .001;

	for (int ix = startPos.x - extents.x + epsilon; ix <= startPos.x + extents.x-epsilon; ix++) {
		for (int iy = startPos.y - extents.y + epsilon; iy <= startPos.y + extents.y-epsilon; iy++) {
			for (int iz = startPos.z + epsilon; iz <= startPos.z + extents.z * 2-epsilon; iz++) {
				BlockData vdata = get(ix, iy, iz);
				VoxelType& vt = config.voxelTypes[vdata];
				if (vt.form == VFORM_CUBE) {
					VoxelTraceRes res;
					res.fraction = 0;
					res.hitPos = startPos;
					res.hitNormal = defNormal;
					return res;
				}
			}
		}
	}

	int vx, vy, vz;
	int stepX, stepY, stepZ;
	double tMaxX, tMaxY, tMaxZ;

	if (delta.x >= 0) {
		vx = startPos.x + extents.x - epsilon;
		stepX = 1;
		double mod = fmod(startPos.x + extents.x, 1);
		if (mod == 0)
			tMaxX = 0;
		else
			tMaxX = (1 - mod) / delta.x;
	}
	else {
		vx = startPos.x - extents.x + epsilon;
		stepX = -1;
		tMaxX = fmod(startPos.x - extents.x, 1) / -delta.x;
	}

	if (delta.y >= 0) {
		vy = startPos.y + extents.y - epsilon;
		stepY = 1;
		double mod = fmod(startPos.y + extents.y, 1);
		if (mod == 0)
			tMaxY = 0;
		else
			tMaxY = (1 - mod) / delta.y;
	}
	else {
		vy = startPos.y - extents.y + epsilon;
		stepY = -1;
		tMaxY = fmod(startPos.y - extents.y, 1) / -delta.y;
	}

	if (delta.z >= 0) {
		vz = startPos.z + extents.z * 2 - epsilon;
		stepZ = 1;
		double mod = fmod(startPos.z + extents.z * 2, 1);
		if (mod == 0)
			tMaxZ = 0;
		else
			tMaxZ = (1 - mod) / delta.z;
	}
	else {
		vz = startPos.z + epsilon;
		stepZ = -1;
		tMaxZ = fmod(startPos.z, 1) / -delta.z;
	}

	double tDeltaX = fabs(1 / delta.x);
	double tDeltaY = fabs(1 / delta.y);
	double tDeltaZ = fabs(1 / delta.z);

	int failsafe = 0;
	bool bail = false;
	while (failsafe++<10000 && !bail) {
		byte dir = 0;
		if (tMaxX < tMaxY) {
			if (tMaxX < tMaxZ) {
				if (tMaxX > 1)
					return VoxelTraceRes();
				vx += stepX;
				tMaxX += tDeltaX;
				if (vx < 0 || vx >= config.dims_x)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= config.dims_z)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		else {
			if (tMaxY < tMaxZ) {
				if (tMaxY>1)
					return VoxelTraceRes();
				vy += stepY;
				tMaxY += tDeltaY;
				if (vy < 0 || vy >= config.dims_y)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= config.dims_z)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}

		if (dir == DIR_X_POS || dir == DIR_X_NEG) {
			double t = tMaxX - tDeltaX;
			double baseY = startPos.y + t*delta.y;
			double baseZ = startPos.z + t*delta.z;
			for (int iy = baseY - extents.y + epsilon; iy <= baseY + extents.y - epsilon; iy++) {
				for (int iz = baseZ + epsilon; iz <= baseZ + extents.z * 2 - epsilon; iz++) {
					BlockData vdata = get(vx, iy, iz);
					VoxelType& vt = config.voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_X_POS) {
							res.hitNormal.x = -1;
							res.hitPos.x -= epsilon;
						}
						else {
							res.hitNormal.x = 1;
							res.hitPos.x += epsilon;
						}
						return res;
					}
				}
			}
		}
		else if (dir == DIR_Y_POS || dir == DIR_Y_NEG) {
			double t = tMaxY - tDeltaY;
			double baseX = startPos.x + t*delta.x;
			double baseZ = startPos.z + t*delta.z;
			for (int ix = baseX - extents.x + epsilon; ix <= baseX + extents.x - epsilon; ix++) {
				for (int iz = baseZ + epsilon; iz <= baseZ + extents.z * 2 - epsilon; iz++) {
					BlockData vdata = get(ix, vy, iz);
					VoxelType& vt = config.voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_Y_POS) {
							res.hitNormal.y = -1;
							res.hitPos.y -= epsilon;
						}
						else {
							res.hitNormal.y = 1;
							res.hitPos.y += epsilon;
						}
						return res;
					}
				}
			}
		}
		else {
			double t = tMaxZ - tDeltaZ;
			double baseX = startPos.x + t*delta.x;
			double baseY = startPos.y + t*delta.y;
			for (int ix = baseX - extents.x + epsilon; ix <= baseX + extents.x - epsilon; ix++) {
				for (int iy = baseY - extents.y + epsilon; iy <= baseY + extents.y - epsilon; iy++) {
					BlockData vdata = get(ix, iy, vz);
					VoxelType& vt = config.voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_Z_POS) {
							res.hitNormal.z = -1;
							res.hitPos.z -= epsilon;
						}
						else {
							res.hitNormal.z = 1;
							res.hitPos.z += epsilon;
						}
						return res;
					}
				}
			}
		}

	}

	vox_print("[bail-hull] %i %i %i", vx, vy, vz);
	return VoxelTraceRes();
}

// Render every single chunk.
// TODO for huge worlds, only render close chunks
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

//floored division, credit http://www.microhowto.info/howto/round_towards_minus_infinity_when_dividing_integers_in_c_or_c++.html
int div_floor(int x, int y) {
	int q = x / y;
	int r = x%y;
	if ((r != 0) && ((r<0) != (y<0))) --q;
	return q;
}

// Gets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
BlockData VoxelWorld::get(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	int qx = x / VOXEL_CHUNK_SIZE;


	VoxelChunk* chunk = getChunk(div_floor(x, VOXEL_CHUNK_SIZE), div_floor(y, VOXEL_CHUNK_SIZE), div_floor(z, VOXEL_CHUNK_SIZE));
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE);
}

// Sets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
bool VoxelWorld::set(VoxelCoord x, VoxelCoord y, VoxelCoord z, BlockData d, bool flagChunks) {
	VoxelChunk* chunk = getChunk(div_floor(x, VOXEL_CHUNK_SIZE), div_floor(y, VOXEL_CHUNK_SIZE), div_floor(z, VOXEL_CHUNK_SIZE));
	if (chunk == nullptr || (!config.huge && (x >= config.dims_x || y >= config.dims_y || z >= config.dims_z)))
		return false;

	chunk->set(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE, d, flagChunks);
	return true;
}

void VoxelWorld::flagChunk(VoxelCoordXYZ chunk_pos, bool high_priority)
{
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
