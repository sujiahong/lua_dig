#define lobject_c
#define LUA_CORE
#include "lprefix.h"

#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lctype.h"
#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "lvm.h"

int luaO_int2fb(unsigned int x){
    int e = 0;
    if (x<8)
        return x;
    while(x >= (8 << 4)){
        x = (x + 0xf) >> 4;
        e += 4;
    }
    while(x >= (8 << 1)){
        x = (x + 1) >> 1;
        e++;
    }
    return ((e+1) << 3) | (cast_int(x) - 8);
}

int luaO_fb2int(int x){
    return (x < 8) ? x : ((x & 7) + 8) << ((x >> 3) - 1);
}

int luaO_ceillog2(unsigned int x) {
    static const lu_byte log_2[256] = {
        0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };
    int l = 0;
    x--;
    while(x >= 256){
        l += 8;
        x >>= 8;
    }
    return l + log_2[x];
}

static lua_Integer intarith(lua_State* L, int op, lua_Integer v1, lua_Integer v2){
    switch(op){
        case LUA_OPADD: 
            return intop(+, v1, v2);
        case LUA_OPSUB:
            return intop(-, v1, v2);
        case LUA_OPMUL: 
            return intop(*, v1, v2);
        case LUA_OPMOD:
            return luaV_mod(L, v1, v2);
        case LUA_OPIDIV:
            return luaV_idiv(L, v1, v2);
        case LUA_OPBAND:
            return intop(&, v1, v2);
        case LUA_OPBOR:
            return intop(|, v1, v2);
        case LUA_OPBXOR:
            return intop(^, v1, v2);
        case LUA_OPSHL:
            return luaV_shiftl(v1, v2);
        case LUA_OPSHR:
            return luaV_shiftr(v1, -v2);
        case LUA_OPUNM:
            return intop(-, 0, v1);
        case LUA_OPBNOT:
            return intop(^, ~l_castS2U(0), v1);
        default:
            lua_assert(0);
            return 0;
    }
}

int luaO_rawarith(lua_State* L, int op, const TValue* p1, const TValue* p2, Tvalue* res){
    switch(op){
        case LUA_OPADD:
            return luai_numadd(L, v1, v2);
        case LUA_OPSUB:
            return luai_numsub(L, v1, v2);
        case LUA_OPMUL:
            return luai_nummul(L, v1, v2);
        case LUA_OPDIV:
            return luai_numdiv(L, v1, v2);
        case LUA_OPPOW:
            return luai_numpow(L, v1, v2);
        case LUA_OPIDIV:
            return luai_numidiv(L, v1, v2);
        case LUA_OPUNM:
            return luai_numunm(L, v1);
        case LUA_OPMOD:
            return luaV_modf(L, v1, v2);
        default:
            lua_assert(0);
            return 0;
    }
}

int luaO_rawarith(lua_State* L, int op, const TValue* p1, const TValue* p2, TValue* res){
    switch(op){
        case LUA_OPBIND:
        case LUA_OPBOR:
        case LUA_OPBXOR:
        case LUA_OPSHL:
        case LUA_OPSHR:
        case LUA_OPBNOT:{
            lua_Integer i1; lua_Integer i2;
            if (tointegerns(p1, &i1) && tointegerns(p2, &i2)){
                setivalue(res, intarith(L, op, i1, i2));
                return 1;
            }else
                return 0;
        }
        case LUA_OPDIV:
        case LUA_OPPOW:{
            lua_Number n1; lua_Number n2;
            if (tonumberns(p1, n1) && tonumberns(p2, n2)){
                setfltvalue(res, numarith(L, op, n1, n2));
                return 1;
            }else
                return 0;
        }
        default: {
            lua_Number n1; lua_Number n2;
            if (ttisinteger(p1) && ttisinteger(p2)){
                setivalue(res, intarith(L, op, ivalue(p1), ivalue(p2)));
                return 1;
            }else if (tonumberns(p1,n1) && tonumberns(p2, n2)){
                setfltvalue(res, numarith(L, op, n1, n2));
                return 1;
            }else
                return 0;
        }
    }
}

