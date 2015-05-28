#include "vox_Voxels.h"

#include "vox_engine.h"
#include "vox_util.h"

//#include "vox_lua_net.h"

#include <cmath>

#include <vector>

#define GAME_DLL
#include "cbase.h"
#undef GAME_DLL

#include "collisionutils.h"

#include "fastlz.h"

#define STD_VERT_FMT VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_VERTEX_SHADER | VERTEX_USERDATA_SIZE(4) | VERTEX_TEXCOORD_SIZE(0, 2)

#define BUILD_MAX_VERTS 8*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE

#define DIR_X_POS 1
#define DIR_Y_POS 2
#define DIR_Z_POS 4

#define DIR_X_NEG 8
#define DIR_Y_NEG 16
#define DIR_Z_NEG 32

std::vector<Voxels*> indexedVoxelRegistry;

int newIndexedVoxels(int index) {
	if (index == -1) {
		indexedVoxelRegistry.push_back(new Voxels());
		return indexedVoxelRegistry.size() - 1;
	}
	else {
		indexedVoxelRegistry.insert(indexedVoxelRegistry.begin() + index, new Voxels());
		return index;
	}
}

Voxels* getIndexedVoxels(int index) {
	try {
		return indexedVoxelRegistry.at(index);
	}
	catch (...) {
		return nullptr;
	}
}

void deleteIndexedVoxels(int index) {
	delete indexedVoxelRegistry.at(index);
	try {
		indexedVoxelRegistry.at(index) = nullptr;
	}
	catch (...) {}
}

void deleteAllIndexedVoxels() {
	for (Voxels* v : indexedVoxelRegistry) {
		if (v != nullptr) {
			delete v;
		}
	}
}

Voxels::~Voxels() {
	/*if (STATE_CLIENT) {
		vox_print("delete voxels - CL");
	}
	else {
		vox_print("delete voxels - SV");
	}*/

	if (chunks != nullptr) {
		for (int i = 0; i < _dimX* _dimY*_dimZ; i++) {
			if (chunks[i] != nullptr) delete chunks[i];
		}
		delete[] chunks;
	}
	if (cl_atlasMaterial != nullptr)
		cl_atlasMaterial->DecrementReferenceCount();
}

uint16 Voxels::generate(int x, int y, int z) {
	if (z < 49) {
		return 5;
	}
	else if (z < 50) {
		if (x % 2 == 0) {
			if (y % 2 == 0)
				return 3;
			else
				return 1;
		}
		else {
			if (y % 2 == 0)
				return 4;
			else
				return 2;
		}
	}
	else if (z == 50) {
		if (x > 315 && x<325 && y>316 && y<324) {
			if (x>316 && x < 324 && y>317 && y < 323) {
				return 6;
			}
			return 5;
		}
	}
	return 0;
}

void Voxels::pushConfig(lua_State* state) {
	LUA->ReferencePush(sv_reg_config);
}

void Voxels::deleteConfig(lua_State* state) {
	LUA->ReferenceFree(sv_reg_config);
}

VoxelChunk* Voxels::addChunk(int chunk_num, const char* data_compressed, int data_len) {
	int x = chunk_num%_dimX;
	int y = (chunk_num/_dimX)%_dimY;
	int z = (chunk_num /_dimX/_dimY)%_dimZ;

	if (chunks[chunk_num] != nullptr)
		delete chunks[chunk_num];

	chunks[chunk_num] = new VoxelChunk(this,x,y,z);
	fastlz_decompress(data_compressed, data_len, chunks[chunk_num]->voxel_data, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2);

	return chunks[chunk_num];
}

VoxelChunk* Voxels::getChunk(int x, int y, int z) {
	if (x < 0 || x >= _dimX || y < 0 || y >= _dimY || z < 0 || z >= _dimZ) {
		return nullptr;
	}
	return chunks[x + y*_dimX + z* _dimX* _dimY];
}

