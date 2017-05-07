#include "vox_util.h"

#include "tier0/dbg.h"
#include "Color.h"

void vox_print(const char* msg, ...) {
	char buffer[256];
	
	va_list args;
	va_start(args, msg);
	vsprintf(buffer, msg, args);
	va_end(args);
	
	ConMsg("[");
	ConColorMsg(Color(0,255,0,255), "VOXELATE");
	ConMsg("-");

#if IS_SERVERSIDE
	ConColorMsg(Color(0,0,255,0), "SV");
#else
	ConColorMsg(Color(255, 255, 0, 0), "CL");
#endif

	ConMsg("] ");
	ConMsg(buffer);
	ConMsg("\n");
}

// I will shit if this still works.
Vector eent_getPos(CBaseEntity* ent) {
	/*float* a_ptr = reinterpret_cast<float*>(ent);

	for (int i=0;i<1000;i++) {
		vox_print("%i - %f",i,a_ptr[i]);
	}
	*/
	#ifdef LINUX
	byte* pos_ptr = reinterpret_cast<byte*>(ent)+792;
	#else
	byte* pos_ptr = reinterpret_cast<byte*>(ent)+772;
	#endif

	return *reinterpret_cast<Vector*>(pos_ptr);
}