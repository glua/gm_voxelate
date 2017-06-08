#include "vox_util.h"

#include "tier0/dbg.h"
#include "Color.h"

// Basically printf with some fancy shit.
void vox_print(const char* msg, ...) {
	char buffer[256];
	
	va_list args;
	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);
	
	ConColorMsg(Color(100, 255, 100, 255), "[Voxelate] ");

#if IS_SERVERSIDE
	ConColorMsg(Color(0x91, 0xdb, 0xe7, 0xff), buffer);
#else
	ConColorMsg(Color(0xe7, 0xdb, 0x74, 0xff), buffer);
#endif

	ConMsg("\n");
}

// I will shit if this still works.
// I guess it still works lol?
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