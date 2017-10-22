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

/*
typedef std::shared_ptr<reactphysics3d::Vector3> VectorPtr;

void setupLuaAdvancedVectors(lua_State* state);

VectorPtr * vox_lua_pushVector(lua_State * state, reactphysics3d::Vector3 * vec);
reactphysics3d::Vector3* vox_lua_getVector(lua_State* state, int32_t index);
*/
