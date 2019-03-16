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

static int dochunk(lua_State* L, int status){
    if (status == LUA_OK)
        status = docall(L, 0, 0);
    return report(L, status);
}

static int dofile(lua_State* L, const char* name){
    return dochunk(L, luaL_loadfile(L, name));
}

static int dostring(lua_State* L, const char* s, const char* name){
    return dochunk(L, luaL_loadbuffer(L, s, strlen(s), name));
}

static int dolibrary(lua_State* L, const char* name){
    int status;
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    status = docall(L, 1, 1);
    if (status == LUA_OK)
        lua_setglobal(L, name);
    return report(L, status);
}

static int pushargs(lua_State* L){
    int i, n;
    if (lua_getglobal(L, "arg") != LUA_TTABLE)
        luaL_error(L, "'arg' is not a table");
    n = (int)luaL_len(L, -1);
    luaL_checkstack(L, n+3, "too many arguments to script");
    for (i = 1; i <= n; ++i){
        lua_rawgeti(L, -i, i);
    }
    lua_remove(L, -i);
    return n;
}

static int handle_script(lua_State* L, char** argv){
    int status;
    const char* fname = argv[0];
    if (strcmp(fname, "-") == 0 && strcmp(argv[-1], "--") != 0){
        fname = NULL;
    }
    status = luaL_loadfile(L, fname);
    if(status == LUA_OK){
        int n = pushargs(L);
        status = docall(L, n, LUA_MULTRET);
    }
    return report(L, status);
}

#define has_error 1
#define has_i 2
#define has_v 4
#define has_e 8
#define has_E 16

static int collectargs(char** argv, int* first){
    int args = 0;
    int i;
    for (i = 1; argv[i]!= NULL; ++i){
        *first = i;
        if (argv[i][0] != '-'){
            return args;
        }
        switch (argv[i][1])
        {
            case '-':
                if (argv[i][2] != '\0')
                    return has_error;
                *first = i + 1;
                return args;
            case '\0':
                return args;
            case 'E':
                if (argv[i][2] != '\0')
                    return has_error;
                args |= has_E;
                break;
            case 'i':
                args |= has_i;
            case 'v':
                if (argv[i][2] != '\0')
                    return has_error;
                args |= has_v;
                break;
            case 'e':
                args |= has_e;
            case 'l':
                if (argv[i][2] == '\0'){
                    i++;
                    if (argv[i] == NULL || argv[i][0] == '-')
                        return has_error;
                }
                break;
            default:
                return has_error;
        }
    }
    *first = i;
    return args;
}

static int runargs(lua_State* L, char** argv, int n){
    int i;
    for (i = 1; i < n; ++i){
        int option = argv[i][1];
        lua_assert(argv[i][0] == '-');
        if (option == 'e' || option == 'l'){
            int status;
            const char* extra = argv[i] + 2;
            if (*extra == '\0')
                extra = argv[++i];
            lua_assert(extra != NULL);
            status = (option == 'e')? dostring(L, extra, "=(command line)")
                                    : dolibrary(L, extra);
            if (status != LUA_OK)
                return 0;
        }
    }
    return 1;
}

static int handle_luainit(lua_State* L){
    const char* name = "=" LUA_INITVARVERSION;
    const char* init = getenv(name);
    if (init == NULL){
        name = "=" LUA_INIT_VAR;
        init = getenv(name + 1);
    }
    if (init == NULL)
        return LUA_OK;
    else if(init[0] == '@')
        return dofile(L, init+1);
    else
        return dostring(L, init, name);
}

#if !defined(LUA_PROMPT)
#define LUA_PROMPT  "> "
#define LUA_PROMPT2 ">> "
#endif

#if !defined(LUA_MAXNPUT)
#define LUA_MAXINPUT  512
#endif

#if !defined(lua_stdin_is_tty)
#if defined(LUA_USE_POSIX)
#include <unistd.h>
#define lua_stdin_is_tty() isatty(0)
#elif defined(LUA_USE_WINDOWS)
#include <io.h>
#include <windows.h>
#define lua_stdin_is_tty() _isatty(_fileno(stdin))

#else
#define lua_stdin_is_tty() 1
#endif

#endif

#if !defined(lua_readline)
#if defined(LUA_USE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#define lua_initreadline(L) ((void)L, rl_readline_name="lua")
#define lua_readline(L,b,p) ((void)L, ((b)=readline(p)) != NULL)
#define lua_saveline(L,line) ((void)L, add_history(line))
#define lua_freeline(L,b) ((void)L, free(b))

#else
#define lua_initreadline(L) ((void)L)
#define lua_readline(L,b,p) ((void)L, fputs(p, stdout), fflush(stdout), \
                            fgets(b, LUA_MAXINPUT, stdin) != NULL)
