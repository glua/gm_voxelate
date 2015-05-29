#include "vox_Voxels.h"

#include "vox_engine.h"

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
	if (chunks != nullptr) {
		for (int i = 0; i < config->dimX* config->dimY*config->dimZ; i++) {
			if (chunks[i] != nullptr) delete chunks[i];
		}
		delete[] chunks;
	}
	if (config)
		delete config;
}

uint16 Voxels::generate(int x, int y, int z) {
	if (z < 40) {
		return 5;
	}
	else if (z < 49) {
		return 8;
	}
	else if (z < 50) {
		return 1;
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

VoxelChunk* Voxels::addChunk(int chunk_num, const char* data_compressed, int data_len) {
	int x = chunk_num%config->dimX;
	int y = (chunk_num/config->dimX)%config->dimY;
	int z = (chunk_num /config->dimX/config->dimY)%config->dimZ;

	if (chunks[chunk_num] != nullptr)
		delete chunks[chunk_num];

	chunks[chunk_num] = new VoxelChunk(this,x,y,z,!data_compressed);
	if (data_compressed)
		fastlz_decompress(data_compressed, data_len, chunks[chunk_num]->voxel_data, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2);
	


	return chunks[chunk_num];
}

VoxelChunk* Voxels::getChunk(int x, int y, int z) {
	if (x < 0 || x >= config->dimX || y < 0 || y >= config->dimY || z < 0 || z >= config->dimZ) {
		return nullptr;
	}
	return chunks[x + y*config->dimX + z* config->dimX* config->dimY];
}

const int Voxels::getChunkData(int chunk_num,char* out) {
	if (chunk_num >= 0 && chunk_num < config->dimX*config->dimY*config->dimZ) {
		const char* input = reinterpret_cast<const char*>(chunks[chunk_num]->voxel_data);

		return fastlz_compress(input, VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE * 2, out);
	}
	return 0;
}

void Voxels::flagAllChunksForUpdate() {
	for (int i = 0; i < config->dimX*config->dimY*config->dimZ; i++) {
		if (chunks[i] != nullptr) {
			chunks_flagged_for_update.insert(chunks[i]);
		}
	}
}

void Voxels::initialize(VoxelConfig* config) {
	this->config = config;

	chunks = new VoxelChunk*[config->dimX*config->dimY*config->dimZ]();

	if (STATE_SERVER) {
		for (int i = 0; i < config->dimX*config->dimY*config->dimZ; i++) {
			addChunk(i, 0, 0);
		}
		flagAllChunksForUpdate();
	}
}

bool Voxels::isInitialized() {
	return chunks != nullptr;
}

Vector Voxels::getExtents() {
	double real_chunk_size = config->scale*VOXEL_CHUNK_SIZE;
	return Vector(
		config->dimX*real_chunk_size,
		config->dimY*real_chunk_size,
		config->dimZ*real_chunk_size
	);
}

void Voxels::doUpdates(int count, CBaseEntity* ent) {
	if (STATE_CLIENT || (config->sv_useMeshCollisions && ent!=nullptr)) {
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

VoxelTraceRes Voxels::doTraceHull(Vector startPos, Vector delta, Vector extents) {
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

VoxelTraceRes Voxels::iTrace(Vector startPos, Vector delta, Vector defNormal) {
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

VoxelTraceRes Voxels::iTraceHull(Vector startPos, Vector delta, Vector extents, Vector defNormal) {
	for (int ix = startPos.x - extents.x; ix <= startPos.x + extents.x; ix++) {
		for (int iy = startPos.y - extents.y; iy <= startPos.y + extents.y; iy++) {
			for (int iz = startPos.z; iz <= startPos.z + extents.z * 2; iz++) {
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
			for (int iy = baseY - extents.y; iy <= baseY + extents.y; iy++) {
				for (int iz = baseZ; iz <= baseZ + extents.z * 2; iz++) {
					uint16 vdata = get(vx, iy, iz);
					VoxelType& vt = config->voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t - tDeltaX*.001;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_X_POS)
							res.hitNormal.x = -1;
						else
							res.hitNormal.x = 1;
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
					VoxelType& vt = config->voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t - tDeltaY*.001;

						res.hitPos = startPos + res.fraction*delta;

						if (dir == DIR_Y_POS)
							res.hitNormal.y = -1;
						else
							res.hitNormal.y = 1;
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
					VoxelType& vt = config->voxelTypes[vdata];
					if (vt.form == VFORM_CUBE) {
						VoxelTraceRes res;
						res.fraction = t - tDeltaZ*.001;

						res.hitPos = startPos + res.fraction*delta;
						
						if (dir == DIR_Z_POS)
							res.hitNormal.z = -1;
						else
							res.hitNormal.z = 1;
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
	IMaterial* atlasMat = config->cl_atlasMaterial;
	if (atlasMat == nullptr) //shitty check to see if initialized
		return;

	CMatRenderContextPtr pRenderContext(iface_cl_materials);

	//Bind material
	pRenderContext->Bind(atlasMat);

	//Set lighting
	Vector4D lighting_cube[] = { Vector4D(.7, .7, .7, 1), Vector4D(.3, .3, .3, 1), Vector4D(.5, .5, .5, 1), Vector4D(.5, .5, .5, 1), Vector4D(1, 1, 1, 1), Vector4D(.1, .1, .1, 1) };
	pRenderContext->SetAmbientLightCube(lighting_cube);
	pRenderContext->SetAmbientLight(0, 0, 0);
	pRenderContext->DisableAllLocalLights();

	for (int i = 0; i < config->dimX* config->dimY*config->dimZ; i++) {
		if (chunks[i] != nullptr)
			chunks[i]->draw(pRenderContext);
	}
}

//floored division, credit http://www.microhowto.info/howto/round_towards_minus_infinity_when_dividing_integers_in_c_or_c++.html
int div_floor(int x, int y) {
	int q = x / y;
	int r = x%y;
	if ((r != 0) && ((r<0) != (y<0))) --q;
	return q;
}

uint16 Voxels::get(int x, int y, int z) {
	int qx = x / VOXEL_CHUNK_SIZE;
	
	
	VoxelChunk* chunk = getChunk(div_floor(x,VOXEL_CHUNK_SIZE), div_floor(y,VOXEL_CHUNK_SIZE), div_floor(z,VOXEL_CHUNK_SIZE));
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE);
}

bool Voxels::set(int x, int y, int z, uint16 d) {
	VoxelChunk* chunk = getChunk(div_floor(x, VOXEL_CHUNK_SIZE), div_floor(y, VOXEL_CHUNK_SIZE), div_floor(z, VOXEL_CHUNK_SIZE));
	if (chunk == nullptr)
		return false;

	chunk->set(x % VOXEL_CHUNK_SIZE, y % VOXEL_CHUNK_SIZE, z % VOXEL_CHUNK_SIZE, d);
	return true;
}

VoxelChunk::VoxelChunk(Voxels* sys,int cx, int cy, int cz, bool generate) {
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

	bool buildExterior;
	if (STATE_CLIENT)
		buildExterior = system->config->cl_drawExterior;

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
	double realX = (x + posX*VOXEL_CHUNK_SIZE) * system->config->scale;
	double realY = (y + posY*VOXEL_CHUNK_SIZE) * system->config->scale;
	double realZ = (z + posZ*VOXEL_CHUNK_SIZE) * system->config->scale;

	double realStep = system->config->scale;
	
	if (STATE_CLIENT) {
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

		iface_sv_collision->PolysoupAddTriangle(phys_soup, v1, v2, v3, 3);
		iface_sv_collision->PolysoupAddTriangle(phys_soup, v1, v3, v4, 3);
	}
}