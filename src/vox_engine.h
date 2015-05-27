#pragma once

#include "GarrysMod/Lua/Interface.h"

#include "materialsystem/imaterialsystem.h"
#include "vphysics_interface.h"
#include "icliententitylist.h"
#include "eiface.h"

extern IMaterialSystem* iface_materials;
extern IPhysicsCollision* iface_collision;
extern IPhysics* iface_physics;
extern IClientEntityList* iface_cl_entlist;
extern IVEngineServer* iface_sv_ents;

extern bool STATE_CLIENT;
extern bool STATE_SERVER;

bool determine_state(lua_State* state);
bool init_interfaces();