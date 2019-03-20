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

#endif