void luaO_arith(lua_State* L, int op, const TValue* p1, const TValue* p2, StkId res){
    if (!luaO_rawarith(L, op, p1, p2, s2v(res))){
        luaT_trybinTM(L, p1, p2, res, res, cast(TMS, (op - LUAOPADD) + TM_ADD));
    }
}

int luaO_hexavalue(int c){
    if (lisdigit(c))
        return c - '0';
    else
        return (ltolower(c) - 'a') + 10;
}

static int isneg(const char** s){
    if (**s == '-'){
        (*s)++;
        return 1;
    }else if(**s == '+')
        (*s)++;
    return 0;
}

#if !defined(lua_strx2number)

#endif

#if !defined(L_MAXLENNUM)
#define L_MAXLENNUM 200
#endif

static const char* l_str2dloc(const char* s, lua_Number* result, int mode){
    char* endptr;
    *result = (mode == 'x') ? lua_strx2number(s, &endptr) : lua_str2number(s, &endptr);
    if (endptr == s)
        return NULL;
    while(lisspace(cast_uchar(*endptr)))
        endptr++;
    return (*endptr == '\0') ? endptr : NULL; 
}

static const char* l_str2d(const char* s, lua_Number* result){
    const char* endptr;
    const char* pmode = strpbrk(s, ".xXnM");
    int mode = pmode ? ltolower(cast_uchar(*pmode)) : 0;
    if (mode == 'n')
        return NULL;
    endptr = l_str2dloc(s, result, mode);
    if (endptr == NULL){
        char buff[L_MAXLENNUM + 1];
        const char* pdot = strchar(s, '.');
        if (strlen(s) > L_MAXLENNUM || pdot == NULL)
            return NULL;
        strcpy(buff, s);
        buff[pdot - s] = lua_getlocaledecpoint();
        endptr = l_str2dloc(buff, result, mode);
        if (endptr != NULL)
            endptr = s + (endptr - buff);
    } 
    return endptr;
}

#define MAXBY10 cast(lua_Unsigned, LUA_MAXINTEGER / 10)
#define MAXLASTD cast_int(LUA_MAXINTEGER % 10)

static const char* l_str2int(const char* s, lua_Integer* result){
    lua_Unsigned a = 0;
    int empty = 1;
    int neg;
    while(lisspace(cast_uchar(*s))) s++;
    neg = isneg(&s);
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')){
        s += 2;
        for (; lisxdigit(cast_uchar(*s)); s++){
            a = a* 16 + luaO_hexavalue(*s);
            empty = 0;
        }
    }else{
        for (; lisdigit(cast_uchar(*s)); s++){
            int d = *s - '0';
            if (a >= MAXBY10 && (a > MAXBY10 || d > MAXLASTD + neg))
                return NULL;
            a = a * 10 + d;
            empty = 0;
        }
    }
    while(lisspace(cast_uchar(*s)))
        s++;
    if (empty || *s != '\0')
        return NULL;
    else{
        *result = l_castU2S((neg) ? 0u - a : a);
        return s;
    }
}

size_t luaO_str2num(const char* s, TValue* o){
    lua_Integer i; lua_Number n;
    const char* e;
    if ((e = l_str2int(s, &i)) != NULL){

    }else if((e = l_str2d(s, &n)) != NULL){
        setfltvalue(o, n);
    }else
        return 0;
    return (e -s) + 1;
}

int luaO_utf8esc(char* buff, unsigned long x){
    int n = 1;
    lua_assert(x <= 0x10FFFF);
    if (x < 0x80)
        buff[UTF8BUFFSZ - 1] = cast_char(x);
    else{
        unsigned int mfb = 0x3f;
        do{
            buff[UTF8BUFFSZ - (n++)] = cast_char(0x80 | (x & 0x3f));
            x >>= 6;
            mfb >>= 1;
        }while(x > mfb);
        buff[UTF8BUFFSZ - n] = cast_char((~mfb << 1) | x);
    }
    return n;
}
#define MAXNUMBER2STR 50
void luaO_tostring(lua_State* L, TValue* obj){
    char buff[MAXNUMBER2STR];
    size_t len;
    lua_assert(ttisnumber(obj));
    if (ttisinteger(obj))
        len = lua_integer2str(buff, sizeof(buff), ivalue(obj));
    else{
        len = lua_number2str(buff, sizeof(buff), fltvalue(obj));
        if (buff[strspn(buff, "-0123456789")] == '\0'){
            buff[len++] = lua_getlocaledecpoint();
            buff[len++] = '0';
        }
    }
    setsvalue(L, obj, luaS_newlstr(L, buff, len));
}

