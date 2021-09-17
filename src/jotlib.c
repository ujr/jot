
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include "lua.h"
#include "lauxlib.h"

#include "jotlib.h"
#include "log.h"
#include "pikchr.h"


/* Return pointer to one after the last occurrence of a directory
 * separator. Return the argument if there is no directory separator.
 * Special case: the basename of "/" is "/".
 */
const char *
basename(const char *path)
{
  static const char sep = '/';
  static const char alt = '\\';
  register const char *p;

  if (!path) return 0;

  for (p = path; *p; ) {
    if (*p++ == sep) {
      if (*p) path = p;
    }
  }

  for (p = path; *p; ) {
    if (*p++ == alt) {
      if (*p) path = p;
    }
  }

  return (char *) path;
}


static int
jot_basename(lua_State *L)
{
  const char *s = lua_tostring(L, 1);
  s = basename(s);
  lua_pushstring(L, s);
  return 1;
}


void
dump_stack(lua_State *L, const char *prefix)
{
  int i, top = lua_gettop(L);  /* stack depth */
  if (!prefix) prefix = "";
  for (i = 1; i <= top; i++) {
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TBOOLEAN:
        log_debug("%s%d: %s", prefix, i, lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        log_debug("%s%d: %g", prefix, i, lua_tonumber(L, i));
        break;
      case LUA_TSTRING:
        log_debug("%s%d: '%s'", prefix, i, lua_tostring(L, i));
        break;
      default:
        log_debug("%s%d: %s", prefix, i, lua_typename(L, t));
        break;
    }
  }
}


/* Logging */

static int
jot_log(lua_State *L, int level)
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
jot_log_trace(lua_State *L)
{
  return jot_log(L, LOG_TRACE);
}

static int
jot_log_debug(lua_State *L)
{
  return jot_log(L, LOG_DEBUG);
}

static int
jot_log_info(lua_State *L)
{
  return jot_log(L, LOG_INFO);
}

static int
jot_log_warn(lua_State *L)
{
  return jot_log(L, LOG_WARN);
}

static int
jot_log_error(lua_State *L)
{
  return jot_log(L, LOG_ERROR);
}

static int
jot_log_panic(lua_State *L)
{
  return jot_log(L, LOG_PANIC);
}


/* File System */

#define JOTLIB_READDIR_REGKEY JOTLIB_REGKEY ".readdir"

static int
jot_readdir_iter(lua_State *L)
{
  struct dirent *entry;
  DIR *dir = *(DIR **) lua_touserdata(L, lua_upvalueindex(1));
  errno = 0;
  entry = readdir(dir);
  if (entry) {
    lua_pushstring(L, entry->d_name);
    return 1;
  }
  if (errno) {
    luaL_error(L, "error reading directory: %s", strerror(errno));
  }
  return 0; /* no more entries */
}

static int jot_readdir_gc(lua_State *L)
{
  DIR **dirp = (DIR **) lua_touserdata(L, 1);
  if (*dirp) {
    errno = 0;
    int r = closedir(*dirp);
    log_trace("jot_readdir_gc: closedir() returned %d, errno=%d", r, errno);
    *dirp = 0;
  }
  return 0;
}

static int
jot_readdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  DIR **dirp = lua_newuserdata(L, sizeof(DIR *));

  assert(dirp != NULL);

  *dirp = 0; /* unopened */
  luaL_getmetatable(L, JOTLIB_READDIR_REGKEY);
  lua_setmetatable(L, -2); /* from now on __gc may be called */

  *dirp = opendir(path);
  if (!*dirp) {
    return luaL_error(L, "cannot open %s: %s", path, strerror(errno));
  }

  /* push iterator function (its upvalue, the DIR, is alreay on stack) */
  lua_pushcclosure(L, jot_readdir_iter, 1);
  return 1;
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
  {"basename",  jot_basename},
  {"readdir",   jot_readdir},
  {"pikchr",    jot_pikchr},
  {0, 0}
};

int
luaopen_jotlib(lua_State *L)
{
  luaL_newmetatable(L, JOTLIB_READDIR_REGKEY);
  lua_pushcfunction(L, jot_readdir_gc);
  lua_setfield(L, -2, "__gc");

  luaL_newlib(L, jotlib);
  return 1;
}
