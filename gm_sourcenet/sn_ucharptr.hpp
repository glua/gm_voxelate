#pragma once

#include <GarrysMod/Lua/Interface.h>
#include <lua.hpp>
#include <GarrysMod/Interfaces.hpp>

namespace UCHARPTR
{

extern uint8_t metatype;
extern const char *metaname;

uint8_t *Push( GarrysMod::Lua::ILuaBase *LAU, int32_t bits, uint8_t *data = nullptr );

uint8_t *Get(
	GarrysMod::Lua::ILuaBase *LAU,
	int32_t index,
	int32_t *bits = nullptr,
	bool *own = nullptr
);

uint8_t *Release( GarrysMod::Lua::ILuaBase *LAU, int32_t index, int32_t *bits );

void Initialize( GarrysMod::Lua::ILuaBase *LAU);

void Deinitialize( GarrysMod::Lua::ILuaBase *LAU);

}