const int Voxels::getChunkData(int chunk_num,char* out) {
	if (chunk_num >= 0 && chunk_num < _dimX*_dimY*_dimZ) {
		const char* input = (const char*)(chunks[chunk_num]->voxel_data);

		return fastlz_compress(input, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2, out);
	}
	return 0;
}

void Voxels::flagAllChunksForUpdate() {
	for (int i = 0; i < _dimX*_dimY*_dimZ; i++) {
		if (chunks[i] != nullptr) {
			chunks_flagged_for_update.insert(chunks[i]);
		}
	}
}

bool Voxels::isInitialized() {
	return chunks != nullptr;
}

Vector Voxels::getExtents() {
	return Vector(
		_dimX*_scale*VOXEL_CHUNK_SIZE,
		_dimY*_scale*VOXEL_CHUNK_SIZE,
		_dimZ*_scale*VOXEL_CHUNK_SIZE
	);
}

void Voxels::doUpdates(int count, CBaseEntity* ent) {
	if (STATE_CLIENT || (sv_useMeshCollisions && ent!=nullptr)) {
		for (int i = 0; i < count; i++) {
			auto iter = chunks_flagged_for_update.begin();
			if (iter == chunks_flagged_for_update.end()) return;
			(*iter)->build(ent);
			chunks_flagged_for_update.erase(iter);
		}
	}
}

VoxelTraceRes Voxels::doTrace(Vector startPos, Vector delta) {
	Vector voxel_extents = getExtents();

	if (startPos.WithinAABox(Vector(0,0,0), voxel_extents)) {
		return iTrace(startPos/_scale , delta/_scale, Vector(0,0,0)) * _scale;
	}
	/*else {
		if (startX < 0 && deltaX>0) {
			double frac = -startX / deltaX;

			double iy = startY + frac*deltaY;
			double iz = startZ + frac*deltaZ;

			if (iy > 0 && iy < maxY && iz>0 && iz < maxZ) {				
				deltaX = (deltaX + startX) / _scale;
				deltaY = deltaY * (1-frac) / _scale;
				deltaZ = deltaZ * (1-frac) / _scale;

				startX = 0;
				startY = iy / _scale;
				startZ = iz / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, -1, 0, 0) * _scale;
			}
		}
		else if (startX > maxX && deltaX<0) {
			double frac = (startX-maxX) / -deltaX;

			double iy = startY + frac*deltaY;
			double iz = startZ + frac*deltaZ;

			if (iy > 0 && iy < maxY && iz>0 && iz < maxZ) {
				deltaX = (deltaX + startX - maxX) / _scale;
				deltaY = deltaY * (1 - frac) / _scale;
				deltaZ = deltaZ * (1 - frac) / _scale;

				startX = maxX / _scale;
				startY = iy / _scale;
				startZ = iz / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, 1, 0, 0) * _scale;
			}
		}
		if (startY < 0 && deltaY>0) {
			double frac = -startY / deltaY;

			double ix = startX + frac*deltaX;
			double iz = startZ + frac*deltaZ;

			if (ix > 0 && ix < maxX && iz>0 && iz < maxZ) {
				deltaY = (deltaY + startY) / _scale;
				deltaX = deltaX * (1 - frac) / _scale;
				deltaZ = deltaZ * (1 - frac) / _scale;

				startY = 0;
				startX = ix / _scale;
				startZ = iz / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, 0, -1, 0) * _scale;
			}
		}
		else if (startY > maxY && deltaY<0) {
			double frac = (startY - maxY) / -deltaY;

			double ix = startX + frac*deltaX;
			double iz = startZ + frac*deltaZ;

			if (ix > 0 && ix < maxY && iz>0 && iz < maxZ) {
				deltaY = (deltaY + startY - maxY) / _scale;
				deltaX = deltaX * (1 - frac) / _scale;
				deltaZ = deltaZ * (1 - frac) / _scale;

				startY = maxY / _scale;
				startX = ix / _scale;
				startZ = iz / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, 0, 1, 0) * _scale;
			}
		}
		if (startZ < 0 && deltaZ>0) {
			double frac = -startZ / deltaZ;

			double ix = startX + frac*deltaX;
			double iy = startY + frac*deltaY;

			if (ix > 0 && ix < maxX && iy>0 && iy < maxY) {
				deltaZ = (deltaZ + startZ) / _scale;
				deltaX = deltaX * (1 - frac) / _scale;
				deltaY = deltaY * (1 - frac) / _scale;

				startZ = 0;
				startX = ix / _scale;
				startY = iy / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, 0, 0, -1) * _scale;
			}
		}
		else if (startZ > maxZ && deltaZ<0) {
			double frac = (startZ - maxZ) / -deltaZ;

			double ix = startX + frac*deltaX;
			double iy = startY + frac*deltaY;

			if (ix > 0 && ix < maxX && iy>0 && iy < maxY) {
				deltaZ = (deltaZ + startZ - maxZ) / _scale;
				deltaX = deltaX * (1 - frac) / _scale;
				deltaY = deltaY * (1 - frac) / _scale;

				startZ = maxZ / _scale;
				startX = ix / _scale;
				startY = iy / _scale;

				return iTrace(startX, startY, startZ, deltaX, deltaY, deltaZ, 0, 0, 1) * _scale;
			}
		}
	}*/

	return VoxelTraceRes();
}

