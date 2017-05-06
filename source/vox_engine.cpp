#include "vox_engine.h"

#include <GarrysMod/Interfaces.hpp>

#if defined(IS_SERVERSIDE) && defined(__linux__)
#define MODULENAME(_module_) _module_  "_srv"
#else
#define MODULENAME(_module_) _module_
#endif

#define LOADINTERFACE(_module_, _version_, _out_) Sys_LoadInterface(MODULENAME(_module_), _version_, NULL, reinterpret_cast<void**>(& _out_ ))

IVEngineServer* iface_sv_ents;
IPhysicsCollision* iface_sv_collision;
IPhysics* iface_sv_physics;

IMaterialSystem* iface_cl_materials;

bool STATE_CLIENT = false;
bool STATE_SERVER = false;

bool determine_state(lua_State* state) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	
	LUA->GetField(-1,"CLIENT");
	if (LUA->GetBool()) {
		STATE_CLIENT = true;
		LUA->Pop(2);
		return true;
	}
	LUA->Pop();

	LUA->GetField(-1, "SERVER");
	if (LUA->GetBool()) {
		STATE_SERVER = true;
		LUA->Pop(2);
		return true;
	}
	LUA->Pop(2);

	return false;
}

bool init_interfaces() {
	

	if (STATE_CLIENT) {
		if (!LOADINTERFACE("materialsystem", MATERIAL_SYSTEM_INTERFACE_VERSION, iface_cl_materials))
			return false;
	}
	else {
		
		if (!LOADINTERFACE("engine", INTERFACEVERSION_VENGINESERVER, iface_sv_ents))
			return false;
		if (!LOADINTERFACE("vphysics", VPHYSICS_INTERFACE_VERSION, iface_sv_physics))
			return false;
		if (!LOADINTERFACE("vphysics", VPHYSICS_COLLISION_INTERFACE_VERSION, iface_sv_collision))
			return false;
	}

	return true;
}