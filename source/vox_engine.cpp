#include "vox_engine.h"

#include <GarrysMod/Interfaces.hpp>

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
IPhysics* IFACE_SV_PHYSICS;
IPhysicsCollision* IFACE_SV_COLLISION;

// Clientside interfaces
IMaterialSystem* IFACE_CL_MATERIALS;

//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
abstract_class IShaderDLLInternal
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect(CreateInterfaceFn factory, bool bIsMaterialSystem) = 0;
	virtual void Disconnect(bool bIsMaterialSystem) = 0;

	// Returns the number of shaders defined in this DLL
	virtual int ShaderCount() const = 0;

	// Returns information about each shader defined in this DLL
	virtual IShader *GetShader(int nShader) = 0;
};

IShaderDLLInternal *GetShaderDLLInternal();
// Sets up interfaces
bool init_interfaces() {

	if (IS_SERVERSIDE) {
		if (!LOADINTERFACE("engine", INTERFACEVERSION_VENGINESERVER, IFACE_SV_ENGINE))
			return false;
		if (!LOADINTERFACE("vphysics", VPHYSICS_INTERFACE_VERSION, IFACE_SV_PHYSICS))
			return false;
		if (!LOADINTERFACE("vphysics", VPHYSICS_COLLISION_INTERFACE_VERSION, IFACE_SV_COLLISION))
			return false;
	}
	else {
		if (!LOADINTERFACE("materialsystem", MATERIAL_SYSTEM_INTERFACE_VERSION, IFACE_CL_MATERIALS))
			return false;
		IShaderDLLInternal* shader_dll = GetShaderDLLInternal();
		CreateInterfaceFn factory = Sys_GetFactory("materialsystem");
	}

	return true;
}