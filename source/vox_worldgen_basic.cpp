#include "vox_worldgen_basic.h"

BlockData vox_worldgen_basic(VoxelCoord _x, VoxelCoord _y, VoxelCoord _z) {
	auto x = static_cast<double>(_x);
	auto y = static_cast<double>(_y);
	auto z = static_cast<double>(_z);

	/*
	z = z - 50 + floor(sin((x + 30) / 32) * 8 + cos((y - 20) / 32) * 8);

	if (z < 40) {
		return 7;
	}
	else if (z < 49) {
		return 8;
	}
	else if (z < 50) {
		return 1;
	}
	else if (z == 50) {
		if (x > 315 && x < 325 && y>316 && y < 324) {
			if (x > 316 && x < 324 && y>317 && y < 323) {
				return 6;
			}

			return 5;
		}
	}
	*/

	if (z < -40) {
		return 7; // stone
	}
	else if (z < 0) {
		return 8; // dirt
	}
	else if (z == 0) {
		return 1; // grass
	}

	return 0;
}
