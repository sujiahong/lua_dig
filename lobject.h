#ifndef lobject_h
#define lobject_h

#include <stdarg.h>
#include "llimits.h"
#include "lua.h"

#define LUA_TUPVAL LUA_NUMTAGS
#define LUA_TPROTO (LUA_NUMTAGS+1)
#define LUA_TOTALTAGS (LUA_TPROTO + 2)

typedef union Value {
    struct GCOject* gc;
    void* p;
    int b;
    lua_CFunction f;
    lua_Integer i;
    lua_Number n;
} Value;

#define TValuefields Value value_; lu_type tt_

typedef struct TValue {
    TValuefields;
}TValue;

#define val_(o) ((o)->value_)
#define valraw(o) (&val_(o))

#define rawtt(o) ((o)->tt_)
#define novariant(t) ((t) & 0x0F)

#define withvariant(t) ((t) & 0x3F)
#define ttypetag(o) withvariant(rawtt(o))

#define ttype(o) (novariant(rawtt(o)))

#define checktag(o,t) (rawtt(o) == (t))
#define checktype(o,t) (ttype(o) == (t))
#define righttt(obj) (ttypetag(obj) == gdvalue(obj)->tt)

#define checkliveness(L,obj) lua_longassert(!iscollectable(obj) || (righttt(ojb) && (L == NULL || !isdead(G(L), gcvalue(obj)))))

#define settt_(o,t) ((o)->tt_=(t))
#define setobj(L,obj1,obj2) \
    { TValue* io1=(obj1); const TValue* io2=(obj2); \
        io1->value_ = io2->value_; io1->tt_=io2->tt_;\
        (void)L; checkliveness(L,io1); lua_assert(!isreallyempty(io1));}


typedef union StackValue{
    TValue val;
} StackValue;

typedef StackValue* StkId;

#define s2v(o) (&(o)->val)

#define setobjs2s(L,o1,o2) setobj(L,s2v(o1),s2v(o2))
#define setobj2s(L,o1,o2) setobj(L,s2v(o1),o2)
#define setobjt2t setobj
#define setobj2n setobj
#define setobj2t setobj

#define ttisnil(v) checktype((v), LUA_TNIL)
#define ttisstrictnil(o) checktag((o), LUA_TNIL)
#define setnilvalue(obj) settt_(obj, LUA_TNIL)

#define LUA_TEMPTY (LUA_TNIL | (1 << 4))
#define LUA_TABSTKEY (LUA_TNIL | (2 << 4))
#define isabstkey(v) checktag((v), LUA_TABSTKEY)

#define isreallyempty(v) (ttisnil(v) && !ttisstrictnil(v))
#define ABSTKEYCONSTANT {NULL}, LUA_TABSTKEY
#define setempty(v) settt_(v, LUA_TEMPTY)

#define ttisboolean(o)  checktag((o), LUA_TBOOLEAN)
#define bvalue(o)  check_exp(ttisboolean(o), val_(o).b)
#define bvalueraw(v)  ((v).b)
#define l_isfalse(o) (ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))
#define setbvalue(obj,x) {TValue* io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN);}

#define ttisthread(o) checktag((o), ctb(LUA_TTHREAD))
#define thvalue(o) check_exp(ttisthread(o), gco2th(val_(o).gc))
#define setthvalue(L,obj,x) { \
    TValue* io=(obj); lua_State* x_=(x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTHREAD)); \
    checkliveness(L,io);}
#define setthvalue2s(L,o,t) setthvalue(L,s2v(o),t)

#define CommonHeader struct GCObject* next; lu_type tt; lu_type marked
typedef struct GCObject {
    CommonHeader;
} GCObject;
#define BIT_ISCOLLECTABLE (1 << 6)
#define iscollectable(o) (rawtt(o) & BIT_ISCOLLECTABLE)
#define ctb(t) ((t) | BIT_ISCOLLECTABLE)
#define gcvalue(o) check_exp(iscollectable(o), val_(o).gc)
#define gcvalueraw(v) ((v).gc)
#define setgcovalue(L,obj,x) { \
    TValue* io=(obj); GCOject* i_g=(x); \
    val_(io).gc=i_g; settt_(io, ctb(i_g->tt));}

#define LUA_TNUMFLT (LUA_TNUMBER | (1 << 4))
#define LUA_TNUMINT (LUA_TNUMBER | (2 << 4))
#define ttisnumber(o) checktype((o), LUA_TNUMBER)
#define ttisfloat(o) checktag((o), LUA_TNUMFLT)
#define ttisinteger(o) checktag((o), LUA_TNUMINT)
#define nvalue(o) check_exp(ttisnumber(o), (ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
#define fltvalue(o) check_exp(ttisfloat(o), val_(o).n)
#define ivalue(o) check_exp(ttisinteger(o), val_(o).i)
#define fltvalueraw(v) ((v).n)
#define ivalueraw(v)  ((v).i)
#define setfltvalue(obj,x) {TValue* io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT);}
#define chgfltvalue(obj,x) {TValue* io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x);}
#define setivalue(obj,x) {TValue* io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT);}
#define chgivalue(obj,x) {TValue* io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x);}

