/* Logging */

#include "lua.h"
#include "lauxlib.h"

#include "log.h"
#include "utils.h"


static int
logup(lua_State *L, int level)
{
  const char *file = "?";
  int line = 0;
  const char *msg = luaL_checkstring(L, 1);

  lua_Debug debug;
  if (lua_getstack(L, 1, &debug)) {
    lua_getinfo(L, "Sl", &debug);
    if (debug.currentline > 0) {
      file = basename(debug.short_src);
      line = debug.currentline;
    }
  }

  log_log(level, file, line, "%s", msg);
  return 0;
}


static int
f_trace(lua_State *L)
{
  return logup(L, LOG_TRACE);
}

static int
f_debug(lua_State *L)
{
  return logup(L, LOG_DEBUG);
}

static int
f_info(lua_State *L)
{
  return logup(L, LOG_INFO);
}

static int
f_warn(lua_State *L)
{
  return logup(L, LOG_WARN);
}

static int
f_error(lua_State *L)
{
  return logup(L, LOG_ERROR);
}

static int
f_panic(lua_State *L)
{
  return logup(L, LOG_PANIC);
}


static int
error_readonly(lua_State *L)
{
  lua_pushstring(L, "cannot update readonly table");
  return lua_error(L);
}


static const struct luaL_Reg loglib[] = {
  {"trace", f_trace },
  {"debug", f_debug },
  {"info",  f_info  },
  {"warn",  f_warn  },
  {"error", f_error },
  {"panic", f_panic },
  {0, 0}
};


int
luaopen_loglib(lua_State *L)
{
  /* just for curiosity: make it readonly */
  lua_newtable(L);  /* proxy table */
  lua_newtable(L);  /* meta table */
  luaL_newlib(L, loglib);
  lua_setfield(L, -2, "__index");
  lua_pushcfunction(L, error_readonly);
  lua_setfield(L, -2, "__newindex");
  lua_setmetatable(L, -2);
  return 1;
}
