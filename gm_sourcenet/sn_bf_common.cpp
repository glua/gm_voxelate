#include "sn_bf_common.hpp"

namespace sn_bf_common {
#if defined _WIN32

	const char *tostring_format = "%s: 0x%p";

#elif defined __linux || defined __APPLE__

	const char *tostring_format = "%s: %p";

#endif

	LUA_FUNCTION(index) {
		LUA->GetMetaTable(1);
		LUA->Push(2);
		LUA->RawGet(-2);
		if (!LUA->IsType(-1, GarrysMod::Lua::Type::NIL))
			return 1;

		LUA->Pop(2);

		lua_getfenv(LUA->GetLuaState(), 1);
		LUA->Push(2);
		LUA->RawGet(-2);
		return 1;
	}

	LUA_FUNCTION(newindex) {
		lua_getfenv(LUA->GetLuaState(), 1);
		LUA->Push(2);
		LUA->Push(3);
		LUA->RawSet(-3);
		return 0;
	}

	LUA_FUNCTION(GetTable) {
		lua_getfenv(LUA->GetLuaState(), 1);
		return 1;
	}

	void CheckType(GarrysMod::Lua::ILuaBase *LUA, int32_t index, int32_t type, const char *nametype) {
		if (!LUA->IsType(index, type))
			luaL_typerror(LUA->GetLuaState(), index, nametype);
	}

	void ThrowError(GarrysMod::Lua::ILuaBase *LUA, const char *fmt, ...) {
		va_list args;
		va_start(args, fmt);
		const char *error = lua_pushvfstring(LUA->GetLuaState(), fmt, args);
		va_end(args);
		LUA->ThrowError(error);
	}
}
