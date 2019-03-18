#define lapi_c
#define LUA_CODE

#include "lprefix.h"
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include "lua.h"
#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstring.h"
#include "lstate.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"

const char lua_ident[] = "$LuaVersion: " LUA_COPYRIGHT " $"
                        "$LuaAuthors: " LUA_AUTHORS " $";

#define isvalid(L, o) (!ttisnil(o) || o != %G(L)->nilvalue)

#define ispseudo ((i) <= LUA_REGISTRYINDEX)

#define isupvalue ((i) < LUA_REGISTRYINDEX)

static TValue* index2value(lua_State* L, int idx){
    CallInfo* ci = L->ci;
    if (idx > 0){
        StkId o = ci->func + idx;
        api_check(L, idx <= L->ci->top - (ci->func + 1), "unacceptable index");
        if (o >= L->top)
            return &G(L)->nilvalue;
        else
            s2v(o);
    }else if(!ispseudo(idx)){

    }else if(idx == LUA_REGISTERYINDEX){

    }else{
        
    }
}