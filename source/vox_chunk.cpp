#include "vox_chunk.h"
#include "vox_voxelworld.h"

// Most shit inside chunks should just work with huge maps
// The one thing that comes to mind is mesh generation, which always builds the chunk offset into the mesh
// I beleive I did this so I wouldn't need to push a matrix for every single chunk (lots of chunks, could be expensive?)
// Could run into FP issues if we keep the offset built in though, as I'm pretty sure meshes use 32bit floats
VoxelChunk::VoxelChunk(VoxelWorld* sys,int cx, int cy, int cz) {
	world = sys;
	posX = cx;
	posY = cy;
	posZ = cz;
}

VoxelChunk::~VoxelChunk() {
	physicsMeshClearAll();
#ifdef VOXELATE_CLIENT
	graphicsMeshClearAll();
#endif
}

// todo allow any function to be used for generation, based on config
void VoxelChunk::generate() {
	int offset_x = posX * VOXEL_CHUNK_SIZE;
	int offset_y = posY * VOXEL_CHUNK_SIZE;
	int offset_z = posZ * VOXEL_CHUNK_SIZE;

	static const int max_x = VOXEL_CHUNK_SIZE;
	static const int max_y = VOXEL_CHUNK_SIZE;
	static const int max_z = VOXEL_CHUNK_SIZE;

	for (int x = 0; x < max_x; x++) {
		for (int y = 0; y < max_y; y++) {
			for (int z = 0; z < max_z; z++) {
				auto block = vox_worldgen_basic(offset_x+x, offset_y+y, offset_z+z);
				set(x, y, z, block, false);
			}
		}
	}
}

