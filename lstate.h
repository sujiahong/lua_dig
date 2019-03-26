#ifndef lstate_h
#define lstate_h

#include "lua.h"
#include "lobject.h"
#include "ltm.h"
#include "lzio.h"

struct lua_longjmp;
#if !defined(l_signalT)
#include <signal.h>
#define l_signalT sig_atomic_t
#endif
#define EXTRA_STACK 5
#define BASIC_STACK_SIZE (2*LUA_MINSTACK)
#define KGC_INC 0
#define KGC_GEN 1

typedef struct stringtable{
    TString** hash;
    int nuse;
    int size;
} stringtable;

typedef struct CallInfo {
    StkId func;
    StkId top;
    struct CallInfo* previous, *next;
    union{
        struct {
            const Instruction* savedpc;
            l_signalT trap;
            int nextraargs;
        } l;
        struct {
            lua_KFunction k;
            ptrdiff_t old_errfunc;
            lua_KContext ctx;
        } c;
    } u;
    union {
        int funcidx;
        int nyield;
        struct {
            unsigned short ftransfer;
            unsigned short ntransfer;
        } transferinfo;
    } u2;
    short nresults;
    unsigned short callstatus;
} CallInfo;

#define CIST_OAH (1 << 0)
#define CIST_C (1 << 1)
#define CIST_HOOKED (1 << 2)
#define CIST_YPCALL (1 << 3)
#define CIST_TAIL (1 << 4)
#define CIST_HOOKYIELD (1 << 5)
#define CIST_FIN (1 << 6)
#define CIST_TRAN (1 << 7)
#if defined(LUA_COMPAT_LT_LE)
#define CIST_LEQ (1 << 8)
#endif

#define isLua(ci) (!((ci)->callstatus & CIST_C))
#define isLuacode(ci) (!((ci)->callstatus & (CIST_C | CIST_HOOKED)))
#define setoah(st,v) ((st) = ((st) & ~CIST_OAH) | (v))
#define getoah(st) ((st) & CIST_OAH)

typedef struct global_State {
    lua_Alloc frealloc;
    void* ud;
    l_mem totalbytes;
    l_mem GCdebt;
    stringtable strt;
    TValue l_registry;
    TValue nilvalue;
    unsigned int seed;
    lu_byte currentwhite;
    lu_byte gcstate;
    lu_byte gckind;
    lu_byte genminormul;
    lu_byte genmajormul;
    lu_byte gcrunning;
    lu_byte gcemergency;
    lu_byte gcpause;
    lu_byte gcstepmul;
    lu_byte gcstepsize;
    GCObject* allgc;
    GCObject** sweepgc;
    GCObject* finobj;
    GCObject* gray;
    GCObject* grayagain;
    GCObject* weak;
    GCObject* ephemeron;
    GCObject* allweak;
    GCObject* tobefnz;
    GCObject* fixedgc;
    GCObject* survival;
    GCObject* old;
    GCObject* reallyold;
    GCObject* finobjsur;
    GCObject* finobjold;
    GCObject* finobjrold;
    struct lua_State* twups;
    lua_CFunction panic;
    struct lua_State* mainthread;
    TString* memerrmsg;
    TString* tmname[TM_N];
    struct Table* mt[LUA_NUMTAGS];
    TString* strcache[STRCACHE_N][STRCACHE_M];
} global_State;

typedef struct lua_State{
    CommonHeader;
    unsigned short nci;
    lu_byte status;
    StkId top;
    global_State* l_G;
    CallInfo* ci;
    const Instruction* oldpc;
    StkId stack_last;
    StkId stack;
    UpVal* openupval;
    GCObject* gclist;
    struct lua_State twups;
    struct lua_longjmp* errorJmp;
    CallInfo base_ci;
    volatile lua_Hook hook;
    ptrdiff_t errfunc;
    int stacksize;
    int basehookcount;
    int hookcount;
    unsigned short nny;
    unsigned short nCcalls;
    l_signalT hookmask;
    lu_byte allowhook;
} lua_State;
#define G(L) (L->l_G)

union GCUnion {
    GCObject gc;
    struct TString ts;
    struct Udata u;
    union Closure cl;
    struct Table h;
    struct Proto p;
    struct lua_State th;
    struct UpVal upv;
};

#define cast_u(o) cast(union GCUnion*, (o))
#define gco2ts(o) check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))
#define gco2u(o)
#define gco2lcl(o)
#define gco2ccl(o)
#define gco2cl(o)
#define gco2t(o)
#define gco2p(o)
#define gco2th(o)
#define gco2upv(o)

#define obj2gco(v) check_exp((v)->tt >= LUA_TSTRING, &(cast_u(v)->gc))
#define gettotalbytes(g) cast(lu_mem, (g)->totalbytes + (g)->GCdebt)

LUAI_FUNC void luaE_setdebt(global_State* g, l_mem debt);
LUAI_FUNC void luaE_freethread(lua_State* L, lua_State* L1);
LUAI_FUNC CallInfo* luaE_extendCI(lua_State* L);
LUAI_FUNC void luaE_freeCI(lua_State* L);
LUAI_FUNC void luaE_shrinkCI(lua_State* L);
LUAI_FUNC void luaE_incCcalls(lua_State* L);


#endif