#if VOXELATE_CLIENT

#include <vox_engine.h>
#include <vox_util.h>
#include <symbolfinder.hpp>
#include <detours.h>

#include <shaders/shader_voxels.h>

#include "detouring/classproxy.hpp"

using namespace Detouring;

class IVoxelateShaderSystemInternalProxy : Detouring::ClassProxy<IShaderSystemInternal, IVoxelateShaderSystemInternalProxy> {
public:
	IVoxelateShaderSystemInternalProxy(IShaderSystemInternal *instance) : Detouring::ClassProxy<IShaderSystemInternal, IVoxelateShaderSystemInternalProxy>(instance) {
		Hook(&IShaderSystemInternal::FindShader, &IVoxelateShaderSystemInternalProxy::FindShader);
	}

	IShader* FindShader(const char* name) {

		if (strcmp(name, "Voxels") == 0) {
			return &Voxel_Shader::s_ShaderInstance;
		}

		IShader* shader = Call(&IShaderSystemInternal::FindShader, name);

		return shader;
	}
};

IVoxelateShaderSystemInternalProxy* VoxShaderSystemInternalProxy;


bool installShaders() {
	VoxShaderSystemInternalProxy = new IVoxelateShaderSystemInternalProxy(IFACE_CL_SHADERS);

	return true;
}

void uninstallShaders() {
	delete VoxShaderSystemInternalProxy;
}

#else

bool installShaders() {
	return true;
}
void uninstallShaders() {}

#endif