void VoxelChunk::build(CBaseEntity* ent, ELevelOfDetail lod) {
	physicsMeshClearAll();
#ifdef VOXELATE_CLIENT
	graphicsMeshClearAll();
#endif

	VoxelChunk* next_chunk_x = world->getChunk(posX + 1, posY, posZ);
	VoxelChunk* next_chunk_y = world->getChunk(posX, posY + 1, posZ);
	VoxelChunk* next_chunk_z = world->getChunk(posX, posY, posZ + 1);

	// Hoo boy. Get ready to see some shit.

	static const int upper_bound_x = VOXEL_CHUNK_SIZE;
	static const int upper_bound_y = VOXEL_CHUNK_SIZE;
	static const int upper_bound_z = VOXEL_CHUNK_SIZE;

	static const int lower_slice_x = 0;
	static const int lower_slice_y = 0;
	static const int lower_slice_z = 0;

	static int upper_slice_x = VOXEL_CHUNK_SIZE;
	static int upper_slice_y = VOXEL_CHUNK_SIZE;
	static int upper_slice_z = VOXEL_CHUNK_SIZE;

	VoxelType* blockTypes = world->config.voxelTypes;

	SliceFace faces[VOXEL_CHUNK_SIZE][VOXEL_CHUNK_SIZE];

	int scaling_size = 1;

	int adjusted_upper_bound_x = upper_bound_x;
	int adjusted_upper_bound_y = upper_bound_y;
	int adjusted_upper_bound_z = upper_bound_z;

	switch (lod) {
	case ELevelOfDetail::FULL:
		break;
	case ELevelOfDetail::TWO_MERGE:
		scaling_size = 2;
		adjusted_upper_bound_x = div_floor(adjusted_upper_bound_x, scaling_size);
		adjusted_upper_bound_y = div_floor(adjusted_upper_bound_y, scaling_size);
		adjusted_upper_bound_z = div_floor(adjusted_upper_bound_z, scaling_size);
		break;
	case ELevelOfDetail::FOUR_MERGE:
		scaling_size = 4;
		adjusted_upper_bound_x = div_floor(adjusted_upper_bound_x, scaling_size);
		adjusted_upper_bound_y = div_floor(adjusted_upper_bound_y, scaling_size);
		adjusted_upper_bound_z = div_floor(adjusted_upper_bound_z, scaling_size);
		break;
	case ELevelOfDetail::EIGHT_MERGE:
		scaling_size = 8;
		adjusted_upper_bound_x = div_floor(adjusted_upper_bound_x, scaling_size);
		adjusted_upper_bound_y = div_floor(adjusted_upper_bound_y, scaling_size);
		adjusted_upper_bound_z = div_floor(adjusted_upper_bound_z, scaling_size);
		break;
	default:
		scaling_size = VOXEL_CHUNK_SIZE / 2;
	}

	// Slices along x axis!
	for (int slice_x = lower_slice_x; slice_x < upper_slice_x; slice_x += scaling_size) {
		int adjusted_x = (slice_x - lower_slice_x) / scaling_size;
		for (int z = 0; z < upper_bound_z; z += scaling_size) {
			int adjusted_z = z / scaling_size;
			for (int y = 0; y < upper_bound_y; y += scaling_size) {
				int adjusted_y = y / scaling_size;

				// Compute base type
				BlockData base;

				if (slice_x < 0)
					base = 0;
				else
					base = get(slice_x, y, z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_x;
				if (slice_x == (VOXEL_CHUNK_SIZE - scaling_size))
					if (next_chunk_x == nullptr)
						offset_x = 0;
					else
						offset_x = next_chunk_x->get(0, y, z);
				else
					offset_x = get(slice_x + scaling_size, y, z);

				VoxelType& offset_x_type = blockTypes[offset_x];

				// Add faces!
				SliceFace& face = faces[adjusted_z][adjusted_y];

				if (base_type.form == VFORM_CUBE && offset_x_type.form == VFORM_NULL) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = true;
					face.texture = base_type.side_xPos;
#endif
				}
				else if (base_type.form == VFORM_NULL && offset_x_type.form == VFORM_CUBE) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = false;
					face.texture = offset_x_type.side_xNeg;
#endif
				}
				else
					face.present = false;
			}
		}

		buildSlice(adjusted_x, DIR_X_POS, faces, adjusted_upper_bound_y, adjusted_upper_bound_z, scaling_size);
	}

	// Slices along y axis!
	for (int slice_y = lower_slice_y; slice_y < upper_slice_y; slice_y += scaling_size) {
		int adjusted_y = (slice_y - lower_slice_y) / scaling_size;
		for (int z = 0; z < upper_bound_z; z += scaling_size) {
			int adjusted_z = z / scaling_size;
			for (int x = 0; x < upper_bound_x; x += scaling_size) {
				int adjusted_x = x / scaling_size;

				// Compute base type
				BlockData base;

				if (slice_y < 0)
					base = 0;
				else
					base = get(x, slice_y, z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_y;
				if (slice_y == (VOXEL_CHUNK_SIZE - scaling_size))
					if (next_chunk_y == nullptr)
						offset_y = 0;
					else
						offset_y = next_chunk_y->get(x, 0, z);
				else
					offset_y = get(x, slice_y + scaling_size, z);

				VoxelType& offset_y_type = blockTypes[offset_y];

				// Add faces!
				SliceFace& face = faces[adjusted_z][adjusted_x];

				if (base_type.form == VFORM_CUBE && offset_y_type.form == VFORM_NULL) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = true;
					face.texture = base_type.side_yPos;
#endif
				}
				else if (base_type.form == VFORM_NULL && offset_y_type.form == VFORM_CUBE) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = false;
					face.texture = offset_y_type.side_yNeg;
#endif
				}
				else
					face.present = false;
			}
		}

		buildSlice(adjusted_y, DIR_Y_POS, faces, adjusted_upper_bound_x, adjusted_upper_bound_z, scaling_size);
	}

	// Slices along z axis! TODO ALSO PROCESS NON-CUBIC BLOCKS IN -THIS- STAGE

	for (int slice_z = lower_slice_z; slice_z < upper_slice_z; slice_z += scaling_size) {
		int adjusted_z = (slice_z - lower_slice_z) / scaling_size;
		for (int y = 0; y < upper_bound_y; y += scaling_size) {
			int adjusted_y = y / scaling_size;
			for (int x = 0; x < upper_bound_x; x += scaling_size) {
				int adjusted_x = x / scaling_size;

				// Compute base type
				BlockData base;

				if (slice_z < 0)
					base = 0;
				else
					base = get(x, y, slice_z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_z;
				if (slice_z == (VOXEL_CHUNK_SIZE - scaling_size))
					if (next_chunk_z == nullptr)
						offset_z = 0;
					else
						offset_z = next_chunk_z->get(x, y, 0);
				else
					offset_z = get(x, y, slice_z + scaling_size);

				VoxelType& offset_z_type = blockTypes[offset_z];

				// Add faces!
				SliceFace& face = faces[adjusted_y][adjusted_x];

				if (base_type.form == VFORM_CUBE && offset_z_type.form == VFORM_NULL) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = true;
					face.texture = base_type.side_zPos;
#endif
				}
				else if (base_type.form == VFORM_NULL && offset_z_type.form == VFORM_CUBE) {
					face.present = true;
#ifdef VOXELATE_CLIENT
					face.direction = false;
					face.texture = offset_z_type.side_zNeg;
#endif
				} else
					face.present = false;
			}
		}

		buildSlice(adjusted_z, DIR_Z_POS, faces, adjusted_upper_bound_x, adjusted_upper_bound_y, scaling_size);
	}

	//final build
	physicsMeshStop(ent);
