/* File Path Manipulation */

#include <assert.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "wildmatch.h"


#if defined(_WIN32)
#define DIRSEP   '\\'
#define PATHSEP  ';'
#else
#define DIRSEP   '/'
#define PATHSEP  ':'
#endif


static struct {
  char dirsep;
  char pathsep;
} M;


static int
f_config(lua_State *L)
{
  const char *dirsep = lua_tostring(L, 1);
  const char *pathsep = lua_tostring(L, 2);
  M.dirsep = dirsep && *dirsep ? *dirsep : DIRSEP;
  M.pathsep = pathsep && *pathsep ? *pathsep : PATHSEP;
  lua_pushlstring(L, &M.dirsep, 1);
  lua_pushlstring(L, &M.pathsep, 1);
  return 2;
}


static int
f_basename(lua_State *L)
{
  register const char *p;
  const char *path = luaL_checkstring(L, 1);
  const char sep = M.dirsep;

  /* pointer to one after last occurrence of separator,
   * assuming an implicit separator just before given path;
   * unlike POSIX, the result may be the empty string;
   * special case: basename of "/" is "/"                */

  for (p = path; *p; ) {
    if (*p++ == sep) {
      path = p;  /* for POSIX: if (*p) path = p; */
    }
  }

  lua_pushstring(L, path);
  return 1;
}


static int
f_dirname(lua_State *L)
{
  /* Special cases: return "/" if path is "/" and
   * return "." if path has no "/" or is empty */
  const char *path = luaL_checkstring(L, 1);
  size_t len = strlen(path);
  const char sep = M.dirsep;

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
splitpath_iter(lua_State *L)
{
  const char *path;
  size_t len, index, scout;
  const char sep = M.dirsep;

  path = luaL_checklstring(L, lua_upvalueindex(1), &len);
  index = luaL_checkinteger(L, lua_upvalueindex(2));

  if (index >= len)
    return 0;

  /* special case: initial dir sep is returned as such */
  if (index == 0 && len > 0 && path[index] == sep) {
    char dirsep[2] = { sep, 0 };
    lua_pushinteger(L, ++index);
    lua_replace(L, lua_upvalueindex(2));
    lua_pushstring(L, dirsep);
    return 1;
  }

  /* skip separators, then scan and yield next part */
  while (index < len && path[index] == sep) ++index;
  scout = index;
  while (scout < len && path[scout] != sep) ++scout;
  if (scout > index) {
    lua_pushinteger(L, scout);
    lua_replace(L, lua_upvalueindex(2));
    lua_pushlstring(L, path+index, scout-index);
    return 1;
  }

  return 0; /* no more parts */
}

static int
f_splitpath(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  lua_pushstring(L, path);
  lua_pushinteger(L, 0);
  lua_pushcclosure(L, splitpath_iter, 2);
  return 1;
}


#define BLEN(bf) (luaL_bufflen(bf))
#define BPTR(bf) (luaL_buffaddr(bf))
#define NEEDSEP(bf, sep) (BLEN(bf) > 0 && BPTR(bf)[BLEN(bf)-1] != sep)

static void
appenddir(luaL_Buffer *pbuf, int i, int n, const char *s)
{
  size_t len;
  int inisep;
  assert(s != NULL);
  len = strlen(s);
  /* trim trailing and leading (but not 1st arg) seps */
  inisep = i==1 && len>0 && s[0]==M.dirsep;
  while (len > 0 && s[len-1] == M.dirsep) --len;
  if (i > 1)
    while (len > 0 && *s == M.dirsep) ++s, --len;
  if (len == 0) {
    /* initial sep or last part empty force a dirsep */
    if (inisep || (i == n && NEEDSEP(pbuf, M.dirsep)))
      luaL_addchar(pbuf, M.dirsep);
  }
  else {
    if (NEEDSEP(pbuf, M.dirsep))
      luaL_addchar(pbuf, M.dirsep);
    luaL_addlstring(pbuf, s, len);
  }
}


/** path.join(table|path...): path */
static int
f_joinpath(lua_State *L)
{
  /* Special cases:
   * - no args: return "."
   * - one arg, empty: return "."
   * - last arg empty: append "/"
   */
  luaL_Buffer buf;
  int i, n = lua_gettop(L);  /* number of args */

  if (n < 1) {
    lua_pushstring(L, ".");
    return 1;
  }

  if (n == 1) {
    int t = lua_type(L, 1);
    if (t == LUA_TSTRING) {
      const char *s = lua_tostring(L, 1);
      lua_pushstring(L, s && *s ? s : ".");
      return 1;
    }
    if (t == LUA_TTABLE) {
      luaL_buffinit(L, &buf);
      n = luaL_len(L, 1);
      for (i = 1; i <= n; i++) {
        if (lua_geti(L, 1, i) != LUA_TSTRING) {
          lua_pushstring(L, "all table entries must be strings");
          return lua_error(L);
        }
        appenddir(&buf, i, n, lua_tostring(L, -1));
        lua_pop(L, 1);
      }
      if (luaL_bufflen(&buf) < 1)
        luaL_addchar(&buf, '.');
      luaL_pushresult(&buf);
      return 1;
    }
    lua_pushstring(L, "single argument must be string or table");
    return lua_error(L);
  }

  luaL_buffinit(L, &buf);
  for (i = 1; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "argument must be a string");
      return lua_error(L);
    }
    appenddir(&buf, i, n, lua_tostring(L, i));
  }
  luaL_pushresult(&buf);
  return 1;
}


/** path.norm(path): path */
static int
f_normpath(lua_State *L)
{
  luaL_Buffer b;
  const char *path;
  size_t len, idx;
  int stk, i;
  const char sep = M.dirsep;

  /* implementation sketch: use a table as a stack; for each
   * part: if "." skip, if ".." pop, else push; concat stack */

  path = luaL_checklstring(L, 1, 0);
  assert(path != NULL);
  len = strlen(path); /* up to first \0 only */

  luaL_buffinit(L, &b);
  if (len > 0 && path[0] == sep) {
    /* special case: initial dir sep */
    luaL_addchar(&b, sep);
  }

  lua_newtable(L);
  for (stk=0, idx=0; idx < len;) {
    while (idx < len && path[idx] == sep) ++idx;
    size_t base = idx;
    while (idx < len && path[idx] != sep) ++idx;
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
      luaL_addchar(&b, sep);;
    lua_rawgeti(L, -1, i);
    luaL_addvalue(&b);
  }

  if (luaL_bufflen(&b) == 0)
    luaL_addchar(&b, '.');

  lua_pop(L, 1); /* the table */
  luaL_pushresult(&b);
  return 1;
}


/** path.match(pat, path): boolean */
static int
f_matchpath(lua_State *L)
{
  const char *pat = luaL_checkstring(L, 1);
  const char *path = luaL_checkstring(L, 2);
  int flags = WILD_PATHNAME | WILD_PERIOD;
  int r = wildmatch(pat, path, flags);
  lua_pushboolean(L, r);
  return 1;
}


static const struct luaL_Reg pathlib[] = {
  {"config",   f_config    },
  {"basename", f_basename  },
  {"dirname",  f_dirname   },
  {"split",    f_splitpath },
  {"join",     f_joinpath  },
  {"norm",     f_normpath  },
  {"match",    f_matchpath },
  {0, 0}
};


int
luaopen_pathlib(lua_State *L)
{
  M.dirsep = DIRSEP;
  M.pathsep = PATHSEP;

  luaL_newlib(L, pathlib);
  return 1;
}