VoxelTraceRes Voxels::doTraceHull(Vector startPos, Vector delta, Vector extents) {
	Vector voxel_extents = getExtents();
	
	//Calculate our bounds based on the offsets used by the player hull. This will not work for everything, but will preserve player movement.
	Vector box_lower = startPos - Vector(extents.x, extents.y, 0);

	Vector box_upper = startPos + Vector(extents.x, extents.y, extents.z*2);

	if (IsBoxIntersectingBox(box_lower,box_upper,Vector(0,0,0),voxel_extents)) {
		return iTraceHull(startPos/_scale,delta/_scale,extents/_scale, Vector(0,0,0)) * _scale;
	}
	return VoxelTraceRes();
}

VoxelTraceRes Voxels::iTrace(Vector startPos, Vector delta, Vector defNormal) {
	int vx = startPos.x;
	int vy = startPos.y;
	int vz = startPos.z;

	uint16 vdata = get(vx, vy, vz);
	VoxelType& vt = voxelTypes[vdata];
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
				if (vx < 0 || vx >= VOXEL_CHUNK_SIZE*_dimX)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*_dimZ)
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
				if (vy < 0 || vy >= VOXEL_CHUNK_SIZE*_dimY)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*_dimZ)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		uint16 vdata = get(vx, vy, vz);
		VoxelType& vt = voxelTypes[vdata];
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

