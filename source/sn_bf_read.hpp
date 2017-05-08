#pragma once

#include "sn_bf_common.hpp"

class bf_read;

namespace sn_bf_read
{

bf_read **Push(
	GarrysMod::Lua::ILuaBase *LAU,
	bf_read *reader = nullptr,
	int32_t bufref = LUA_NOREF
);

bf_read *Get( GarrysMod::Lua::ILuaBase *LAU, int32_t index, int32_t *bufref = nullptr );

void Initialize( GarrysMod::Lua::ILuaBase *LAU);

void Deinitialize( GarrysMod::Lua::ILuaBase *LAU);

}
