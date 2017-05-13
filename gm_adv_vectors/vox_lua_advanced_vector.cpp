#include "vox_lua_advanced_vector.h"

static uint8_t metatype = GarrysMod::Lua::Type::NONE;
static const char *metaname = "AdvancedVector";

#include "reactphysics3d.h"

#include <memory>

#if defined _WIN32
const char *tostring_format = "%s: 0x%p";
#elif defined __linux || defined __APPLE__
const char *tostring_format = "%s: %p";
#endif

typedef std::shared_ptr<reactphysics3d::Vector3> VectorPtr;

#define LUA state->luabase

VectorPtr* vox_lua_pushVector(lua_State* state, reactphysics3d::Vector3* vec) {
	VectorPtr* udata = LUA->NewUserType<VectorPtr>(metatype);

	udata = new VectorPtr(vec);

	return udata;
}

VectorPtr* vox_lua_getVectorPtr(lua_State* state, int32_t index) {
	if (!LUA->IsType(index, metatype)) {
		luaL_typerror(LUA->GetLuaState(), index, metaname);
	}

	return LUA->GetUserType<VectorPtr>(index, metatype);
}

reactphysics3d::Vector3* vox_lua_getVector(lua_State* state, int32_t index) {
	if (!LUA->IsType(index, metatype)) {
		luaL_typerror(LUA->GetLuaState(), index, metaname);
	}

	auto vecPtr = LUA->GetUserType<VectorPtr>(index, metatype);

	if (vecPtr != NULL) {
		return vecPtr->get();
	}

	return NULL;
}

int vox_lua_vector_gc(lua_State* state) {
	auto vecptr = vox_lua_getVectorPtr(state, 1);

	delete vecptr;
	LUA->SetUserType(1, nullptr);

	return 0;
}

int vox_lua_vector_tostring(lua_State* state) {
	auto vecptr = vox_lua_getVector(state, 1);

	lua_pushfstring(state, tostring_format, metaname, vecptr);

	return 1;
}

int vox_lua_vector_ctor(lua_State* state) {
	double x = luaL_checknumber(state, 1);
	double y = luaL_checknumber(state, 2);
	double z = luaL_checknumber(state, 3);

	auto vec = new reactphysics3d::Vector3(x, y, z);

	vox_lua_pushVector(state, vec);

	return 1;
}

void setupLuaAdvancedVectors(lua_State * state) {
	metatype = LUA->CreateMetaTable(metaname);

	lua_pushcfunction(state, vox_lua_vector_gc);
	lua_setfield(state, -2, "__gc");

	lua_pushcfunction(state, vox_lua_vector_tostring);
	lua_setfield(state, -2, "__tostring");

	LUA->Pop(1);

	lua_pushcfunction(state, vox_lua_vector_ctor);
	lua_setglobal(state, metaname);
}
