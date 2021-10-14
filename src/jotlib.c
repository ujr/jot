
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lua.h"
#include "lauxlib.h"

#include "jot.h"
#include "jotlib.h"
#include "log.h"
#include "pikchr.h"
#include "wildmatch.h"


static struct {
  char dirsep;         /* directory separator */
} J;


static int
jot_error(lua_State *L, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  lua_pushvfstring(L, fmt, ap);
  va_end(ap);
  return lua_error(L);
}


static int
error_readonly(lua_State  *L)
{
  return jot_error(L, "cannot update readonly table");
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


bool
streq(const char *s, const char *t)
{
  if (!s && !t) return true;
  if (!s || !t) return false;
  return strcmp(s, t) == 0;
}


/* File System Paths */


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
      path = p;  /* for POSIX: if (*p) path = p; */
    }
  }

  for (p = path; *p; ) {
    if (*p++ == alt) {
      path = p;  /* for POSIX: if (*p) path = p; */
    }
  }

  return path;
}


static int
jot_basename(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  assert(path != NULL);
  lua_pushstring(L, basename(path));
  return 1;
}


static int
jot_dirname(lua_State *L)
{
  /* Special cases: return "/" if path is "/" and
   * return "." if path has no "/" or is empty */
  const char *path = luaL_checkstring(L, 1);
  assert(path != NULL);
  size_t len = strlen(path);
  char sep = J.dirsep;

  /* scan for last dir sep in path: */
  while (len > 0 && path[len-1] != sep) len--;
  /* now path[0..len-2] is dirname: */
  if (len > 0) {
    if (len > 1) --len;
    lua_pushlstring(L, path, len);
    return 1;
  }
  /* path is empty or has no sep: */
  lua_pushstring(L, ".");
  return 1;
}


static int
jot_splitpath_iter(lua_State *L)
{
  const char *path;
  size_t len, index, scout;

  path = luaL_checklstring(L, lua_upvalueindex(1), &len);
  index = luaL_checkinteger(L, lua_upvalueindex(2));

  if (index >= len)
    return 0;

  /* special case: initial dir sep is returned as such */
  if (index == 0 && len > 0 && path[index] == J.dirsep) {
    char dirsep[2] = { J.dirsep, 0 };
    lua_pushinteger(L, ++index);
    lua_replace(L, lua_upvalueindex(2));
    lua_pushstring(L, dirsep);
    return 1;
  }

  /* skip separators, then scan and yield next part */
  while (index < len && path[index] == J.dirsep) ++index;
  scout = index;
  while (scout < len && path[scout] != J.dirsep) ++scout;
  if (scout > index) {
    lua_pushinteger(L, scout);
    lua_replace(L, lua_upvalueindex(2));
    lua_pushlstring(L, path+index, scout-index);
    return 1;
  }

  return 0; /* no more parts */
}

static int
jot_splitpath(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  assert(path != NULL);

  lua_pushstring(L, path);
  lua_pushinteger(L, 0);
  lua_pushcclosure(L, jot_splitpath_iter, 2);
  return 1;
}


static int
jot_joinpath(lua_State *L)
{
  /* Special cases:
   * - no args: return "."
   * - one arg, empty: return "."
   * - last arg empty: append "/"
   */
  luaL_Buffer buf;
  const char *s;
  size_t len;
  int needsep;
  int i, n = lua_gettop(L);  /* number of args */
  if (n < 1) {
    lua_pushstring(L, ".");
    return 1;
  }
  if (n == 1) {
    s = luaL_checkstring(L, 1);
    lua_pushstring(L, s && *s ? s : ".");
    return 1;
  }
  luaL_buffinit(L, &buf);
  for (needsep = 0, i = 1; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "expect string arguments");
      return lua_error(L);
    }
    s = lua_tostring(L, i);
    assert(s != NULL);
    len = strlen(s);
    if ((i == n) && !*s)
      luaL_addchar(&buf, J.dirsep);
    else {
      /* trim trailing and leading (but not 1st arg) seps */
      while (len > 0 && s[len-1] == J.dirsep) --len;
      if (i > 1)
        while (len > 0 && *s == J.dirsep) ++s, --len;
      if (len == 0) {
        /* end with sep if last part is empty */
        if (i == n) luaL_addchar(&buf, J.dirsep);
      }
      else {
        if (needsep) luaL_addchar(&buf, J.dirsep);
        luaL_addlstring(&buf, s, len);
        needsep = 1;
      }
    }
  }
  luaL_pushresult(&buf);
  return 1;
}


