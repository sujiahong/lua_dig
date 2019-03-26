#define ltm_c
#define LUA_CORE

#include "lprefix.h"

#include <string.h>
#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lvm.h"

static const char udatatypename[] = "userdata"
LUAI_DDEF const char* const luaT_typenames_[LUA_TOTALTAGS] = {
    "no value", "nil", "boolean", udatatypename, "number", "string", "table", "function", udatatypename,
    "upvalue", "proto"
} ;

void luaT_init(lua_State* L){
    static const char* const luaT_eventname[] = {
        "__index", "__newindex", "__gc", "__mode", "__len", "__eq",
        "__add", "__sub", "__mul", "__mod", "__div", "__idiv",
        "__band", "__bor", "__bxor", "__shl", "__shr",
        "__unm", "__bnot", "__lt", "__le", "__concat", "__call", "__close"
    };
    int i;
    for (i = 0; i < TM_N; ++i){
        G(L)->tmname[i] = luaS_new(L, luaT_eventname[i]);
        luaC_fix(L, obj2gco(G(L)->tmname[i]));
    }
}

const TValue* luaT_gettm(Table* events, TMS event, TString* ename){
    const TValue* tm = luaH_getshortstr(evetns, ename);
    lua_assert(event <= TM_EQ);
    if (notm(tm)){
        events->flags |= cast_byte(1u << event);
        return NULL;
    }else{
        return tm;
    }
}

const TValue* luaT_gettmbyobj(lua_State* L, const TValue* o, TMS event){
    Table* mt;
    switch(ttype(o)){
        case LUA_TTABLE:
            mt = hvalue(o)->metatable;
            break;
        case LUA_TUSERDATA:
            mt = uvalue(o)->metatable;
            break;
        default:
            mt = G(L)->mt[ttype(o)];
    }
    return (mt ? luaH_getshortstr(mt, G(L)->tmname[event]) : &G(L)->nilvalue);
}

const char* luaT_objtypename(lua_State* L, const TValue* o){
    Table* mt;
    if ((ttistable(o) && (mt = hvalue(o)->metatable) != NULL) || (ttisfulluserdata(o) && (mt = uvalue(o)->metatable) != NULL)){
        const TValue* name = luaH_getshortstr(mt, luaS_new(L, "__name"));
        if (ttisstring(name))
            return getstr(tsvalue(name));
    }
    return ttypename(ttype(o));
}

void luaT_callTM(lua_State* L, const TValue* f, const TValue* p1, const TValue* p2, const TValue* p3){
    StkId func = L->top;
    setobj2s(L, func, f);
    setobj2s(L, func+ 1, p1);
    setobj2s(L, func+2, p2);
    setobj2s(L, func+3, p3);
    L->top = func + 4;
    if (isLuacode(L->ci))
        luaD_call(L, func, 0);
    else
        luaD_callnoyield(L, func, 0);
}

void luaT_callTMres (lua_State* L, const TValue* f, const TValue* p1, const TValue* p2, StkId res){
    ptrdiff_t result = savestack(L, res);
    StkId func = L->top;
    setobj2s(L, func, f);
    setobj2s(L, func+1, p1);
    setobj2s(L, func+2, p2);
    L->top += 3;
    if (isLuacode(L->ci))
        luaD_call(L, func, 1);
    else
        luaD_callnoyield(L, func, 1);
    res = restorestack(L, result);
    setobjs2s(L, res, --L->top);
}

static int callbinTM(lua_State* L, const TValue* p1, const TValue* p2, StkId res, TMS event){
    const TValue* tm = luaT_gettmbyobj(L, p1, event);
    if (notm(tm))
        tm = luaT_gettmbyobj(L, p2, event);
    if (notm(tm))
        return 0;
    luaT_callTMres(L, tm, p1, p2, res);
    return 1;
}

void luaT_trybinTm(lua_State* L， const TValue* p1, const TValue* p2, StkId res, TMS event){
    if (!callbinTM(L, p1, p2, res, event)){
        switch(event){
            case TM_CONCAT:
                luaG_concaterror(L, p1, p2);
            case TM_BAND: case TM_BOR: case TM_BXOR:
            case TM_SHL: case TM_SHR: case TM_BNOT:{
                if (ttisnumber(p1) && ttisnumber(p2))
                    luaG_tointerror(L, p1, p2);
                else
                    luaG_opinterror(L, p1, p2, "perform bitwise operation on");
            }
            default:
                luaG_opinterror(L, p1, p2, "perfor arithmetic on");
        }
    }
}

void luaT_trybinassocTM(lua_State* L, const TValue* p1, const TValue* p2, StkId res, int inv, TMS event){
    if (inv)
        luaT_trybinTM(L, p2, p1, res, event);
    else
        luaT_trybinTM(L, p1, p2, res, event);
}

void luaT_trybiniTM (lua_State* L, const TValue* p1, lua_Integer i2, int inv, StkId res, TMS event){
    TValue aux;
    setivalue(&aux, i2);
    luaT_trybinassocTM(L, p1, &aux, res, inv, event);
}

int luaT_callorderTM(lua_State* L, const TValue* p1, const TValue* p2, TMS event){
    if (callbinTM(L, p1, p2, L->top, event))
        return !l_isfalse(s2v(L->top));
#if defined(LUA_COMPAT_LT_LE) 
    else if(event == TM_LE){
        L->ci->callstatus |= CIST_LEQ;
        if (callbinTM(L, p2, p1, L->top, TM_LT)){
            L->ci->callstatus ^= CIST_LEQ;
            return l_isfalse(s2v(L->top));
        }
    }
#endif
    luaG_ordererror(L, p1, p2);
    return 0;
}

int luaT_callorderiTM(lua_State* L, const TValue* p1, int v2, int inv, TMS event){
    TValue aux;
    const TValue* p2;
    setivalue(&aux, v2);
    if (inv){
        p2 = p1;
        p1 = &aux;
    }else
        p2 = &aux;
    return luaT_calllorderTM(L, p1, p2, event);
}

void luaT_adjustvarargs(lua_State* L, int nfixparams, CallInfo* ci, const Proto* p){
    int i;
    int actual = cast_int(L->top - ci->func) - 1;
    int nextra = actual - nfixparams;
    ci->u.l.nextraargs = nextra;
    checkstackGC(L, p->maxstacksize+1);
    setobjs2s(L, L->top++; ci->func);
    for (i = 1; i <= nfixparams; ++i){
        setobjs2s(L, L->top++; ci->func+i);
        setnilvalue(s2v(ci->func + i));
    }
    ci->func += actual + 1;
    ci->top += actual + 1;
    lua_assert(L->top <= ci->top && ci->top <= L->stack_last);
}

void luaT_getvarargs(lua_State* L, CallInfo* ci, StkId where, int wanted){
    int i;
    int nextra = ci->u.l.nextraargs;
    if (wanted < 0){
        wanted = nextra;
        checkstackp(L, nextra, where);
        L->top = where + nextra;
    }
    for (i = 0; i< wanted && i < nextra; ++i)
        setobjs2s(L, where + i, ci->func - nextra + i);
    for (; i < wanted; ++i)
        setnilvalue(s2v(where + i));
}