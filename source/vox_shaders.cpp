#if VOXELATE_CLIENT

#include <vox_util.h>
#include <materialsystem/IShader.h>
#include <symbolfinder.hpp>
#include <detours.h>

#include <shaders/shader_voxels.h>

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

/*
How to find these patterns:

find "game_shader_generic_garrysmod"

xref up to the vtable, which looks like this:

4: load_std_shader_modules
5
6: load_all_shader_modules
7
8
9
10: find_shader <==============

*/

#if defined _WIN32
uint8_t FindShader_pattern[] = {
	0x55, // push ebp
	0x8B, 0xEC, // mov ebp, esp
	0x83, 0xEC, 0x0C, // sub esp, 0Ch
	0x53, // push ebx
	0x56, // push esi
	0x8B, 0x2A, 0x2A, // mov ?,[?]
	0x83, 0x2A, 0x2A, // sub ?,?
	0x89, 0x2A, 0x2A, // mov [?],?
	0x57, // push edi
	0x78, 0x2A, // js ?
	0x8B, 0x2A, 0x2A, // mov ?, [?]
	0x8D, 0x3C, 0x76 // lea edi, [esi+esi*2]
};
#elif defined __linux__
// NOTE: this signature is unmasked!!!!!! someone plz mask it im too lazy i only did windows
uint8_t FindShader_pattern[] = {
	0x55, 0x89, 0xE5, 0x57, 0x56, 0x53, 0x83, 0xEC, 0x1C, 0x8B, 0x45, 0x08, 0x8B, 0x70, 0x14, 0x83,
	0xEE, 0x01, 0x8D, 0x1C, 0x76, 0xC1, 0xE3, 0x04, 0xEB, 0x09, 0x8D, 0xB6, 0x00, 0x00, 0x00, 0x00,
	0x83, 0xEE, 0x01, 0x85, 0xF6, 0x78, 0x39, 0x8B, 0x45, 0x08, 0x89, 0xDF, 0x83, 0xEB, 0x30, 0x03,
	0x78, 0x08, 0x8B, 0x45, 0x0C, 0x89, 0x44, 0x24, 0x04, 0x8D, 0x47, 0x14, 0x89, 0x04, 0x24, 0xE8
};
#elif defined __APPLE__
// NOTE: this signature is ALSO unmasked!!!!!! someone plz mask it im too lazy i only did windows
// My materialsystem binary for OSX has all the symbol names preserved!
// CShaderSystem::FindShader
uint8_t FindShader_pattern[] = {
	0x55, 0x89, 0xE5, 0x53, 0x57, 0x56, 0x83, 0xEC, 0x1C, 0x8B, 0x45, 0x08, 0x8B, 0x78, 0x14, 0x6B,
	0xD7, 0x30, 0x47, 0x83, 0xC2, 0xE4, 0x8B, 0x4D, 0x0C, 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00,
	0x89, 0xD6, 0x4F, 0x31, 0xC0, 0x85, 0xFF, 0x7E, 0x43, 0x8D, 0x56, 0xD0, 0x8B, 0x45, 0x08, 0x8B,
	0x58, 0x08, 0x85, 0xC9, 0x74, 0xEA, 0x89, 0x4D, 0xE8, 0x8D, 0x45, 0xE8, 0x89, 0x44, 0x24, 0x04
};
#else
#error NO SHADER PATTERN FOR THIS PLATFORM. WAT?
#endif

bool installShaders() {
	MH_Initialize();

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