#ifdef VOXELATE_CLIENT
	graphicsMeshStop();
#endif
}

void VoxelChunk::buildSlice(int slice, byte dir, SliceFace faces[VOXEL_CHUNK_SIZE][VOXEL_CHUNK_SIZE], int upper_bound_x, int upper_bound_y, int scaling_size) {
	for (int y = 0; y < upper_bound_y; y++) {
		for (int x = 0; x < upper_bound_x; x++) {
			if (faces[y][x].present) {
				SliceFace& current_face = faces[y][x];

				int end_x;
				int end_y;

				for (end_x = x + 1; end_x < upper_bound_x && current_face == faces[y][end_x]; end_x++) {
					faces[y][end_x].present = false;
				}

				if (end_x >= upper_bound_x) {
					end_x = upper_bound_x;
				}


				for (end_y = y + 1; end_y < upper_bound_y; end_y++) {
					for (int ix = x; ix < end_x; ix++) {
						if (!(current_face == faces[end_y][ix]))
							goto bail;
					}

					for (int ix = x; ix < end_x; ix++) {
						faces[end_y][ix].present = false;
					}
				}

				if (end_y >= upper_bound_y) {
					end_y = upper_bound_y;
				}
			bail:

				current_face.present = false;

				int w = end_x - x;
				int h = end_y - y;

#ifdef VOXELATE_CLIENT
				addSliceFace(
					slice * scaling_size, 
					x * scaling_size, 
					y * scaling_size, 
					w * scaling_size,
					h * scaling_size,
					current_face.texture.x, 
					current_face.texture.y, 
					current_face.direction ? dir : dir + 3, 
					scaling_size
				);
#else
				addSliceFace(
					slice * scaling_size, 
					x * scaling_size, 
					y * scaling_size, 
					w * scaling_size,
					h * scaling_size,
					0, 
					0, 
					dir,
					scaling_size
				);
#endif
			}
		}
	}
}

extern lua_State* lastState;

#define push_LVEC(x,y,z) \
	lua_getglobal(lastState, "Vector"); \
	lua_pushnumber(lastState, x); \
	lua_pushnumber(lastState, y); \
	lua_pushnumber(lastState, z); \
	lua_call(lastState, 3, 1)

#define push_LANG(x,y,z) \
	lua_getglobal(lastState, "Angle"); \
	lua_pushnumber(lastState, x); \
	lua_pushnumber(lastState, y); \
	lua_pushnumber(lastState, z); \
	lua_call(lastState, 3, 1)

#define push_LCOL(x,y,z) \
	lua_getglobal(lastState, "Color"); \
	lua_pushnumber(lastState, x); \
	lua_pushnumber(lastState, y); \
	lua_pushnumber(lastState, z); \
	lua_call(lastState, 3, 1)

void VoxelChunk::draw(CMatRenderContextPtr& pRenderContext) {
	/*
	lua_getglobal(lastState, "render");
	lua_getfield(lastState, -1, "DrawWireframeBox");

	auto center = getWorldCoords();
	push_LVEC(center[0], center[1], center[2]);
	push_LANG(0, 0, 0);
	push_LVEC(0, 0, 0);
	push_LVEC(VOXEL_CHUNK_SIZE, VOXEL_CHUNK_SIZE, VOXEL_CHUNK_SIZE);
	push_LCOL(255, 0, 0);
	lua_pushboolean(lastState, false);

	lua_call(lastState, 6, 0);
	lua_pop(lastState, 1);
	*/

	if (meshes.begin() != meshes.end())
		for (IMesh* m : meshes)
			m->Draw();
}

VoxelCoordXYZ VoxelChunk::getWorldCoords() {
	return{ posX*VOXEL_CHUNK_SIZE, posY*VOXEL_CHUNK_SIZE, posZ*VOXEL_CHUNK_SIZE };
}

BlockData VoxelChunk::get(VoxelCoord x, VoxelCoord y, VoxelCoord z) {
	return voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE];
}

