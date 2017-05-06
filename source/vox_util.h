#pragma once

#include "mathlib/vector.h"

void vox_print(const char* msg,...);

class CBaseEntity;
Vector eent_getPos(CBaseEntity* ent);