static int
jot_normpath(lua_State *L)
{
  luaL_Buffer b;
  const char *path;
  size_t len, idx;
  int stk, i;

  /* implementation sketch: use a table as a stack; for each
   * part: if "." skip, if ".." pop, else push; concat stack */

  path = luaL_checklstring(L, 1, 0);
  assert(path != NULL);
  len = strlen(path); /* up to first \0 only */

  luaL_buffinit(L, &b);
  if (len > 0 && path[0] == J.dirsep) {
    /* special case: initial dir sep */
    luaL_addchar(&b, J.dirsep);
  }

  lua_newtable(L);
  for (stk=0, idx=0; idx < len;) {
    while (idx < len && path[idx] == J.dirsep) ++idx;
    size_t base = idx;
    while (idx < len && path[idx] != J.dirsep) ++idx;
    if (idx == base) break;
    if (strncmp(path+base, ".", idx-base) == 0)
      continue; /* skip */
    if (strncmp(path+base, "..", idx-base) == 0) {
      if (stk > 0) {
        lua_pushnil(L);
        lua_rawseti(L, -2, stk--); /* pop */
      }
      continue;
    }
    lua_pushlstring(L, path+base, idx-base);
    lua_rawseti(L, -2, ++stk); /* push */
  }

  for (i = 1; i <= stk; i++) {
    if (i > 1)
      luaL_addchar(&b, J.dirsep);;
    lua_rawgeti(L, -1, i);
    luaL_addvalue(&b);
  }

  if (luaL_bufflen(&b) == 0)
    luaL_addchar(&b, '.');

  lua_pop(L, 1); /* the table */
  luaL_pushresult(&b);
  return 1;
}


static int
jot_matchpath(lua_State *L)
{
  int flags, r;
  const char *pat = luaL_checkstring(L, 1);
  const char *path = luaL_checkstring(L, 2);
  if (!lua_isnone(L, 3))
    return jot_error(L, "too many arguments");
  flags = WILD_PATHNAME | WILD_PERIOD;
  r = wildmatch(pat, path, flags);
  lua_pushboolean(L, r);
  return 1;
}


static const struct luaL_Reg pathlib[] = {
  {"basename", jot_basename},
  {"dirname",  jot_dirname},
  {"split",    jot_splitpath},
  {"join",     jot_joinpath},
  {"norm",     jot_normpath},
  {"match",    jot_matchpath},
  {0, 0}
};


static int
luaopen_jotlib_path(lua_State *L)
{
  luaL_newlib(L, pathlib);
  return 1;
}


/* File System Operations */


#define makedir(path) mkdir((path), 0775)
#define dropdir(path) rmdir((path))


static int
jot_getcwd(lua_State *L)
{
  char *buf = 0;
  size_t len;
  if (!lua_isnone(L, 1))
    return jot_error(L, "expect no arguments");
  for (len=512; ; len *= 2) {
    char *p = realloc(buf, len);
    if (!p) {
      if (buf) free(buf);
      return jot_error(L, "out of memory");
    }
    if (getcwd((buf=p), len)) {
      lua_pushstring(L, buf);
      free(buf);
      break;
    }
    if (errno != ERANGE) {
      if (buf) free(buf);
      return jot_error(L, "cannot getcwd: %s", strerror(errno));
    }
  }
  return 1;
}


static int
jot_mkdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  log_trace("calling makedir %s", path);
  if (makedir(path) < 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "mkdir %s: %s", path, strerror(errno));
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}


static int
jot_rmdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  log_trace("calling dropdir %s", path);
  if (dropdir(path) < 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "rmdir %s: %s", path, strerror(errno));
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}


static int
jot_exists(lua_State *L)
{
  struct stat statbuf;
  const char *path = luaL_checkstring(L, 1);
  const char *type = lua_tostring(L, 2);
  enum { Any, Reg, Dir, Lnk } t;
  if (!lua_isnone(L, type ? 3 : 2))
    return jot_error(L, "too many arguments");
  if (!type || streq(type, "any")) t = Any;
  else if (streq(type, "file") || streq(type, "regular")) t = Reg;
  else if (streq(type, "dir") || streq(type, "directory")) t = Dir;
  else if (streq(type, "symlink")) t = Lnk;
  else return jot_error(L, "invalid arg #2");
  if (stat(path, &statbuf) < 0) {
    if (errno == ENOENT) {
      lua_pushboolean(L, 0);
      return 1;
    }
    return jot_error(L, "cannot stat %s: %s", path, strerror(errno));
  }
  lua_pushboolean(L, t == Any ||
    (t == Reg && S_ISREG(statbuf.st_mode)) ||
    (t == Dir && S_ISDIR(statbuf.st_mode)) ||
    (t == Lnk && S_ISLNK(statbuf.st_mode)));
  return 1;
}


