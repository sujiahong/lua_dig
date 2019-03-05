#define lua_c

#include "lprefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#if !defined(LUA_PROGNAME)
#define LUA_PROGNAME "lua"
#endif

#if !defined(LUA_INIT_VAR)
#define LUA_INIT_VAR "LUA_INIT"
#endif

#define LUA_INITVARVERSION LUA_INIT_VAR LUA_VERSUFFIX
static lua_State* globalL = NULL;
static const char* progname = LUA_PROGNAME;

static void lstop (lua_State* L, lua_Debug* ar){
    (void)ar;
    lua_sethook(L, NULL, 0, 0);
    luaL_error(L, "interrupted!");
}

static void laction(int i) {
    signal(i, SIG_DFL);
    lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

static void print_usage (const char* badoption){
    lua_writestringerror("%s: ", progname);
    if (badoption[1] == 'e' || badoption[1] == 'l')
        lua_writestringerror("'%s' needs argument\n", badoption);
    else
        lua_writestringerror("unrecognized option '%s'\n", badoption);
    
    lua_writestringerror(
        "usage: %s [options] [script [args]]\n"
        "Available options are:\n"
        "  -e stat  execute string 'stat'\n"
        "  -i       enter interactive mode after executing 'script'\n"
        "  -l name  require library 'name' into global 'name'\n"
        "  -v       show version information\n"
        "  -E       ignore environment variables\n"
        "  --       stop handling options\n"
        "  -        stop handling options and execute stdin\n"
        , progname);
}

static void l_message(const char* pname, const char* msg){
    if (pname)
        lua_writestringerror("%s: ", pname);
    lua_writestringerror("%s\n", msg);
}

static int report(lua_State* L, int status) {
    if (status != LUA_OK){
        const char* msg = lua_tostring(L, -1);
        l_message(progname, msg);
        lua_pop(L, 1);
    }
    return status;
}

static int msghandler (lua_State* L){
    const char* msg = lua_tostring(L, 1);
    if (msg == NULL){
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
            return 1;
        else
            msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

static int docall(lua_State* L, int narg, int nres){
    int status;
    int base = lua_gettop(L) - narg;
    lua_pushcfunction(L, msghandler);
    lua_insert(L, base);
    globalL = L;
    signal(SIGINT, laction);
    status = lua_pcall(L, narg, nres, base);
    signal(SIGINT, SIG_DFL);
    lua_remove(L, base);
    return status;
}

static void print_version(void){
    lua_writestring(LUA_COPYRIGHT, strlen(LUA_COPYRIGHT));
    lua_writeline();
}

static void createargtable (lua_State* L, char** argv, int argc, int script) {
    int i, narg;
    if (script == argc)
        script = 0;
    narg = argc - (script + 1);
    lua_createtable(L, narg, script + 1);
    for (i = 0; i < argc; ++i){
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - script);
    }
    lua_setglobal(L, "arg");
}




int main(int argc, char** argv){
    int status, result;
    lua_State* L = luaL_newstate();
    if (L == NULL){
        l_message(argv[0], "cannot create: not enough memory");
        return EXIT_FAILURE;
    }
    lua_pushcfunction(L, &pmain);
    lua_pushinteger(L, argc);
    lua_pushlightuserdata(L, argv);
    status = lua_pcall(L, 2, 1, 0);
    result = lua_toboolean(L, -1);
    report(L, status);
    lua_close(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
}