void VoxelChunk::set(VoxelCoord x, VoxelCoord y, VoxelCoord z, BlockData d, bool flagChunks) {
	voxel_data[x + y*VOXEL_CHUNK_SIZE + z*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE] = d;

	if (!flagChunks)
		return;

	world->flagChunk({posX,posY,posZ}, true);

	if (x == 0) {
		world->flagChunk({ posX - 1,posY,posZ }, true);
	}

	if (y == 0) {
		world->flagChunk({ posX,posY - 1,posZ }, true);
	}

	if (z == 0) {
		world->flagChunk({ posX,posY,posZ-1 }, true);
	}

}
/*
void VoxelChunk::send(int sys_index, int ply_id, bool init, int chunk_num) {
	net_sv_sendChunk(sys_index, ply_id, init, chunk_num , voxel_data, (VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE*VOXEL_CHUNK_SIZE)*2);
}
*/
void VoxelChunk::physicsMeshClearAll() {
	if (chunkBody != nullptr) {
		world->dynamicsWorld->removeRigidBody(chunkBody);
		delete chunkBody;
	}

	if (meshInterface != nullptr) {
		delete meshInterface;
	}

	meshInterface = nullptr;
	chunkBody = nullptr;

#ifdef VOXELATE_SERVER_VPHYSICS
	// *
	if (phys_obj!=nullptr) {
		IPhysicsEnvironment* env = IFACE_SH_PHYSICS->GetActiveEnvironmentByIndex(0);
		phys_obj->SetGameData(nullptr);
		phys_obj->EnableCollisions(false);
		phys_obj->RecheckCollisionFilter();
		phys_obj->RecheckContactPoints();
		env->DestroyObject(phys_obj);
		phys_obj = nullptr;

		//Not sure if we should be calling this, but it may be required to prevent a leak.
		IFACE_SH_COLLISION->DestroyCollide(phys_collider);
	}
	// */
#endif
}

#ifdef VOXELATE_CLIENT
void VoxelChunk::graphicsMeshClearAll() {
	CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);

	while (meshes.begin() != meshes.end()) {
		pRenderContext->DestroyStaticMesh(*meshes.begin());
		meshes.erase(meshes.begin());
	}
}
#endif

void VoxelChunk::physicsMeshStart() {
	meshInterface = new btTriangleMesh();

#ifdef VOXELATE_SERVER_VPHYSICS
	phys_soup = IFACE_SH_COLLISION->PolysoupCreate();
#endif
}

#ifdef VOXELATE_CLIENT
void VoxelChunk::graphicsMeshStart() {
	graphics_vertsRemaining = BUILD_MAX_VERTS;

	CMatRenderContextPtr pRenderContext(IFACE_CL_MATERIALS);
	current_mesh = pRenderContext->CreateStaticMesh(VOXEL_VERT_FMT, "");

	meshBuilder.Begin(current_mesh, MATERIAL_QUADS, BUILD_MAX_VERTS / 4);
}
#endif

void VoxelChunk::physicsMeshStop(CBaseEntity* ent) {
	if (meshInterface != nullptr) {
		btBvhTriangleMeshShape* trimesh = new btBvhTriangleMeshShape(meshInterface, true, true);
		// meshInterface = nullptr;

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 0, 0));

		btScalar mass(0.); // static
		btVector3 localInertia(0, 0, 0);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, trimesh, localInertia);
		chunkBody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		world->dynamicsWorld->addRigidBody(chunkBody);
	}
	
#ifdef VOXELATE_SERVER_VPHYSICS
	// *
	if (phys_soup == nullptr)
		return;

	phys_collider = IFACE_SH_COLLISION->ConvertPolysoupToCollide(phys_soup, false); //todo what the fuck is MOPP?
	IFACE_SH_COLLISION->PolysoupDestroy(phys_soup);
	phys_soup = nullptr;

	objectparams_t op = { 0 };
	op.enableCollisions = true;
	op.pGameData = static_cast<void *>(ent);
	op.pName = "voxels";

	Vector pos = eent_getPos(ent);

	IPhysicsEnvironment* env = IFACE_SH_PHYSICS->GetActiveEnvironmentByIndex(0);
	phys_obj = env->CreatePolyObjectStatic(phys_collider, 3, pos, QAngle(0, 0, 0), &op);
	// */
#endif
}

#ifdef VOXELATE_CLIENT
void VoxelChunk::graphicsMeshStop() {
	if (current_mesh == nullptr)
		return;

	meshBuilder.End();

	meshes.push_back(current_mesh);
	current_mesh = nullptr;

	graphics_vertsRemaining = 0;
}
#endif

