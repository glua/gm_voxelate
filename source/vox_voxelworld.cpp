#include "vox_voxelworld.h"

#include "vox_engine.h"

//#include "vox_lua_net.h"

#include <cmath>

#include <vector>

#include "collisionutils.h"

#include "fastlz.h"

#include "vox_network.h"
#include <bitbuf.h>

#define STD_VERT_FMT VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_VERTEX_SHADER | VERTEX_USERDATA_SIZE(4) | VERTEX_TEXCOORD_SIZE(0, 2)

#define BUILD_MAX_VERTS 8*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE

#define DIR_X_POS 1
#define DIR_Y_POS 2
#define DIR_Z_POS 4

#define DIR_X_NEG 8
#define DIR_Y_NEG 16
#define DIR_Z_NEG 32

std::vector<VoxelWorld*> indexedVoxelWorldRegistry;

int newIndexedVoxelWorld(int index) {
	if (index == -1) {
		indexedVoxelWorldRegistry.push_back(new VoxelWorld());
		return indexedVoxelWorldRegistry.size() - 1;
	}
	else {
		indexedVoxelWorldRegistry.insert(indexedVoxelWorldRegistry.begin() + index, new VoxelWorld());
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
	delete indexedVoxelWorldRegistry.at(index);
	try {
		indexedVoxelWorldRegistry.at(index) = nullptr;
	}
	catch (...) {}
}

void deleteAllIndexedVoxelWorlds() {
	for (VoxelWorld* v : indexedVoxelWorldRegistry) {
		if (v != nullptr) {
			delete v;
		}
	}
}

VoxelWorld::~VoxelWorld() {
	for (auto it : chunks_new) {
		if (it.second != nullptr) {
			delete it.second;
		}
	}

	chunks_new.clear();

	if (config)
		delete config;
}

VoxelChunk* VoxelWorld::addChunk(Coord x, Coord y, Coord z) {
	XYZCoordinate coord = { x, y, z };

	auto chunk = new VoxelChunk(this, x, y, z);
	chunks_new.insert({coord,chunk});

	return chunk;
}

VoxelChunk* VoxelWorld::getChunk(Coord x, Coord y, Coord z) {
	if (x < 0 || x >= config->dimX || y < 0 || y >= config->dimY || z < 0 || z >= config->dimZ) {
		return nullptr;
	}

	return chunks_new.at({ x, y, z });
}

const int VoxelWorld::getChunkData(Coord x, Coord y, Coord z,char* out) {
	if (x < 0 || x >= config->dimX || y < 0 || y >= config->dimY || z < 0 || z >= config->dimZ) {
		return 0;
	}

	const char* input = reinterpret_cast<const char*>(chunks_new[{x, y, z}]->voxel_data);

	return fastlz_compress(input, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2, out);
}

void VoxelWorld::setChunkData(Coord x, Coord y, Coord z, const char* data_compressed, int data_len) {
	if (x < 0 || x >= config->dimX || y < 0 || y >= config->dimY || z < 0 || z >= config->dimZ) {
		return;
	}

	fastlz_decompress(data_compressed, data_len, chunks_new[{x, y, z}]->voxel_data, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2);
	//chunks_flagged_for_update.insert(chunks_new[{x, y, z}]);
}

void VoxelWorld::initialize(VoxelConfig* config) {
	this->config = config;

	// YO 3D LOOP TIME NIGGA
	// TODO: remove this and only add chunks when entities are nearby
	// TODO: remove this entirely actually, this only generates positive int chunks, but we're going arbitrary...
	for (Coord x = 0; x < config->dimX; x++) {
		for (Coord y = 0; y < config->dimY; y++) {
			for (Coord z = 0; z < config->dimZ; z++) {
				addChunk(x, y, z);
			}
		}
	}

	initialised = true;
}

bool VoxelWorld::isInitialized() {
	return initialised;
}

Vector VoxelWorld::getExtents() {
	double real_chunk_size = config->scale*VOXEL_CHUNK_SIZE;
	return Vector(
		config->dimX*real_chunk_size,
		config->dimY*real_chunk_size,
		config->dimZ*real_chunk_size
	);
}

void VoxelWorld::getCellExtents(int& x, int &y, int &z) {
	x = config->dimX*VOXEL_CHUNK_SIZE;
	y = config->dimY*VOXEL_CHUNK_SIZE;
	z = config->dimZ*VOXEL_CHUNK_SIZE;
}

// Get real upset and flag every single chunk we know about for an update
// This happens right after we receive all the chunks.
// TODO prioritize based on distance from player
// TODO need different logic for huge worlds
void VoxelWorld::flagAllChunksForUpdate() {
	// calling this is probably a VERY VERY VERY VERY bad idea once we have an infinite map working
	for (auto it : chunks_new) {
		chunks_flagged_for_update.insert(it.second);
	}
}

bool VoxelWorld::sendChunksAround(int peerID, XYZCoordinate pos, Coord radius) {
	auto maxSize = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2 * radius * + 24;

	auto data = new(std::nothrow) uint8_t[maxSize];

	bf_write writer;

	writer.StartWriting(data, maxSize);

	if (data == NULL) {
		return false;
	}

	// write chunk num

	writer.WriteUBitLong(radius, 8);

	// write origin

	writer.WriteSBitLong(pos[0], 32);
	writer.WriteSBitLong(pos[1], 32);
	writer.WriteSBitLong(pos[2], 32);

	for (Coord x = 0; x < radius; x++) {
		for (Coord y = 0; y < radius; y++) {
			for (Coord z = 0; z < radius; z++) {
				char chunkData[VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2];
				int len = getChunkData(x, y, z, chunkData);

				writer.WriteUBitLong(len, 16);
				writer.WriteBytes(chunkData, len);
			}
		}
	}

	writer.WriteOneBit(0); // null terminate for good measure

	networking::channelSend(peerID, VOX_NETWORK_CHANNEL_CHUNKRADIUS_DATA, data, writer.GetNumBytesWritten());
}

// Updates up to n chunks
// Logic probably okay for huge worlds, although we may have to double check that the chunk still exists,
// or clean out chunks_flagged_for_update when we unload chunks
void VoxelWorld::doUpdates(int count, CBaseEntity* ent) {
	// On the server, we -NEED- the entity. Not so important on the client
	if (updates_enabled && (!IS_SERVERSIDE || ent != nullptr)) {
		for (int i = 0; i < count; i++) {
			auto iter = chunks_flagged_for_update.begin();
			if (iter == chunks_flagged_for_update.end()) return;
			(*iter)->build(ent);
			chunks_flagged_for_update.erase(iter);
		}
	}
}

// Enables updates -- Client doesn't want to update until it has all the chunks
// Going to need totally different logic for huge maps.
// Might not hurt to totally ditch this logic and use some other method to determine
// when chunks should start updating.
void VoxelWorld::enableUpdates(bool enable) {
	if (!IS_SERVERSIDE || (config->sv_useMeshCollisions))
		updates_enabled = enable;
}

// Function for line traces. Re-scales vectors and moves the start to the beggining of the voxel entity,
// Then calls fast trace function
VoxelTraceRes VoxelWorld::doTrace(Vector startPos, Vector delta) {
	Vector voxel_extents = getExtents();

	if (startPos.WithinAABox(Vector(0,0,0), voxel_extents)) {
		return iTrace(startPos/config->scale , delta/config->scale, Vector(0,0,0)) * config->scale;
	}
	else {
		Ray_t ray;
		ray.Init(startPos,startPos+delta);
		CBaseTrace tr;
		IntersectRayWithBox(ray, Vector(0, 0, 0), voxel_extents, 0, &tr);

		startPos = tr.endpos;
		delta *= 1-tr.fraction;

		return iTrace(startPos / config->scale, delta / config->scale, tr.plane.normal) * config->scale;
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
		return iTraceHull(startPos/config->scale,delta/config->scale,extents/config->scale, Vector(0,0,0)) * config->scale;
	}
	else {
		Ray_t ray;
		ray.Init(startPos, startPos + delta, box_lower, box_upper);
		CBaseTrace tr;
		IntersectRayWithBox(ray, Vector(0, 0, 0), voxel_extents, 0, &tr);

		startPos = tr.endpos;
		delta *= 1 - tr.fraction;

		return iTraceHull(startPos / config->scale, delta / config->scale, extents / config->scale, tr.plane.normal) * config->scale;
	}
}

// Fast trace function, based on http://www.cse.chalmers.se/edu/year/2011/course/TDA361/Advanced%20Computer%20Graphics/grid.pdf
VoxelTraceRes VoxelWorld::iTrace(Vector startPos, Vector delta, Vector defNormal) {
	int vx = startPos.x;
	int vy = startPos.y;
	int vz = startPos.z;

	uint16 vdata = get(vx, vy, vz);
	VoxelType& vt = config->voxelTypes[vdata];
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
				if (vx < 0 || vx >= VOXEL_CHUNK_SIZE*config->dimX)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*config->dimZ)
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
				if (vy < 0 || vy >= VOXEL_CHUNK_SIZE*config->dimY)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*config->dimZ)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		uint16 vdata = get(vx, vy, vz);
		VoxelType& vt = config->voxelTypes[vdata];
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
				uint16 vdata = get(ix, iy, iz);
				VoxelType& vt = config->voxelTypes[vdata];
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
				if (vx < 0 || vx >= VOXEL_CHUNK_SIZE*config->dimX)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*config->dimZ)
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
				if (vy < 0 || vy >= VOXEL_CHUNK_SIZE*config->dimY)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*config->dimZ)
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
					uint16 vdata = get(vx, iy, iz);
					VoxelType& vt = config->voxelTypes[vdata];
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
					uint16 vdata = get(ix, vy, iz);
					VoxelType& vt = config->voxelTypes[vdata];
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
					uint16 vdata = get(ix, iy, vz);
					VoxelType& vt = config->voxelTypes[vdata];
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
	IMaterial* atlasMat = config->cl_atlasMaterial;
	if (!isInitialized())
		return;

	CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);

	//Bind material
	pRenderContext->Bind(atlasMat);

	//Set lighting
	Vector4D lighting_cube[] = { Vector4D(.7, .7, .7, 1), Vector4D(.3, .3, .3, 1), Vector4D(.5, .5, .5, 1), Vector4D(.5, .5, .5, 1), Vector4D(1, 1, 1, 1), Vector4D(.1, .1, .1, 1) };
	pRenderContext->SetAmbientLightCube(lighting_cube);
	pRenderContext->SetAmbientLight(0, 0, 0);
	pRenderContext->DisableAllLocalLights();

	// TODO: only draw nearby chunks
	for (auto it : chunks_new) {
		it.second->draw(pRenderContext);
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
uint16 VoxelWorld::get(Coord x, Coord y, Coord z) {
	int qx = x / VOXEL_CHUNK_SIZE;


	VoxelChunk* chunk = getChunk(div_floor(x,VOXEL_CHUNK_SIZE), div_floor(y,VOXEL_CHUNK_SIZE), div_floor(z,VOXEL_CHUNK_SIZE));
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE);
}

