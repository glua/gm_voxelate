#include "vox_util.h"

#include "tier0/dbg.h"
#include "Color.h"

Color COLOR_LIME(0, 255, 0, 255);

void vox_print(const char* msg, ...) {
	char buffer[256];
	
	va_list args;
	va_start(args, msg);
	vsprintf(buffer, msg, args);
	va_end(args);
	
	ConMsg("[");
	ConColorMsg(COLOR_LIME, "VOX");
	ConMsg("] ");
	ConMsg(buffer);
	ConMsg("\n");
}

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