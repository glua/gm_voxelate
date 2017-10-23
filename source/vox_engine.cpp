#include "vox_engine.h"

#include <GarrysMod/Interfaces.hpp>
// #include <GarrysMod/Lua/LuaInterface.h>
// #include <GarrysMod/Lua/LuaShared.h>

// Deals with different module names on loonix
// Could use Meta's stuff but W/E
#if IS_SERVERSIDE && defined(__linux__)
#define MODULENAME(_module_) _module_  "_srv"
#else
#define MODULENAME(_module_) _module_
#endif

#define LOADINTERFACE(_module_, _version_, _out_) Sys_LoadInterface(MODULENAME(_module_), _version_, NULL, reinterpret_cast<void**>(& _out_ ))

// Serverside interfaces
IVEngineServer* IFACE_SV_ENGINE;
IPhysics* IFACE_SH_PHYSICS;

// Clientside interfaces
IMaterialSystem* IFACE_CL_MATERIALS;

// Shared interfaces
IPhysicsCollision* IFACE_SH_COLLISION;
// GarrysMod::Lua::ILuaShared* IFACE_SH_LUA;

// Internal shader dll
abstract_class IShaderDLLInternal
{
public:
	virtual bool Connect(CreateInterfaceFn factory, bool bIsMaterialSystem) = 0;
};

#ifdef VOXELATE_CLIENT
IShaderDLLInternal *GetShaderDLLInternal();
#endif

#include "vox_util.h"

// Sets up interfaces
bool init_interfaces() {
	if (!LOADINTERFACE("vphysics", VPHYSICS_COLLISION_INTERFACE_VERSION, IFACE_SH_COLLISION))
		return false;
	if (!LOADINTERFACE("vphysics", VPHYSICS_INTERFACE_VERSION, IFACE_SH_PHYSICS))
		return false;

#ifdef VOXELATE_SERVER
	if (!LOADINTERFACE("engine", INTERFACEVERSION_VENGINESERVER, IFACE_SV_ENGINE))
		return false;
#else
	if (!LOADINTERFACE("materialsystem", MATERIAL_SYSTEM_INTERFACE_VERSION, IFACE_CL_MATERIALS))
		return false;

	// This initializes some of our module's internal shit that the shader library needs.
	IShaderDLLInternal* shader_dll = GetShaderDLLInternal();
	CreateInterfaceFn factory = Sys_GetFactory("materialsystem");
	if (!(shader_dll->Connect(factory, false)))
		return false;
#endif

	return true;
}