// Sets a voxel given VOXEL COORDINATES -- NOT WORLD COORDINATES OR COORDINATES LOCAL TO ENT -- THOSE ARE HANDLED BY LUA CHUNK
bool VoxelWorld::set(Coord x, Coord y, Coord z, uint16 d, bool flagChunks) {
	VoxelChunk* chunk = getChunk(div_floor(x, VOXEL_CHUNK_SIZE), div_floor(y, VOXEL_CHUNK_SIZE), div_floor(z, VOXEL_CHUNK_SIZE));
	if (chunk == nullptr)
		return false;

	chunk->set(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE, d, flagChunks);
	return true;
}

// Most shit inside chunks should just work with huge maps
// The one thing that comes to mind is mesh generation, which always builds the chunk offset into the mesh
// I beleive I did this so I wouldn't need to push a matrix for every single chunk (lots of chunks, could be expensive?)
// Could run into FP issues if we keep the offset built in though, as I'm pretty sure meshes use 32bit floats
VoxelChunk::VoxelChunk(VoxelWorld* sys,int cx, int cy, int cz) {
	system = sys;
	posX = cx;
	posY = cy;
	posZ = cz;
}

VoxelChunk::~VoxelChunk() {
	meshClearAll();
}

void VoxelChunk::build(CBaseEntity* ent) {

	meshClearAll();

	meshStart();

	VoxelChunk* next_chunk_x = system->getChunk(posX + 1, posY, posZ);
	VoxelChunk* next_chunk_y = system->getChunk(posX, posY + 1, posZ);
	VoxelChunk* next_chunk_z = system->getChunk(posX, posY, posZ + 1);

	//bool buildExterior; TODO see if this broke anything
	//if (STATE_CLIENT)
	bool buildExterior = system->config->cl_drawExterior;

	int lower_bound_x = (buildExterior && posX == 0) ? -1 : 0;
	int lower_bound_y = (buildExterior && posY == 0) ? -1 : 0;
	int lower_bound_z = (buildExterior && posZ == 0) ? -1 : 0;
	//int upper_bound_def = tmp_drawExterior ? VOXEL_CHUNK_SIZE : VOXEL_CHUNK_SIZE-1;

	//int upper_bound_x = next_chunk_x != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;
	//int upper_bound_y = next_chunk_y != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;
	//int upper_bound_z = next_chunk_z != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;

	VoxelType* blockTypes = system->config->voxelTypes;

	for (int x = lower_bound_x; x < VOXEL_CHUNK_SIZE; x++) {
		for (int y = lower_bound_y; y < VOXEL_CHUNK_SIZE; y++) {
			for (int z = lower_bound_z; z < VOXEL_CHUNK_SIZE; z++) {

				uint16 base;

				if (x == -1 || y == -1 || z == -1)
					base = 0;
				else
					base = get(x, y, z);

				VoxelType& base_type = blockTypes[base];

				if ((buildExterior || (x != -1 && (x != VOXEL_CHUNK_SIZE - 1 || next_chunk_x != nullptr))) && y != -1 && z != -1) {
					uint16 offset_x;
					if (x == VOXEL_CHUNK_SIZE - 1)
						if (next_chunk_x == nullptr)
							offset_x = 0;
						else
							offset_x = next_chunk_x->get(0, y, z);
					//else if (x == -1)
					//	offset_x = 0;
					else
						offset_x = get(x + 1, y, z);

					VoxelType& offset_x_type = blockTypes[offset_x];
					if (base_type.form == VFORM_CUBE && offset_x_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.side_xPos.x, base_type.side_xPos.y, DIR_X_POS);
					else if (base_type.form == VFORM_NULL && offset_x_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_x_type.side_xNeg.x, offset_x_type.side_xNeg.y, DIR_X_NEG);
				}

				if ((buildExterior || (y != -1 && (y != VOXEL_CHUNK_SIZE - 1 || next_chunk_y != nullptr))) && x != -1 && z != -1) {
					uint16 offset_y;
					if (y == VOXEL_CHUNK_SIZE - 1)
						if (next_chunk_y == nullptr)
							offset_y = 0;
						else
							offset_y = next_chunk_y->get(x, 0, z);
					//else if (y == -1)
					//	offset_y = 0;
					else
						offset_y = get(x, y + 1, z);

					VoxelType& offset_y_type = blockTypes[offset_y];
					if (base_type.form == VFORM_CUBE && offset_y_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.side_yPos.x, base_type.side_yPos.y, DIR_Y_POS);
					else if (base_type.form == VFORM_NULL && offset_y_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_y_type.side_yNeg.x, offset_y_type.side_yNeg.y, DIR_Y_NEG);
				}

				if ((buildExterior || (z != -1 && (z != VOXEL_CHUNK_SIZE - 1 || next_chunk_z != nullptr))) && x != -1 && y != -1) {
					uint16 offset_z;
					if (z == VOXEL_CHUNK_SIZE - 1)
						if (next_chunk_z == nullptr)
							offset_z = 0;
						else
							offset_z = next_chunk_z->get(x, y, 0);
					//else if (z == -1)
					//	offset_z = 0;
					else
						offset_z = get(x, y, z + 1);

					VoxelType& offset_z_type = blockTypes[offset_z];
					if (base_type.form == VFORM_CUBE && offset_z_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.side_zPos.x, base_type.side_zPos.y, DIR_Z_POS);
					else if (base_type.form == VFORM_NULL && offset_z_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_z_type.side_zNeg.x, offset_z_type.side_zNeg.y, DIR_Z_NEG);
				}
			}
		}
	}

	//final build
	meshStop(ent);

}