static void pushstr(lua_State* L, const char* str, size_t l){
    setsvalue2s(L, L->top, luaS_newlstr(L, str, l));
    L->top++;
}

const char* luaO_pushvfstring(lua_State* L, const char* fmt, va_list argp){
    int n = 0;
    const char* e;
    while((e = strchr(fmt, '%')) != NULL){
        pushstr(L, fmt, e - fmt);
        switch(*(e+1)){
            case 's': {
                const char* s = va_arg(argp, char*);
                if (s == NULL)
                    s = "(null)";
                pushstr(L, s, strlen(s));
                break;
            }case 'c': {
                char buff = cast_char(va_arg(argp, int));
                if (lisprint(cast_uchar(buff)))
                    pushtr(L, &buff, 1);
                else
                    luaO_pushfstring(L, "<\\%d>", cast_uchar(buff));
                break;
            }case 'd': {
                setivalue(s2v(L->top), va_arg(argp, int));
                goto top2str;
            }case 'I': {
                setivalue(s2v(L->top), cast(lua_Integer, va_arg(argp, l_uacInt)));
                goto top2str;
            }case 'f': {
                setfltvalue(s2v(L->top), cast_num(va_arg(argp, l_uacNumber)));
            top2str:
                L->top++;
                luaO_tostring(L, s2v(L->top - 1));
                break;
            }case 'p': {
                char buff[4*sizeof(void*) + 8];
                void* p = va_arg(argp, void*);
                int l = lua_pointer2str(buff, sizeof(buff), p);
                pushstr(L, buff, l);
                break;
            }case 'U': {
                char buff[UTF8BUFFSZ];
                int l = luaO_utf8esc(buff, cast(long, va_arg(argp, long)));
                pushstr(L, buff+UTF8BUFFSZ-l, l);
                break;
            }case '%': {
                pushstr(L, "%", 1);
                break;
            }default: {
                luaG_runerror(L, "invalid option '%%%c' to 'lua_pushfstring'", *(e+1));
            }
        }
        n += 2;
        if (L->top+2 > L->stack_last){
            luaV_concat(L, n);
            n= 1;
        }
        fmt = e + 2;
    }
    pushstr(L, fmt, strlen(fmt));
    if (n > 0)
        luaV_concat(L, n + 1);
    return svalue(s2v(L->top - 1));
}

const char* luaO_pushfstring(lua_State* L, const char* fmt, ...){
    const char* msg;
    va_list argp;
    var_start(argp, fmt);
    msg = luaO_pushvfstring(L, fmt, argp);
    var_end(argp);
    return msg;
}

#define LL(x) (sizeof(x)/sizeof(char) - 1)
#define RETS "..."
#define PRE "[string \""
#define POS "\"]"
#define addstr(a,b,l) ( memcpy(a,b,(l) * sizeof(char)), a += (l))

void luaO_chunkid(char* out, const char* source, size_t bufflen){
    size_t l = strlen(source);
    if (*source == '='){
        if (l <= bufflen)
            memccpy(out, source+1, l*sizeof(char));
        else{
            addstr(out, source + 1, bufflen - 1);
            *out = '\0';
        }
    }else if(*source == '@'){
        if (l <= bufflen)
            memcpy(out, source+1, l*sizeof(char));
        else{
            addstr(out, RETS, LL(RETS));
            bufflen -= LL(RETS);
            memcpy(out, source + 1 + l- bufflen, bufflen * sizeof(char));
        }
    }else{
        const char* nl = strchr(source, '\n');
        addstr(out, PRE, LL(PRE));
        bufflen -= LL(PRE RETS POS) + 1;
        if (l < bufflen && nl == NULL){
            addstr(out, source, l);
        }else{
            if (nl != NULL)
                l = nl - source;
            if (l > bufflen)
                l = bufflen;
            addstr(out, source, l);
            addstr(out, RETS, LL(RETS));
        }
        memcpy(out, POS, (LL(POS) + 1) * sizeof(char));
    }
}