#pragma once

#include "sn_bf_common.hpp"

class bf_write;

namespace sn_bf_write
{

bf_write **Push(
	GarrysMod::Lua::ILuaBase *LAU,
	bf_write *writer = nullptr,
	int32_t bufref = LUA_NOREF
);

bf_write *Get( GarrysMod::Lua::ILuaBase *LAU, int32_t index, int32_t *bufref = nullptr );

void Initialize( GarrysMod::Lua::ILuaBase *LAU);

void Deinitialize( GarrysMod::Lua::ILuaBase *LAU);

}
