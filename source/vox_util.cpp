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