void VoxelChunk::draw(CMatRenderContextPtr& pRenderContext) {
	for (IMesh* m : meshes) {
		m->Draw();
	}
}

uint16 VoxelChunk::get(Coord x, Coord y, Coord z) {
	return voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE];
}

void VoxelChunk::set(Coord x, Coord y, Coord z, uint16 d, bool flagChunks) {
	voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = d;

	if (!flagChunks)
		return;

	system->chunks_flagged_for_update.insert(this);

	if (x == 0) {
		VoxelChunk* c = system->getChunk(posX - 1, posY, posZ);
		if (c)
			system->chunks_flagged_for_update.insert(c);
	}

	if (y == 0) {
		VoxelChunk* c = system->getChunk(posX, posY - 1, posZ);
		if (c)
			system->chunks_flagged_for_update.insert(c);
	}

	if (z == 0) {
		VoxelChunk* c = system->getChunk(posX, posY, posZ - 1);
		if (c)
			system->chunks_flagged_for_update.insert(c);
	}
}
/*
void VoxelChunk::send(int sys_index, int ply_id, bool init, int chunk_num) {
	net_sv_sendChunk(sys_index, ply_id, init, chunk_num , voxel_data, (VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE)*2);
}
*/
void VoxelChunk::meshClearAll() {
	if (!IS_SERVERSIDE) {
		CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);

		while (meshes.begin() != meshes.end()) {
			pRenderContext->DestroyStaticMesh(*meshes.begin());
			meshes.erase(meshes.begin());
		}
	}
	else {
		if (phys_obj!=nullptr) {
			IPhysicsEnvironment* env = IFACE_SV_PHYSICS->GetActiveEnvironmentByIndex(0);
			phys_obj->SetGameData(nullptr);
			phys_obj->EnableCollisions(false);
			phys_obj->RecheckCollisionFilter();
			phys_obj->RecheckContactPoints();
			env->DestroyObject(phys_obj);
			phys_obj = nullptr;

			//Not sure if we should be calling this, but it may be required to prevent a leak.
			IFACE_SV_COLLISION->DestroyCollide(phys_collider);
		}
	}
}