#define lua_saveline(L,line) {(void)L; (void)line;}
#define lua_freeline(L,b) {(void)L; (void)b;}
#endif
#endif

static const char* get_prompt(lua_State* L, int firstline){
    const char* p;
    lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2");
    p = lua_tostring(L, -1);
    if (p == NULL)
        p = (firstline ? LUA_PROMPT : LUA_PROMPT2);
    return p;
}

#define EOFMASK "<eof>"
#define marklen (sizeof(EOFMASK)/sizeof(char) - 1)

static int incomplete(lua_State* L, int status){
    if (status == LUA_ERRSYNTAX){
        size_t lmsg;
        const char* msg = lua_tolstring(L, -1, &lmsg);
        if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMASK) == 0){
            lua_pop(L, 1);
            return 1;
        }
    }
    return 0;
}

static int pushline(lua_State* L, int firstline){
    char buffer[LUA_MAXINPUT];
    char* b = buffer;
    size_t l;
    const char* prmt = get_prompt(L, firstline);
    int readstatus = lua_readline(L, b, prmt);
    if (readstatus == 0){
        return 0;
    }
    lua_pop(L, 1);
    l = strlen(b);
    if (l > 0 && b[l-1] == '\n'){
        b[--l] = '\0';
    }
    if (firstline && b[0] == '=')
        lua_pushfstring(L, "return %s", b + 1);
    else
        lua_pushlstring(L, b, l);
    lua_pushlstring(L, b, l);
    lua_freeline(L, b);
    return 1;
}

static int addreturn(lua_State* L){
    const char* line = lua_tostring(L, -1);
    const char* retline = lua_pushfstring(L, "return %s;", line);
    int status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
    if (status == LUA_OK){
        lua_remove(L, -2);
        if (line[0] != '\0'){
            lua_saveline(L, line);
        }
    }else {
        lua_pop(L, 2);
    }
    return status;
}

static int multiline(lua_State* L){
    for (;;){
        size_t len;
        const char* line = lua_tolstring(L, 1, &len);
        int status = luaL_loadbuffer(L, line, len, "=stdin");
        if (!incomplete(L, status) || !pushline(L, 0)){
            lua_saveline(L, line);
            return status;
        }
        lua_pushliteral(L, "\n");
        lua_insert(L, -2);
        lua_concat(L, 3);
    }
}

static loadline(lua_State* L){
    int status;
    lua_settop(L, 0);
    if (!pushline(L, 1)){
        return -1;
    }
    if ((status = addreturn(L)) != LUA_OK)
        status = multiline(L);
    lua_remove(L, 1);
    lua_assert(lua_gettop(L) == 1);
    return status;
}

static void l_print(lua_State* L){
    int n = lua_gettop(L);
    if (n > 0){
        luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
        lua_getglobal(L, "print");
        lua_insert(L, 1);
        if (lua_pcall(L, n, 0, 0) != LUA_OK)
            l_message(progname, lua_pushfstring(L, "error calling 'print' (%s)", lua_tostring(L, -1)));
    }
}

static void doREPL(lua_State* L){
    int status;
    const char* oldprogname = progname;
    progname = NULL;
    lua_initreadline(L);
    while((status = loadline(L)) != -1){
        if (status == LUA_OK)
            status = docall(L, 0, LUA_MULTRET);
            if (status == LUA_OK)
                l_print(L);
            else
                report(L, status);
    }
    lua_settop(L, 0);
    lua_writeline();
    progname = oldprogname;
}

static int pmain(lua_State* L){
    int argc = (int)lua_tointeger(L, 1);
    char** argv = (char**)lua_touserdata(L, 2);
    int script;
    int args = collectargs(argv, &script);
    luaL_checkversion(L);
    if (argv[0] && argv[0][0])
        progname = argv[0];
    if (args == has_error){
        print_usage(argv[script]);
        return 0;
    }
    if (args & has_v)
        print_version();
    if (args & has_E){
        lua_pushboolean(L, 1);
        lua_setfield(L, LUA_REGISTERYINDEX, "LUA_NOENV");
    }
    luaL_openlibs(L);
    createargtable(L, argv, argc, script);
    lua_gc(L, LUA_GCGEN, 0, 0);
    if (!(args & has_E)){
        if (handle_luainit(L) != LUA_OK)
            return 0;
    }
    if (!runargs(L, argv, script))
        return 0;
    if (script < argc && handle_script(L, argv + script) != LUA_OK)
        return 0;
    if (args & has_i)
        doREPL(L);
    else if(script == argc && !(args & (has_e | has_v))){
        if (lua_stdin_is_tty()){
            print_version();
            doREPL(L);
        }else{
            dofile(L, NULL);
        }
    }
    lua_pushboolean(L, 1);
    return 1;
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