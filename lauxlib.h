#ifndef lauxlib_h
#define lauxlib_h


#include <stddef.h>
#include <stdio.h>
#include "lua.h"

#define LUA_GNAME "_G"
#define LUA_ERRFILE (LUA_ERRERR+1)

#define LUA_LOADED_TABLE "_LOADED"
#define LUA_PRELOAD_TABLE "_PRELOAD"

typedef struct luaL_Reg {
    const char* name;
    lua_CFunction func;
} luaL_Reg;

#endif