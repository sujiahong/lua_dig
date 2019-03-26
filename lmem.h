#ifndef lmem_h
#define lmem_h

#include <stddef.h>
#include "llimits.h"
#include "lua.h"

#define luaM_error(L) luaD_throw(L, LUAERRMEM)

#define luaM_testsize(n,e) \
    (sizeof(n) >= sizeof(size_t) && cast_sizet((n)) + 1 > MAX_SIZET/(e))

#define luaM_checksize(L,n,e) (luaM_testsize(n,e) ? luaM_toobig(L) : cast_void(0))

#define luaM_limitN(n,t) ((cast_sizet(n) <= MAX_SIZET/sizeof(t)) ? (n) : cast_unit((MAX_SIZET/sizeof(t))))
#define luaM_reallocvchar(L,b,on,n) cast_charp(luaM_saferealloc_(L, (b), (on)*sizeof(char), (n)*sizeof(char)))
#define luaM_freemem(L,b,s) luaM_free_(L,(b),(s))
#define luaM_free(L,b) luaM_free_(L,(b),sizeof(*(b)))
#define luaM_freearray(L,b,n) luaM_free(L,(b),(n)*sizeof(*(b)))

#define luaM_new(L,t) cast(t*,luaM_malloc_(L,sizeof(t),0))
#define luaM_newvector(L,n,t) cat(t*,luaM_malloc_(L,(n)*sizeof(t),0))
#define luaM_newvectorchecked(L,n,t) (luaM_checksize(L,n,sizeof(t)), luaM_newvector(L,n,t))
#define luaM_newobject(L,tag,s) luaM_malloc_(L,(s),tag)
#define luaM_growvector(L,v,nelems,size,t,limit,e) ((v)=cast(t*,luaM_growaux_(L,v,nelems,&(size),sizeof(t),luaM_limitN(limit,t),t)))
#define luaM_rellocvector(L,v,oldn,n,t) (cast(t*,luaM_realloc_(L,v,cast_sizet(oldn)*sizeof(t),cast_sizet(n)*sizeof(t))))
#define luaM_shrinkvector(L,v,size,fs,t) ((v)=cast(t*,luaM_shrinkvector_(L,v,&(size),fs,sizeof(t))))

LUAI_FUNC l_noret luaM_toobig(lua_State*L);
LUAI_FUNC void* luaM_realloc_ (lua_State* L, void* block, size_t oldsize, size_t size);
LUAI_FUNC void* luaM_saferelloc_(lua_State* L, void* block, size_t oldsize, size_t size);
LUAI_FUNC void luaM_free_(lua_State* L, void* block, size_t osize);
LUAI_FUNC void* luaM_growaux_(lua_State* L, void* block, int nelems, int* size, int size_elem, int limit, const char* what);
LUAI_FUNC void* luaM_shrinkvector_(lua_State* L, void* block, int* nelem, int final_n, int size_elem);
LUAI_FUNC void* luaM_malloc_(lua_State* L, size_t size, int tag);

#endif