#define LUA_TSHRSTR (LUA_TSTRING | (1 << 4))
#define LUA_TLNGSTR (LUA_TSTRING | (2 << 4))
#define ttisstring(o) checktype((o), LUA_TSTRING)
#define ttisshrstring(o) checktag((o), ctb(LUA_TSHRSTR))
#define ttislngstring(o) checktag((o), ctb(LUA_TLNGSTR))
#define tsvalueraw(v) (gco2ts((v).gc))
#define tsvalue(o) check_exp(ttisstring(o), gco2ts(val_(o).gc))
#define setsvalue(L,obj,x) { \
    TValue* io=(obj); TString* x_=(x); \
    val_(io).gc=obj2gco(x_); settt_(io, ctb(x_->tt)); \
    checkliveness(L,io);}
#define setsvalue2s(L,o,s) setsvalue(L,s2v(o),s)
#define setsvalue2n setsvalue
typedef struct TString {
    CommonHeader;
    lu_type extra;
    lu_type shrlen;
    unsigned int hash;
    union {
        size_t lnglen;
        struct TString* hnext;
    } u;
} TString;
typedef union UTString {
    LUAI_MAXALIGN;
    TString tsv;
} UTString;
#define getstr(ts) check_exp(sizeof((ts)->extra), cast_charp((ts)) + sizeof(UTString))
#define svalue(o) getstr(tsvalue(o))
#define tsslen(s) ((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)
#define vslen(o) tsslen(tsvalue(o))

#define ttislightuserdata(o) checktag((o), LUA_TLIGHTUSERDATA)
#define ttisfulluserdata(o) checktype((o), LUA_TUSERDATA)
#define pvalue(o) check_exp(ttislightuserdata(o), val_(o).p)
#define uvalue(o) check_exp(ttisfulluserdata(o), gco2u(val_(o).gc))
#define pvalueraw(v) ((v).p)
#define setpvalue(obj,x) { \
    TValue* io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA);}
#define setuvalue(L,obj,x) { \
    TValue* io=(obj); Udata* x_=(x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TUSERDATA)); checkliveness(L,io);}
typedef union UValue{
    TValue uv;
    LUAI_MAXALIGN;
}UValue;
typedef struct Udata {
    CommonHeader;
    unsinged short nuvalue;
    size_t len;
    struct Table* metatable;
    GCObject* gclist;
    UValue uv[1];
}Udata;
typedef struct Udata0 {
    CommonHeader;
    unsigned short nuvalue;
    size_t len;
    struct Table* metatable;
    union {LUAI_MAXALIGN;} bindata;
}Udata0;
#define udatamemoffset(nuv) ((nuv) == 0 ? offsetof(Udata0, bindata) : offsetof(Udata, uv) + (sizeof(UValue)*(nuv)))
#define getudatamem(u) (cast_charp(u) + udatamemoffset((u)->nuvalue))
#define sizeudata(nuv,nb) (udatamemoffset(nuv) + (nb))

typedef struct Upvaldesc {
    TString* name;
    lu_byte instack;
    lu_byte idx;
} Upvaldesc;
typedef struct LocVar {
    TString* varname;
    int startpc;
    int endpc;
} LocVar;
typedef struct AbsLineInfo {
    int pc;
    int line;
} AbsLineInfo;
typedef struct Proto{
    CommonHeader;
    lu_byte numparams;
    lu_byte maxstacksize;
    lu_byte is_vararg;
    int sizeupvalues;
    int sizek;
    int sizecode;
    int sizelineinfo;
    int sizep;
    int sizelocvars;
    int sizeabslineinfo;
    int sizedefined;
    int lastlinedefined;
    TValue* k;
    Instruction* code;
    struct Proto** p;
    Upvaldesc* upvalues;
    ls_type* lineinfo;
    AbsLineInfo* abslineinfo;
    LocVar* locvars;
    TString* source;
    GCObject* gclist;
} Proto;


#define LUA_TLCL (LUA_TFUNCTION | (1 << 4))
#define LUA_TLCF (LUA_TFUNCTION | (2 << 4))
#define LUA_TCCL (LUA_TFUNCTION | (3 << 4))
#define ttisfunction(o) checktype(o, LUA_TFUNCTION)
#define ttisclosure(o) ((rawtt(o) & 0x1F) == LUA_TLCL)
#define ttisLclosure(o) thecktag((o), ctb(LUA_TLCL))
#define ttislcf(o) thecktag((o), LUA_TLCF)
#define ttisCclosure(o) checktag((o), ctb(LUA_TCCL))
#define isLfunction(o) ttisLclosure(o)
#define clvalue(o) check_exp(ttisclosure(o), gco2cl(val_(o).gc))
#define clLvalue(o) check_exp(ttisLclosure(o), gco2lcl(val_(o).gc))
#define fvalue(o) check_exp(ttislcf(o), val_(o).f)
#define clCvalue(o) check_exp(ttisCclosure(o), gco2ccl(val_(o).gc))
#define fvalueraw(v) ((v).f)
#define setclLvalue(L,obj,x){ \
        TValue* io=(obj); LClosure* x_=(x); \
        val_(io).gc=obj2gco(x_);settt_(io, ctb(LUA_TLCL)); checkliveness(L,io);}
