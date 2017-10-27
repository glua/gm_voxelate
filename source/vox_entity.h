#pragma once

#include "vox_world_common.h"

class VoxelWorld;

class VoxelEntity {
public:
	VoxelWorld* world;
    EntityID id;

	btVector3 pos;
	btQuaternion angQuat;

	btRigidBody* physicsBody;
};