void VoxelChunk::meshStart() {
	if (!IS_SERVERSIDE) {
		CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);
		current_mesh = pRenderContext->CreateStaticMesh(STD_VERT_FMT, "");

		//try MATERIAL_INSTANCED_QUADS
		meshBuilder.Begin(current_mesh, MATERIAL_QUADS, BUILD_MAX_VERTS / 4);

		verts_remaining = BUILD_MAX_VERTS;
	}
	else {
		phys_soup = IFACE_SV_COLLISION->PolysoupCreate();
	}
}

void VoxelChunk::meshStop(CBaseEntity* ent) {
	if (!IS_SERVERSIDE) {
		meshBuilder.End();

		if (verts_remaining == BUILD_MAX_VERTS) {
			CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);
			pRenderContext->DestroyStaticMesh(current_mesh);
		}
		else {
			meshes.push_back(current_mesh);
		}
	}
	else {
		phys_collider = IFACE_SV_COLLISION->ConvertPolysoupToCollide(phys_soup, false); //todo what the fuck is MOPP?
		IFACE_SV_COLLISION->PolysoupDestroy(phys_soup);

		objectparams_t op = { 0 };
		op.enableCollisions = true;
		op.pGameData = static_cast<void *>(ent);
		op.pName = "voxels";

		Vector pos = eent_getPos(ent);

		IPhysicsEnvironment* env = IFACE_SV_PHYSICS->GetActiveEnvironmentByIndex(0);
		phys_obj = env->CreatePolyObjectStatic(phys_collider, 3, pos, QAngle(0, 0, 0), &op);
	}
}

