#pragma once

#include "glua.h"

#include "materialsystem/imaterialsystem.h"
#include "materialsystem\ishadersystem_declarations.h"
#include "materialsystem\ishaderapi.h"
#include "extra_sourcesdk_headers/ishadersystem.h"
#include "vphysics_interface.h"
#include "icliententitylist.h"
#include "eiface.h"
// #include "eifacev21.h"

// Server interfaces
extern IVEngineServer* IFACE_SV_ENGINE;
extern IPhysicsCollision* IFACE_SV_COLLISION;
extern IPhysics* IFACE_SV_PHYSICS;

// Client interfaces
extern IMaterialSystem* IFACE_CL_MATERIALS;
extern IShaderSystemInternal* IFACE_CL_SHADERS;


bool init_interfaces();