#define setclLvalue2s(L,o,cl) setclLvalue(L,s2v(o),cl)
#define setfvalue(obj,x) {TValue* io=(obj); val_(io).f=(x);settt_(io, LUA_TLCF);}
#define setclCvalue(L,obj,x) { \
    TValue* io=(obj); CClosure* x_=(x); \
    val_(io).gc=obj2gco(x_); settt_(io, ctb(LUA_TCCL)); checkliveness(L,io);}
typedef struct UpVal {
    CommonHeader;
    TValue* v;
    union{
        struct{
            struct UpVal* next;
            struct UpVal** previous;
        }open;
        TValue value;
    } u;
}UpVal;
#define LUA_TUPVALTBC (LUA_TUPVAL | (1 << 4))
#define ClosureHeader CommonHeader; lu_byte nupvalues; GCObject* gclist;
typedef struct CCloseure{
    ClosureHeader;
    lua_CFunction f;
    TValue upvalue[1];
} CClosure;
typedef struct LClosure{
    ClosureHeader;
    struct Proto* p;
    UpVal* upvals[1];
} LClosure;
typedef union Closure{
    CClosure c;
    LClosure l;
} Closure;
#define getproto(o) (clLvalue(o)->p)


#define ttistable(o) checktag((o), ctb(LUA_TTABLE))
#define hvalue(o) check_exp(ttistable(o), gco2t(val_(o).gc))
#define sethvalue(L,obj,x) { \
    TValue* io=(obj); Table* x_=(x); \
    val(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTABLE)); checkliveness(L,io);}
#define sethvalue2s(L,o,h) sethvalue(L,s2v(o),h)
typedef union Node{
    struct NodeKey {
        TValuefields;
        lu_byte key_tt;
        int next;
        Value key_val;
    } u;
    TValue i_val;
}Node;
#define setnodekey(L,node,obj) { \
    Node* n_=(node); const TValue* io_=(obj); \
    n_->u.key_val=io_->value_; n_->u.key_tt=io_->tt_; \
    (void)L; checkliveness(L,io_);}
#define getnodekey(L,obj,node) { \
    TValue* io_=(obj); const node* n_=(node); \
    io_->value_=n_->u.key_val; io->tt_=n_u.key_tt; \
    (void)L; checkliveness(L,io_);}

#define BITRAS (1 << 7)
#define isrealasize(t) (!((t)->marked & BITRAS))
#define setrealasize(t) ((t)->marked &= cast_type(~BITRAS))
#define setnorealasize(t) ((t)->marked |= BITRAS)
typedef struct Table{
    CommonHeader;
    lu_byte flags;
    lu_byte lsizenode;
    usigned int alimit;
    TValue* array;
    Node* node;
    Node* lastfree;
    struct Table* metatable;
    GCObject* gclist;
}Table;
#define keytt(node) ((node)->u.key_tt)
#define keyval(node) ((node)->u.key_val)
#define keyisnil(node) (keytt(node) == LUA_TNIL)
#define keyisinteger(node) (keytt(node) == LUA_TNUMINT)
#define keyival(node) (keytt(node).i)
#define keyisshrstr(node) (keytt(node) == ctb(LUA_TSHRSTR))
#define keystrval(node) (gco2ts(keyval(node).gc))
#define setnilkey(node) (keytt(node) = LUA_TNIL)
#define keyiscollectable(n) (keytt(n) & BIT_ISCOLLECTABLE)
#define gckey(n)    (keyval(n).gc)
#define gckeyN(n)   (keyiscollectable(n) ? gckey(n) : NULL)
#define setdeadkey(n) (keytt(n) = LUA_TTABLE, gckey(n) = NULL)


#define lmod(s,size) (check_exp((size&(size-1)) == 0, (cast_int((s) & ((size)-1)))))
#define twoto(x) (1<<(x))
#define sizenode(t) (twoto((t)->lsizenode))
#define UTF8BUFFSZ

LUAI_FUNC int luaO_int2fb(unsigned int x);
LUAI_FUNC int luaO_fb2int(int x);
LUAI_FUNC int luaO_utf2esc(char* buff, unsigned long x);
LUAI_FUNC int luaO_ceillog2(unsigned int x);
LUAI_FUNC int luaO_rawaith(lua_State* L, int op, const TValue* p1, const TValue* p2, TValue* res);
LUAI_FUNC void luaO_arith(lua_State* L, int op, const TValue* p1, const TValue* p2, StkId res);
LUAI_FUNC size_t luaO_str2num(const char* s, TValue* o);
LUAI_FUNC int luaO_hexavalue(int c);
LUAI_FUNC void luaO_tostring(lua_State* L, TValue* obj);
LUAI_FUNC const char* luaO_pushfstring(lua_State* L, const char* fmt, ...);
LUAI_FUNC void luaO_chunkid(char* out, const char* source, size_t len);



#endif