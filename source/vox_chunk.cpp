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
	int offset_x = posX*VOXEL_CHUNK_SIZE;
	int offset_y = posY*VOXEL_CHUNK_SIZE;
	int offset_z = posZ*VOXEL_CHUNK_SIZE;

	int max_x = world->config.huge ? VOXEL_CHUNK_SIZE : MIN(world->config.dims_x - offset_x, VOXEL_CHUNK_SIZE);
	int max_y = world->config.huge ? VOXEL_CHUNK_SIZE : MIN(world->config.dims_y - offset_y, VOXEL_CHUNK_SIZE);
	int max_z = world->config.huge ? VOXEL_CHUNK_SIZE : MIN(world->config.dims_z - offset_z, VOXEL_CHUNK_SIZE);

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

	VoxelChunk* next_chunk_x = world->getChunk(posX + 1, posY, posZ);
	VoxelChunk* next_chunk_y = world->getChunk(posX, posY + 1, posZ);
	VoxelChunk* next_chunk_z = world->getChunk(posX, posY, posZ + 1);

	bool huge = world->config.huge;
	bool buildExterior = world->config.buildExterior;

	// Hoo boy. Get ready to see some shit.

	int hard_upper_bound_x = (world->config.dims_x - posX*VOXEL_CHUNK_SIZE);
	int hard_upper_bound_y = (world->config.dims_y - posY*VOXEL_CHUNK_SIZE);
	int hard_upper_bound_z = (world->config.dims_z - posZ*VOXEL_CHUNK_SIZE);

	int upper_bound_x = !huge && hard_upper_bound_x < VOXEL_CHUNK_SIZE ? hard_upper_bound_x : VOXEL_CHUNK_SIZE;
	int upper_bound_y = !huge && hard_upper_bound_y < VOXEL_CHUNK_SIZE ? hard_upper_bound_y : VOXEL_CHUNK_SIZE;
	int upper_bound_z = !huge && hard_upper_bound_z < VOXEL_CHUNK_SIZE ? hard_upper_bound_z : VOXEL_CHUNK_SIZE;

	int lower_slice_x = !huge && buildExterior && posX == 0 ? -1 : 0;
	int lower_slice_y = !huge && buildExterior && posY == 0 ? -1 : 0;
	int lower_slice_z = !huge && buildExterior && posZ == 0 ? -1 : 0;

	if (!huge && !buildExterior) {
		hard_upper_bound_x--;
		hard_upper_bound_y--;
		hard_upper_bound_z--;
	}

	int upper_slice_x = !huge && hard_upper_bound_x < VOXEL_CHUNK_SIZE ? hard_upper_bound_x : VOXEL_CHUNK_SIZE;
	int upper_slice_y = !huge && hard_upper_bound_y < VOXEL_CHUNK_SIZE ? hard_upper_bound_y : VOXEL_CHUNK_SIZE;
	int upper_slice_z = !huge && hard_upper_bound_z < VOXEL_CHUNK_SIZE ? hard_upper_bound_z : VOXEL_CHUNK_SIZE;

	VoxelType* blockTypes = world->config.voxelTypes;

	SliceFace faces[VOXEL_CHUNK_SIZE][VOXEL_CHUNK_SIZE];

	int scaling = 1;

	switch (lod) {
	case ELevelOfDetail::FULL:
		break;
	case ELevelOfDetail::TWO_MERGE:
		scaling = 2;
		break;
	default:
		scaling = VOXEL_CHUNK_SIZE;
	}

	// Slices along x axis!
	for (int slice_x = lower_slice_x; slice_x < upper_slice_x; slice_x += scaling) {

		for (int z = 0; z < upper_bound_z; z++) {
			for (int y = 0; y < upper_bound_y; y++) {

				// Compute base type
				BlockData base;

				if (slice_x < 0)
					base = 0;
				else
					base = get(slice_x, y, z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_x;
				if (slice_x == VOXEL_CHUNK_SIZE - 1)
					if (next_chunk_x == nullptr)
						offset_x = 0;
					else
						offset_x = next_chunk_x->get(0, y, z);
				else
					offset_x = get(slice_x + 1 , y, z);

				VoxelType& offset_x_type = blockTypes[offset_x];

				// Add faces!
				SliceFace& face = faces[z][y];

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

		buildSlice(slice_x, DIR_X_POS, faces, upper_bound_y, upper_bound_z, scaling);
	}

	// Slices along y axis!
	for (int slice_y = lower_slice_y; slice_y < upper_slice_y; slice_y += scaling) {

		for (int z = 0; z < upper_bound_z; z++) {
			for (int x = 0; x < upper_bound_x; x++) {

				// Compute base type
				BlockData base;

				if (slice_y < 0)
					base = 0;
				else
					base = get(x, slice_y, z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_y;
				if (slice_y == VOXEL_CHUNK_SIZE - 1)
					if (next_chunk_y == nullptr)
						offset_y = 0;
					else
						offset_y = next_chunk_y->get(x, 0, z);
				else
					offset_y = get(x, slice_y + 1, z);

				VoxelType& offset_y_type = blockTypes[offset_y];

				// Add faces!
				SliceFace& face = faces[z][x];

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

		buildSlice(slice_y, DIR_Y_POS, faces, upper_bound_x, upper_bound_z, scaling);
	}

	// Slices along z axis! TODO ALSO PROCESS NON-CUBIC BLOCKS IN -THIS- STAGE

	for (int slice_z = lower_slice_z; slice_z < upper_slice_z; slice_z += scaling) {
		
		for (int y = 0; y < upper_bound_y; y++) {
			for (int x = 0; x < upper_bound_x; x++) {
				
				// Compute base type
				BlockData base;

				if (slice_z < 0)
					base = 0;
				else
					base = get(x, y, slice_z);

				VoxelType& base_type = blockTypes[base];

				// Compute offset type
				BlockData offset_z;
				if (slice_z == VOXEL_CHUNK_SIZE - 1)
					if (next_chunk_z == nullptr)
						offset_z = 0;
					else
						offset_z = next_chunk_z->get(x, y, 0);
				else
					offset_z = get(x, y, slice_z + 1);

				VoxelType& offset_z_type = blockTypes[offset_z];

				// Add faces!
				SliceFace& face = faces[y][x];

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

		buildSlice(slice_z, DIR_Z_POS, faces, upper_bound_x, upper_bound_y, scaling);
	}

	//final build
	physicsMeshStop(ent);
#ifdef VOXELATE_CLIENT
	graphicsMeshStop();
#endif
}

void VoxelChunk::buildSlice(int slice, byte dir, SliceFace faces[VOXEL_CHUNK_SIZE][VOXEL_CHUNK_SIZE], int upper_bound_x, int upper_bound_y, int scaling) {

	for (int y = 0; y < upper_bound_y; y += scaling) {
		for (int x = 0; x < upper_bound_x; x += scaling) {

			if (faces[y][x].present) {
				SliceFace& current_face = faces[y][x];

				int end_x;
				int end_y;

				for (end_x = x + 1; end_x < upper_bound_x && current_face == faces[y][end_x]; end_x++) {
					faces[y][end_x].present = false;
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
			bail:

				current_face.present = false;

				int w = end_x - x;
				int h = end_y - y;

#ifdef VOXELATE_CLIENT
				addSliceFace(slice, x, y, w, h, current_face.texture.x, current_face.texture.y, current_face.direction ? dir : dir+3, scaling);
#else
				addSliceFace(slice, x, y, w, h, 0, 0, dir, scaling);
#endif
			}
		}
	}
}

void VoxelChunk::draw(CMatRenderContextPtr& pRenderContext) {
	for (IMesh* m : meshes) {
		m->Draw();
	}
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

#ifdef VOXELATE_SERVER
	// *
	if (phys_obj!=nullptr) {
		IPhysicsEnvironment* env = IFACE_SV_PHYSICS->GetActiveEnvironmentByIndex(0);
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

#ifdef VOXELATE_SERVER
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

		btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -56, 0));

		btScalar mass(0.); // static
		btVector3 localInertia(0, 0, 0);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		chunkBody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		world->dynamicsWorld->addRigidBody(chunkBody);
	}
	
#ifdef VOXELATE_SERVER
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

	IPhysicsEnvironment* env = IFACE_SV_PHYSICS->GetActiveEnvironmentByIndex(0);
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

void VoxelChunk::addSliceFace(int slice, int x, int y, int w, int h, int tx, int ty, byte dir, int scaling) {
	double realStep = world->config.scale * scaling;
	double realX;
	double realY;
	double realZ;

	Vector v1, v2, v3, v4;
	switch (dir) {

	case DIR_X_POS:

		realX = (slice + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (x + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX + realStep, realY, realZ);
		v2 = Vector(realX + realStep, realY, realZ + realStep * h);
		v3 = Vector(realX + realStep, realY + realStep * w, realZ + realStep * h);
		v4 = Vector(realX + realStep, realY + realStep * w, realZ);
		break;

	case DIR_Y_POS:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (slice + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (y + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY + realStep, realZ);
		v2 = Vector(realX + realStep * w, realY + realStep, realZ);
		v3 = Vector(realX + realStep * w, realY + realStep, realZ + realStep * h);
		v4 = Vector(realX, realY + realStep, realZ + realStep * h);
		break;

	case DIR_Z_POS:

		realX = (x + posX*VOXEL_CHUNK_SIZE) * realStep;
		realY = (y + posY*VOXEL_CHUNK_SIZE) * realStep;
		realZ = (slice + posZ*VOXEL_CHUNK_SIZE) * realStep;

		v1 = Vector(realX, realY, realZ + realStep);
		v2 = Vector(realX, realY + realStep * h, realZ + realStep);
		v3 = Vector(realX + realStep * w, realY + realStep * h, realZ + realStep);
		v4 = Vector(realX + realStep * w, realY, realZ + realStep);
		break;

	default:
		return;
	}

	if (meshInterface == nullptr) {
		physicsMeshStart();
	}
	
	meshInterface->addTriangle(
		btVector3(v1.x, v1.y, v1.z),
		btVector3(v2.x, v2.y, v2.z),
		btVector3(v3.x, v3.y, v3.z)
	);
	meshInterface->addTriangle(
		btVector3(v1.x, v1.y, v1.z),
		btVector3(v3.x, v3.y, v3.z),
		btVector3(v4.x, v4.y, v4.z)
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
#elif VOXELATE_SERVER
	// *
	if (phys_soup == nullptr) {
		physicsMeshStart();
	}

	IFACE_SH_COLLISION->PolysoupAddTriangle(phys_soup, v1, v2, v3, 3);
	IFACE_SH_COLLISION->PolysoupAddTriangle(phys_soup, v1, v3, v4, 3);
	// */
#endif
}