VoxelTraceRes Voxels::iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal) {
	for (int ix = startPos.x - extents.x; ix <= startPos.x + extents.x; ix++) {
		for (int iy = startPos.y - extents.y; iy <= startPos.y + extents.y; iy++) {
			for (int iz = startPos.z; iz <= startPos.z + extents.z * 2; iz++) {
				uint16 vdata = get(ix, iy, iz);
				VoxelType& vt = voxelTypes[vdata];
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
		vx = startPos.x + extents.x;
		stepX = 1;
		tMaxX = (1 - fmod(startPos.x + extents.x, 1)) / delta.x;
	}
	else {
		vx = startPos.x - extents.x;
		stepX = -1;
		tMaxX = fmod(startPos.x - extents.x, 1) / -delta.x;
	}

	if (delta.y >= 0) {
		vy = startPos.y + extents.y;
		stepY = 1;
		tMaxY = (1 - fmod(startPos.y + extents.y, 1)) / delta.y;
	}
	else {
		vy = startPos.y - extents.y;
		stepY = -1;
		tMaxY = fmod(startPos.y - extents.y, 1) / -delta.y;
	}

	if (delta.z >= 0) {
		vz = startPos.z + extents.z*2;
		stepZ = 1;
		tMaxZ = (1 - fmod(startPos.z + extents.z*2, 1)) / delta.z;
	}
	else {
		vz = startPos.z;
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
				if (vx < 0 || vx >= VOXEL_CHUNK_SIZE*_dimX)
					return VoxelTraceRes();
				dir = stepX > 0 ? DIR_X_POS : DIR_X_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*_dimZ)
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
				if (vy < 0 || vy >= VOXEL_CHUNK_SIZE*_dimY)
					return VoxelTraceRes();
				dir = stepY > 0 ? DIR_Y_POS : DIR_Y_NEG;
			}
			else {
				if (tMaxZ>1)
					return VoxelTraceRes();
				vz += stepZ;
				tMaxZ += tDeltaZ;
				if (vz < 0 || vz >= VOXEL_CHUNK_SIZE*_dimZ)
					return VoxelTraceRes();
				dir = stepZ > 0 ? DIR_Z_POS : DIR_Z_NEG;
			}
		}
		
		if (dir == DIR_X_POS || dir == DIR_X_NEG) {
			double t = tMaxX - tDeltaX;
			double baseY = startPos.y + t*delta.y;
			double baseZ = startPos.z + t*delta.z;
			for (int iy = baseY - extents.y; iy <= baseY + extents.y; iy++) {
				for (int iz = baseZ; iz <= baseZ + extents.z * 2; iz++) {
					uint16 vdata = get(vx, iy, iz);
					VoxelType& vt = voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_X_POS) {
							res.hitNormal.x = -1;
							res.hitPos.x -= .00002;
						}
						else {
							res.hitNormal.x = 1;
							res.hitPos.x += .00002;
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
			for (int ix = baseX - extents.x; ix <= baseX + extents.x; ix++) {
				for (int iz = baseZ; iz <= baseZ + extents.z * 2; iz++) {
					uint16 vdata = get(ix, vy, iz);
					VoxelType& vt = voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_Y_POS) {
							res.hitNormal.y = -1;
							res.hitPos.y -= .00002;
						}
						else {
							res.hitNormal.y = 1;
							res.hitNormal.y += .00002;
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
			for (int ix = baseX-extents.x; ix <= baseX+extents.x; ix++) {
				for (int iy = baseY-extents.y; iy <= baseY+extents.y; iy++) {
					uint16 vdata = get(ix, iy, vz);
					VoxelType& vt = voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t;

						res.hitPos = startPos + res.fraction*delta;
						
						if (dir == DIR_Z_POS) {
							res.hitNormal.z = -1;
							res.hitPos.z -= .00002;
						}
						else {
							res.hitNormal.z = 1;
							res.hitPos.z += .00002;
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

void Voxels::draw() {
	if (cl_atlasMaterial == nullptr)
		return;

	CMatRenderContextPtr pRenderContext(iface_cl_materials);

	//Bind material
	pRenderContext->Bind(cl_atlasMaterial);

	//Set lighting
	Vector4D lighting_cube[] = { Vector4D(.7, .7, .7, 1), Vector4D(.3, .3, .3, 1), Vector4D(.5, .5, .5, 1), Vector4D(.5, .5, .5, 1), Vector4D(1, 1, 1, 1), Vector4D(.1, .1, .1, 1) };
	pRenderContext->SetAmbientLightCube(lighting_cube);
	pRenderContext->SetAmbientLight(0, 0, 0);
	pRenderContext->DisableAllLocalLights();

	for (int i = 0; i < _dimX* _dimY*_dimZ; i++) {
		if (chunks[i] != nullptr)
			chunks[i]->draw(pRenderContext);
	}
}

uint16 Voxels::get(int x, int y, int z) {
	VoxelChunk* chunk = getChunk(x/VOXEL_CHUNK_SIZE, y/VOXEL_CHUNK_SIZE, z/VOXEL_CHUNK_SIZE);
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE);
}

bool Voxels::set(int x, int y, int z, uint16 d) {
	VoxelChunk* chunk = getChunk(x / VOXEL_CHUNK_SIZE, y / VOXEL_CHUNK_SIZE, z / VOXEL_CHUNK_SIZE);
	if (chunk == nullptr)
		return false;

	chunk->set(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE, d);
	return true;
}

VoxelChunk::VoxelChunk(Voxels* sys,int cx, int cy, int cz) {
	system = sys;
	posX = cx;
	posY = cy;
	posZ = cz;

	for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
		for (int y = 0; y < VOXEL_CHUNK_SIZE; y++) {
			for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
				voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = system->generate(x + VOXEL_CHUNK_SIZE*posX, y + VOXEL_CHUNK_SIZE*posY, z + VOXEL_CHUNK_SIZE*posZ);
			}
		}
	}
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

	bool drawExterior = system->cl_drawExterior;

	int lower_bound_x = (drawExterior && posX == 0) ? -1 : 0;
	int lower_bound_y = (drawExterior && posY == 0) ? -1 : 0;
	int lower_bound_z = (drawExterior && posZ == 0) ? -1 : 0;
	//int upper_bound_def = tmp_drawExterior ? VOXEL_CHUNK_SIZE : VOXEL_CHUNK_SIZE-1;

	//int upper_bound_x = next_chunk_x != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;
	//int upper_bound_y = next_chunk_y != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;
	//int upper_bound_z = next_chunk_z != nullptr ? VOXEL_CHUNK_SIZE : upper_bound_def;

	for (int x = lower_bound_x; x < VOXEL_CHUNK_SIZE; x++) {
		for (int y = lower_bound_y; y < VOXEL_CHUNK_SIZE; y++) {
			for (int z = lower_bound_z; z < VOXEL_CHUNK_SIZE; z++) {

				uint16 base;

				if (x == -1 || y == -1 || z == -1)
					base = 0;
				else
					base = get(x, y, z);

				VoxelType& base_type = system->voxelTypes[base];

				if ((drawExterior || (x != -1 && (x != VOXEL_CHUNK_SIZE - 1 || next_chunk_x != nullptr))) && y != -1 && z != -1) {
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

					VoxelType& offset_x_type = system->voxelTypes[offset_x];
					if (base_type.form == VFORM_CUBE && offset_x_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.atlasX, base_type.atlasY, DIR_X_POS);
					else if (base_type.form == VFORM_NULL && offset_x_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_x_type.atlasX, offset_x_type.atlasY, DIR_X_NEG);
				}

				if ((drawExterior || (y != -1 && (y != VOXEL_CHUNK_SIZE - 1 || next_chunk_y != nullptr))) && x != -1 && z != -1) {
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

					VoxelType& offset_y_type = system->voxelTypes[offset_y];
					if (base_type.form == VFORM_CUBE && offset_y_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.atlasX, base_type.atlasY, DIR_Y_POS);
					else if (base_type.form == VFORM_NULL && offset_y_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_y_type.atlasX, offset_y_type.atlasY, DIR_Y_NEG);
				}

				if ((drawExterior || (z != -1 && (z != VOXEL_CHUNK_SIZE - 1 || next_chunk_z != nullptr))) && x != -1 && y != -1) {
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

					VoxelType& offset_z_type = system->voxelTypes[offset_z];
					if (base_type.form == VFORM_CUBE && offset_z_type.form == VFORM_NULL)
						addFullVoxelFace(x, y, z, base_type.atlasX, base_type.atlasY, DIR_Z_POS);
					else if (base_type.form == VFORM_NULL && offset_z_type.form == VFORM_CUBE)
						addFullVoxelFace(x, y, z, offset_z_type.atlasX, offset_z_type.atlasY, DIR_Z_NEG);
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

uint16 VoxelChunk::get(int x, int y, int z) {
	return voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE];
}

void VoxelChunk::set(int x, int y, int z, uint16 d) {
	voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = d;

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
	if (STATE_CLIENT) {
		CMatRenderContextPtr pRenderContext(iface_cl_materials);

		while (meshes.begin() != meshes.end()) {
			pRenderContext->DestroyStaticMesh(*meshes.begin());
			meshes.erase(meshes.begin());
		}
	}
	else {
		if (phys_obj!=nullptr) {
			IPhysicsEnvironment* env = iface_sv_physics->GetActiveEnvironmentByIndex(0);
			phys_obj->SetGameData(nullptr);
			phys_obj->EnableCollisions(false);
			phys_obj->RecheckCollisionFilter();
			phys_obj->RecheckContactPoints();
			env->DestroyObject(phys_obj);
			phys_obj = nullptr;

			//Not sure if we should be calling this, but it may be required to prevent a leak.
			iface_sv_collision->DestroyCollide(phys_collider);
		}
	}
}

void VoxelChunk::meshStart() {
	if (STATE_CLIENT) {
		CMatRenderContextPtr pRenderContext(iface_cl_materials);
		current_mesh = pRenderContext->CreateStaticMesh(STD_VERT_FMT, "");

		//try MATERIAL_INSTANCED_QUADS
		meshBuilder.Begin(current_mesh, MATERIAL_QUADS, BUILD_MAX_VERTS / 4);

		verts_remaining = BUILD_MAX_VERTS;
	}
	else {
		phys_soup = iface_sv_collision->PolysoupCreate();
	}
}

void VoxelChunk::meshStop(CBaseEntity* ent) {
	if (STATE_CLIENT) {
		meshBuilder.End();

		if (verts_remaining == BUILD_MAX_VERTS) {
			CMatRenderContextPtr pRenderContext(iface_cl_materials);
			pRenderContext->DestroyStaticMesh(current_mesh);
		}
		else {
			meshes.push_back(current_mesh);
		}
	}
	else {
		phys_collider = iface_sv_collision->ConvertPolysoupToCollide(phys_soup, false); //todo what the fuck is MOPP?
		iface_sv_collision->PolysoupDestroy(phys_soup);

		objectparams_t op = { 0 };
		op.enableCollisions = true;
		op.pGameData = static_cast<void *>(ent);
		op.pName = "voxels";

		Vector pos = eent_getPos(ent);

		IPhysicsEnvironment* env = iface_sv_physics->GetActiveEnvironmentByIndex(0);
		phys_obj = env->CreatePolyObjectStatic(phys_collider, 3, pos, QAngle(0, 0, 0), &op);
	}
}

void VoxelChunk::addFullVoxelFace(int x, int y, int z, int tx, int ty, byte dir) {
	double realX = (x + posX*VOXEL_CHUNK_SIZE) * system->_scale;
	double realY = (y + posY*VOXEL_CHUNK_SIZE) * system->_scale;
	double realZ = (z + posZ*VOXEL_CHUNK_SIZE) * system->_scale;

	double realStep = system->_scale;
	
	if (STATE_CLIENT) {
		if (verts_remaining < 4) {
			meshStop(nullptr);
			meshStart();
		}
		verts_remaining -= 4;

		double uMin = ((double)tx / system->cl_atlasWidth) + system->cl_pixel_bias_x;
		double uMax = ((tx + 1.0) / system->cl_atlasWidth) - system->cl_pixel_bias_x;

		double vMin = ((double)ty / system->cl_atlasHeight) + system->cl_pixel_bias_y;
		double vMax = ((ty + 1.0) / system->cl_atlasHeight) - system->cl_pixel_bias_y;

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

		iface_sv_collision->PolysoupAddTriangle(phys_soup, v1, v2, v3, 3);
		iface_sv_collision->PolysoupAddTriangle(phys_soup, v1, v3, v4, 3);
	}
}