#pragma once

#include "glua.h"

#include "materialsystem/imaterialsystem.h"
#include "vphysics_interface.h"
#include "icliententitylist.h"
#include "eiface.h"
// #include "eifacev21.h"
#include "mathlib/vector.h"

// Server interfaces
extern IVEngineServer* IFACE_SV_ENGINE;
extern IPhysicsCollision* IFACE_SH_COLLISION;
extern IPhysics* IFACE_SH_PHYSICS;

// Client interfaces
extern IMaterialSystem* IFACE_CL_MATERIALS;

bool init_interfaces();