void VoxelChunk::addSliceFace(int slice, int x, int y, int w, int h, int tx, int ty, byte dir, int scale) {
	double realStep = world->config.scale;
	double realX;
	double realY;
	double realZ;

	Vector v1, v2, v3, v4;
	switch (dir) {

	case DIR_X_POS:

		realX = (slice + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (x + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX + realStep * scale, realY, realZ);
		v2 = Vector(realX + realStep * scale, realY, realZ + realStep * h);
		v3 = Vector(realX + realStep * scale, realY + realStep * w, realZ + realStep * h);
		v4 = Vector(realX + realStep * scale, realY + realStep * w, realZ);
		break;

	case DIR_Y_POS:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (slice + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY + realStep * scale, realZ);
		v2 = Vector(realX + realStep * w, realY + realStep * scale, realZ);
		v3 = Vector(realX + realStep * w, realY + realStep * scale, realZ + realStep * h);
		v4 = Vector(realX, realY + realStep * scale, realZ + realStep * h);
		break;

	case DIR_Z_POS:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (y + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (slice + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY, realZ + realStep * scale);
		v2 = Vector(realX, realY + realStep * h, realZ + realStep * scale);
		v3 = Vector(realX + realStep * w, realY + realStep * h, realZ + realStep * scale);
		v4 = Vector(realX + realStep * w, realY, realZ + realStep * scale);
		break;

	case DIR_X_NEG:

		realX = (slice + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (x + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX + realStep * scale, realY, realZ);
		v2 = Vector(realX + realStep * scale, realY + realStep * w, realZ);
		v3 = Vector(realX + realStep * scale, realY + realStep * w, realZ + realStep * h);
		v4 = Vector(realX + realStep * scale, realY, realZ + realStep * h);
		break;

	case DIR_Y_NEG:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (slice + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY + realStep * scale, realZ);
		v2 = Vector(realX, realY + realStep * scale, realZ + realStep * h);
		v3 = Vector(realX + realStep * w, realY + realStep * scale, realZ + realStep * h);
		v4 = Vector(realX + realStep * w, realY + realStep * scale, realZ);
		break;

	case DIR_Z_NEG:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (y + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (slice + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY, realZ + realStep * scale);
		v2 = Vector(realX + realStep * w, realY, realZ + realStep * scale);
		v3 = Vector(realX + realStep * w, realY + realStep * h, realZ + realStep * scale);
		v4 = Vector(realX, realY + realStep * h, realZ + realStep * scale);
		break;

	default:
		return;
	}

	if (meshInterface == nullptr) {
		physicsMeshStart();
	}
	
	meshInterface->addTriangle(
		SourcePositionToBullet(v1),
		SourcePositionToBullet(v2),
		SourcePositionToBullet(v3)
	);
	meshInterface->addTriangle(
		SourcePositionToBullet(v1),
		SourcePositionToBullet(v3),
		SourcePositionToBullet(v4)
	);

#ifdef VOXELATE_CLIENT
	if (graphics_vertsRemaining < 4) {
		graphicsMeshStop();
		graphicsMeshStart();
	}
	graphics_vertsRemaining -= 4;

	VoxelConfig* cl_config = &(world->config);

	double uMin = ((double)tx / cl_config->atlasWidth);
	double uMax = ((tx + 1.0) / cl_config->atlasWidth);

	double vMin = ((double)ty / cl_config->atlasHeight);
	double vMax = ((ty + 1.0) / cl_config->atlasHeight);

	meshBuilder.Position3f(v1.x, v1.y, v1.z);
	meshBuilder.TexCoord2f(0, 0, h);
	meshBuilder.TexCoord2f(1, uMin, vMin);
	meshBuilder.Normal3f(1, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(v2.x, v2.y, v2.z);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.TexCoord2f(1, uMin, vMin);
	meshBuilder.Normal3f(1, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(v3.x, v3.y, v3.z);
	meshBuilder.TexCoord2f(0, w, 0);
	meshBuilder.TexCoord2f(1, uMin, vMin);
	meshBuilder.Normal3f(1, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(v4.x, v4.y, v4.z);
	meshBuilder.TexCoord2f(0, w, h);
	meshBuilder.TexCoord2f(1, uMin, vMin);
	meshBuilder.Normal3f(1, 0, 0);
	meshBuilder.AdvanceVertex();
#elif VOXELATE_SERVER_VPHYSICS
	// *
	if (phys_soup == nullptr) {
		physicsMeshStart();
	}

	IFACE_SH_COLLISION->PolysoupAddTriangle(phys_soup, v1, v2, v3, 3);
	IFACE_SH_COLLISION->PolysoupAddTriangle(phys_soup, v1, v3, v4, 3);
	// */
#endif
}
