#pragma once

#ifndef GLUA_HEADER_MEMES
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

#endif

#include <memory>

#include "btBulletDynamicsCommon.h"

typedef std::shared_ptr<btVector3> VectorPtr;

void setupLuaAdvancedVectors(lua_State* state);

btVector3 luaL_checkbtVector3(lua_State* state, int loc);
void luaL_pushbtVector3(lua_State* state, btVector3 vecSrc);
