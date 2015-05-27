#include "vox_engine.h"

#define LOADINTERFACE(_module_, _version_, _out_) Sys_LoadInterface(_module_, _version_, NULL, reinterpret_cast<void**>(& _out_ ))

IMaterialSystem* iface_materials;
IPhysicsCollision* iface_collision;
IPhysics* iface_physics;

IClientEntityList* iface_cl_entlist;

VEngineServerV21::IVEngineServer* iface_sv_ents;

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
	
	if (!LOADINTERFACE("materialsystem", MATERIAL_SYSTEM_INTERFACE_VERSION, iface_materials))
		return false;

	if (!LOADINTERFACE("vphysics", VPHYSICS_COLLISION_INTERFACE_VERSION, iface_collision))
		return false;

	if (!LOADINTERFACE("vphysics", VPHYSICS_INTERFACE_VERSION, iface_physics))
		return false;

	if (STATE_CLIENT) {
		if (!LOADINTERFACE("client", VCLIENTENTITYLIST_INTERFACE_VERSION, iface_cl_entlist))
			return false;
	}
	else {
		if (!LOADINTERFACE("engine", VENGINESERVER_INTERFACEVERSION_21, iface_sv_ents))
			return false;
	}

	return true;
}