void VoxelChunk::addFullVoxelFace(Coord x, Coord y, Coord z, int tx, int ty, byte dir) {
	double realX = (x + posX*VOXEL_CHUNK_SIZE) * system->config->scale;
	double realY = (y + posY*VOXEL_CHUNK_SIZE) * system->config->scale;
	double realZ = (z + posZ*VOXEL_CHUNK_SIZE) * system->config->scale;

	double realStep = system->config->scale;

	if (!IS_SERVERSIDE) {
		if (verts_remaining < 4) {
			meshStop(nullptr);
			meshStart();
		}
		verts_remaining -= 4;

		VoxelConfig* cl_config = system->config;

		double uMin = ((double)tx / cl_config->cl_atlasWidth) + cl_config->cl_pixel_bias_x;
		double uMax = ((tx + 1.0) / cl_config->cl_atlasWidth) - cl_config->cl_pixel_bias_x;

		double vMin = ((double)ty / cl_config->cl_atlasHeight) + cl_config->cl_pixel_bias_y;
		double vMax = ((ty + 1.0) / cl_config->cl_atlasHeight) - cl_config->cl_pixel_bias_y;

		if (dir == DIR_X_POS) {
			meshBuilder.Position3f(realX + realStep, realY, realZ);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(1, 0, 0);
			meshBuilder.AdvanceVertex();
		}
		else if (dir == DIR_X_NEG) {
			meshBuilder.Position3f(realX + realStep, realY, realZ);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(-1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(-1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(-1, 0, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(-1, 0, 0);
			meshBuilder.AdvanceVertex();
		}
		else if (dir == DIR_Y_POS) {
			meshBuilder.Position3f(realX, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(0, 1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(0, 1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(0, 1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(0, 1, 0);
			meshBuilder.AdvanceVertex();
		}
		else if (dir == DIR_Y_NEG) {
			meshBuilder.Position3f(realX, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(0, -1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(0, -1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(0, -1, 0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(0, -1, 0);
			meshBuilder.AdvanceVertex();
		}
		else if (dir == DIR_Z_POS) {
			meshBuilder.Position3f(realX, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(0, 0, 1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(0, 0, 1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(0, 0, 1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(0, 0, 1);
			meshBuilder.AdvanceVertex();
		}
		else if (dir == DIR_Z_NEG) {
			meshBuilder.Position3f(realX, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMax);
			meshBuilder.Normal3f(0, 0, -1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMax);
			meshBuilder.Normal3f(0, 0, -1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX + realStep, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMax, vMin);
			meshBuilder.Normal3f(0, 0, -1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f(realX, realY + realStep, realZ + realStep);
			meshBuilder.TexCoord2f(0, uMin, vMin);
			meshBuilder.Normal3f(0, 0, -1);
			meshBuilder.AdvanceVertex();
		}
	}
	else {
		Vector v1, v2, v3, v4;
		if (dir == DIR_X_POS) {
			v1 = Vector(realX + realStep, realY, realZ);
			v2 = Vector(realX + realStep, realY, realZ + realStep);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX + realStep, realY + realStep, realZ);
		}
		else if (dir == DIR_X_NEG) {
			v1 = Vector(realX + realStep, realY, realZ);
			v2 = Vector(realX + realStep, realY + realStep, realZ);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX + realStep, realY, realZ + realStep);
		}
		else if (dir == DIR_Y_POS) {
			v1 = Vector(realX, realY + realStep, realZ);
			v2 = Vector(realX + realStep, realY + realStep, realZ);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX, realY + realStep, realZ + realStep);
		}
		else if (dir == DIR_Y_NEG) {
			v1 = Vector(realX, realY + realStep, realZ);
			v2 = Vector(realX, realY + realStep, realZ + realStep);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX + realStep, realY + realStep, realZ);
		}
		else if (dir == DIR_Z_POS) {
			v1 = Vector(realX, realY, realZ + realStep);
			v2 = Vector(realX, realY + realStep, realZ + realStep);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX + realStep, realY, realZ + realStep);
		}
		else if (dir == DIR_Z_NEG) {
			v1 = Vector(realX, realY, realZ + realStep);
			v2 = Vector(realX + realStep, realY, realZ + realStep);
			v3 = Vector(realX + realStep, realY + realStep, realZ + realStep);
			v4 = Vector(realX, realY + realStep, realZ + realStep);
		}

		IFACE_SV_COLLISION->PolysoupAddTriangle(phys_soup, v1, v2, v3, 3);
		IFACE_SV_COLLISION->PolysoupAddTriangle(phys_soup, v1, v3, v4, 3);
	}
}