static int
jot_getinfo(lua_State *L)
{
  struct stat statbuf;
  const char *type;
  const char *path = luaL_checkstring(L, 1);
  bool gottab = lua_istable(L, 2);
  if (!lua_isnone(L, gottab ? 3 : 2))
    return jot_error(L, "too many arguments");
  if (stat(path, &statbuf) < 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "cannot stat %s: %s", path, strerror(errno));
    return 2;
  }

       if (S_ISREG(statbuf.st_mode)) type = "file";
  else if (S_ISDIR(statbuf.st_mode)) type = "directory";
  else if (S_ISLNK(statbuf.st_mode)) type = "symlink";
  else                               type = "other";

  if (gottab) lua_pushvalue(L, 2);
  else lua_createtable(L, 0, 3);
  lua_pushstring(L, type);
  lua_setfield(L, -2, "type");
  lua_pushinteger(L, statbuf.st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, statbuf.st_mtime);
  lua_setfield(L, -2, "mtime");
  return 1;
}


static int
jot_glob(lua_State *L)
{
  const char *root = luaL_checkstring(L, 1);
  const char *pat = lua_tostring(L, 2);

  UNUSED(root);
  UNUSED(pat);

  // iterator yielding matching paths
  // probably much easier to write in Lua
  // dir walk: start dir is glob prefix (up to last / befor first wildcard), filter paths against glob
  return jot_error(L, "glob: not yet implemented");
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


static const struct luaL_Reg loglib[] = {
  {"trace", jot_log_trace},
  {"debug", jot_log_debug},
  {"info",  jot_log_info},
  {"warn",  jot_log_warn},
  {"error", jot_log_error},
  {"panic", jot_log_panic},
  {0, 0}
};


static int
luaopen_jotlib_log(lua_State *L)
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
    jot_error(L, "error reading directory: %s", strerror(errno));
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
    return jot_error(L, "cannot open %s: %s", path, strerror(errno));
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


/* Return directory separator: '\' on Windows and '/' elsewhere.
 * Get dirsep from package.config, a Lua compile time constant.
 */
static char
getdirsep(lua_State *L)
{
  const char *dirsep;
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "config");
  dirsep = lua_tostring(L, -1);
  lua_pop(L, 2); // package and config
  return dirsep && *dirsep ? *dirsep : '/';
}


static const struct luaL_Reg jotlib[] = {
  {"log_trace", jot_log_trace},
  {"log_debug", jot_log_debug},
  {"log_info",  jot_log_info},
  {"log_warn",  jot_log_warn},
  {"log_error", jot_log_error},
  {"log_panic", jot_log_panic},

  {"basename",  jot_basename},
  {"dirname",   jot_dirname},
  {"splitpath", jot_splitpath},
  {"joinpath",  jot_joinpath},
  {"normpath",  jot_normpath},
  {"matchpath", jot_matchpath},

  {"readdir",   jot_readdir},
  {"getcwd",    jot_getcwd},
  {"mkdir",     jot_mkdir},
  {"rmdir",     jot_rmdir},
  {"exists",    jot_exists},
  {"getinfo",   jot_getinfo},
  {"glob",      jot_glob},
  {"pikchr",    jot_pikchr},
  {0, 0}
};


int
luaopen_jotlib(lua_State *L)
{
  J.dirsep = getdirsep(L);
  log_trace("using '%c' as dirsep", J.dirsep);

  luaL_newmetatable(L, JOTLIB_READDIR_REGKEY);
  lua_pushcfunction(L, jot_readdir_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  luaL_newlib(L, jotlib);

  luaopen_jotlib_log(L);
  lua_setfield(L, -2, "log");

  luaopen_jotlib_path(L);
  lua_setfield(L, -2, "path");

  lua_pushstring(L, VERSION);
  lua_setfield(L, -2, "VERSION");

  return 1;
}
