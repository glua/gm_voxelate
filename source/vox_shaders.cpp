#if VOXELATE_CLIENT

#include <vox_util.h>
#include <materialsystem/IShader.h>
#include <symbolfinder.hpp>
#include <detours.h>

#include <shader_voxels.h>

typedef IShader* (__thiscall *FindShader_t)(void *_this, const char* name);

FindShader_t FindShader_base;

IShader* __fastcall FindShader_hook(IShader* _this, void* _edx, const char* name) {

	if (strcmp(name, "Voxels") == 0) {
		return &Voxel_Shader::s_ShaderInstance;
	}
	
	IShader* shader = FindShader_base(_this, name);
	return shader;
}

MologieDetours::Detour<FindShader_t>* FindShader_detour = nullptr;

uint8_t FindShader_pattern[] = {
	0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x0C, 0x53, 0x56, 0x57, 0x8B, 0x79, 0x14, 0x4F, 0x89, 0x4D, 0xFC,
	0x78, 0x36, 0x8D, 0x1C, 0x7F, 0xC1, 0xE3, 0x04, 0x8B, 0x45, 0x08, 0x8B, 0x71, 0x08, 0x85, 0xC0,
	0x74, 0x20, 0x89, 0x45, 0xF4, 0x8D, 0x45, 0xF4, 0x50, 0x8D, 0x4C, 0x1E, 0x14, 0xE8, 0x2E, 0xF5,
	0xFF, 0xFF, 0x0F, 0xB7, 0xC0, 0xB9, 0xFF, 0xFF, 0x00, 0x00, 0x66, 0x3B, 0xC1, 0x75, 0x14, 0x8B
};

bool installShaders() {

	SymbolFinder finder;

	void* FindShader_ptr = finder.FindPatternFromBinary("materialsystem", FindShader_pattern, sizeof(FindShader_pattern));

	if (FindShader_ptr == nullptr) {
		vox_print("Shader lookup function not found.");
		return false;
	}

	try {
		FindShader_detour = new MologieDetours::Detour<FindShader_t>(reinterpret_cast<FindShader_t>(FindShader_ptr), reinterpret_cast<FindShader_t>(&FindShader_hook));
		FindShader_base = FindShader_detour->GetOriginalFunction();
	}
	catch (MologieDetours::DetourException &e) {
		vox_print("Shader lookup detour failed.");
		return false;
	}

	return true;
}

void uninstallShaders() {
	delete FindShader_detour;
}

#else

bool installShaders() {
	return true;
}
void uninstallShaders() {}

#endif