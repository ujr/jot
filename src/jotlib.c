
#include <assert.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "jotlib.h"
#include "log.h"
#include "pikchr.h"


static int
jot_log_trace(lua_State *L)
{
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  log_trace(s);
  return 0;
}

static int
jot_log_debug(lua_State *L)
{
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  log_debug(s);
  return 0;
}

static int
jot_log_info(lua_State *L)
{
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  log_info(s);
  return 0;
}

static int
jot_log_warn(lua_State *L)
{
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  log_warn(s);
  return 0;
}

static int
jot_log_error(lua_State *L)
{
  const char *s = luaL_checkstring(L, 1);
  log_error(s);
  return 0;
}

static int
jot_log_panic(lua_State *L)
{
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  log_panic(s);
  return 0;
}


/* Lua function: pikchr(str) */
static int
jot_pikchr(lua_State *L)
{
  const char *s, *t;
  const char *class = "pikchr";
  int w, h, darkmode, r;
  unsigned int flags;

  s = luaL_checkstring(L, 1);
  darkmode = lua_toboolean(L, 2);

  flags = 0;
  if (darkmode) flags |= PIKCHR_DARK_MODE;
  flags |= PIKCHR_PLAINTEXT_ERRORS;

  t = pikchr(s, class, flags, &w, &h);

  if (!t) {
    lua_pushnil(L);
    lua_pushstring(L, "pikchr() returns null; out of memory?");
    return 2;
  }

  if (w < 0) {
    lua_pushnil(L);
    lua_pushstring(L, t);
    r = 2;
  }
  else {
    lua_pushstring(L, t);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    r = 3;
  }

  free((void *) t);

  return r;
}


static const struct luaL_Reg jotlib[] = {
  {"log_trace", jot_log_trace},
  {"log_debug", jot_log_debug},
  {"log_info",  jot_log_info},
  {"log_warn",  jot_log_warn},
  {"log_error", jot_log_error},
  {"log_panic", jot_log_panic},
  {"pikchr",    jot_pikchr},
  {0, 0}
};

int
luaopen_jotlib(lua_State *L)
{
  //luaL_newlib(L, jotlib);  is equiv to:
  luaL_newlibtable(L, jotlib);
  luaL_setfuncs(L, jotlib, 0);
  return 1;
}
