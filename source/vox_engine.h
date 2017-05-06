#pragma once

#include "glua.h"

#include "materialsystem/imaterialsystem.h"
#include "vphysics_interface.h"
#include "icliententitylist.h"
#include "eiface.h"
// #include "eifacev21.h"

extern IVEngineServer* iface_sv_ents;
extern IPhysicsCollision* iface_sv_collision;
extern IPhysics* iface_sv_physics;

extern IMaterialSystem* iface_cl_materials;

extern bool STATE_CLIENT;
extern bool STATE_SERVER;

bool determine_state(lua_State* state);
bool init_interfaces();