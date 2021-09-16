#ifndef JOTLIB_H
#define JOTLIB_H

#include "lua.h"

#define JOTLIB_REGKEY "Sylphe.jot"

const char *basename(const char *path);

int luaopen_jotlib(lua_State *L);

#endif /* JOTLIB_H */

