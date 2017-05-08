#pragma once

#ifdef GMOD_ALLOW_DEPRECATED
#if !defined( _MSC_VER ) || _MSC_VER >= 1900
#define GMOD_NOEXCEPT noexcept
#elif _MSC_VER >= 1700
#include <yvals.h>
#define GMOD_NOEXCEPT _NOEXCEPT
#else
#define GMOD_NOEXCEPT
#endif
#endif

#include <GarrysMod/Lua/Interface.h>
#include <lua.hpp>
#include <GarrysMod/Interfaces.hpp>

namespace sn_bf_common {
	extern const char *tostring_format;

	int index(lua_State *state) GMOD_NOEXCEPT;
	int newindex(lua_State *state) GMOD_NOEXCEPT;
	int GetTable(lua_State *state) GMOD_NOEXCEPT;

	void CheckType(GarrysMod::Lua::ILuaBase *LAU, int32_t index, int32_t type, const char *nametype);

	void ThrowError(GarrysMod::Lua::ILuaBase *LAU, const char *fmt, ...);
}
