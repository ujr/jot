#ifndef JOTLIB_H
#define JOTLIB_H

#include "lua.h"

#define JOTLIB_REGKEY "Sylphe.jot"

const char *basename(const char *path);
void dump_stack(lua_State *L, const char *prefix);

int luaopen_jotlib(lua_State *L);

#endif /* JOTLIB_H */

