
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "lua.h"
#include "lauxlib.h"

#include "jot.h"
#include "jotlib.h"
#include "log.h"
#include "pikchr.h"
#include "utils.h"
#include "walkdir.h"


int
jot_error(lua_State *L, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  lua_pushvfstring(L, fmt, ap);
  va_end(ap);
  return lua_error(L);
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


/* File System Operations */


static int
touchfile(const char *path, time_t ttime)
{
  struct utimbuf times, *ptimes;
  mode_t mode = 0666; /* rw-rw-rw- */
  int fd = open(path, O_RDWR|O_CREAT|O_NOCTTY, mode);
  if (fd < 0) return -1;
  if (close(fd) < 0) return -1;
  /* now the file exists; set times */
  if (ttime > 0) {
    times.modtime = ttime;
    times.actime = ttime;
    ptimes = &times;
  }
  else ptimes = 0; /* set current time */
  if (utime(path, ptimes) < 0) return -1;
  return 0; /* ok */
}


static int
ok(lua_State *L)
{
  lua_pushboolean(L, 1);
  return 1;
}


static int
failed(lua_State *L, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  lua_pushnil(L);
  lua_pushvfstring(L, fmt, ap);
  va_end(ap);
  return 2;
}


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
  log_trace("calling mkdir %s", path);
  if (mkdir(path, 0775) < 0)
    return failed(L, "mkdir %s: %s", path, strerror(errno));
  return ok(L);
}


static int
jot_rmdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  log_trace("calling rmdir %s", path);
  if (rmdir(path) < 0)
    return failed(L, "rmdir %s: %s", path, strerror(errno));
  return ok(L);
}


static int
jot_listdir(lua_State *L)
{
  DIR *dp;
  struct dirent *e;
  const char *path;
  int i;

  path = luaL_checkstring(L, 1);
  if (lua_gettop(L) > 1)
    return jot_error(L, "too many arguments");

  dp = opendir(path);
  if (!dp)
    return failed(L, "opendir %s: %s", path, strerror(errno));

  lua_newtable(L);
  errno = 0;
  for (i = 1; (e = readdir(dp)); i++) {
    lua_pushinteger(L, i);
    lua_pushstring(L, e->d_name);
    lua_settable(L, -3);
  }

  if (errno) {
    lua_pop(L, 1); /* the table */
    return failed(L, "listdir %s: %s", path, strerror(errno));
  }

  closedir(dp);
  return 1; /* the table */
}


static int
jot_touch(lua_State *L)
{
  int isnum;
  const char *path = luaL_checkstring(L, 1);
  lua_Number ttime = lua_tonumberx(L, 2, &isnum);
  if (!lua_isnone(L, 3))
    return jot_error(L, "too many arguments");
  log_trace("calling touchfile %s", path);
  if (touchfile(path, (time_t)(long)ttime) < 0)
    return failed(L, "touch %s: %s", path, strerror(errno));
  return ok(L);
}


static int
jot_remove(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  if (lua_gettop(L) > 1)
    return jot_error(L, "too many arguments");
  if (remove(path) != 0)
    return failed(L, "remove %s: %s", path, strerror(errno));
  return ok(L);
}


static int
jot_rename(lua_State *L) {
  const char *oldname = luaL_checkstring(L, 1);
  const char *newname = luaL_checkstring(L, 2);
  if (lua_gettop(L) > 2)
    return jot_error(L, "too many arguments");
  if (rename(oldname, newname))
    return failed(L, "rename: %s", strerror(errno));
  return ok(L);
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
  if (stat(path, &statbuf) < 0)
    return failed(L, "cannot stat %s: %s", path, strerror(errno));

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
jot_getenv(lua_State *L)
{
  const char *name = luaL_checkstring(L, 1);
  const char *value = getenv(name);
  lua_pushstring(L, value); /* pushes nil if value is null */
  return 1;
}


static void
randlets(char *buf, size_t len)
{
  /* 64 (6 bits) letters, some repeated (lower+upper is only 52) */
  static const char letters[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZADGJMP"
    "abcdefghijklmnopqrstuvwxyzknqtwz";
  size_t i;

  /* Seed with clock and buffer's address; repeatedly get 6 bits
     from somewhere in the middle of the next random number */
  srand(clock()*14741+(uintptr_t)buf);
  for (i=0; i<len; i++)
    buf[i] = letters[(rand()&0x3F0)>>4];
}


static int
jot_tempdir(lua_State *L)
{
  static const char *pat = "XXXXXX";
  const char *arg = lua_tostring(L, 1);
  size_t len, patlen = strlen(pat);
  char *template;

  if (arg) {
    len = strlen(arg);
    if (len < patlen || memcmp(arg+len-patlen, pat, patlen)) {
      errno = EINVAL;
      return failed(L, "tempdir: argument must end in %s", pat);
    }
    template = malloc(len+1);
    if (!template) return failed(L, "tempdir: %s", strerror(errno));
    memcpy(template, arg, len+1);
  }
  else {
    const char *tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";
    len = strlen(tmp)+5+patlen;
    template = malloc(len+1);
    if (!template) return failed(L, "tempdir: %s", strerror(errno));
    sprintf(template, "%s/jot-%s", tmp, pat);
  }

  int attempts = 99;
  do {
    randlets(template+len-patlen, patlen);
    if (mkdir(template, 0700) == 0) {
      log_trace("created directory %s", template);
      lua_pushstring(L, template);
      free(template);
      return 1;
    }
  } while (--attempts > 0 && errno == EEXIST);

  free(template);
  return failed(L, "tempdir: %s", strerror(errno));
}


static int
jot_walkdir_gc(lua_State *L)
{
  struct walk *pwalk = lua_touserdata(L, 1);
  if (pwalk) {
    log_trace("jot_walkdir_gc");
    walkdir_free(pwalk);
  }
  return 0;
}


static int
jot_walkdir_iter(lua_State *L)
{
  struct walk *pwalk = lua_touserdata(L, lua_upvalueindex(1));

  int type = walkdir_next(pwalk);
  const char *path = walkdir_path(pwalk);

  if (type < 0)
    return jot_error(L, "walkdir: %s", strerror(errno));
  if (type == WALK_DONE)
    return 0;

  lua_pushstring(L, path);

  switch (type) {
    case WALK_F:   lua_pushstring(L, "F");   break;
    case WALK_D:   lua_pushstring(L, "D");   break;
    case WALK_DP:  lua_pushstring(L, "DP");  break;
    case WALK_SL:  lua_pushstring(L, "SL");  break;
    case WALK_NS:  lua_pushstring(L, "NS");  break;
    case WALK_DNR: lua_pushstring(L, "DNR"); break;
    default:
      return jot_error(L, "walkstring: unexpected return value %d", type);
  }

  lua_pushinteger(L, walkdir_size(pwalk));
  lua_pushinteger(L, walkdir_mtime(pwalk));

  return 4;
}


#define JOTLIB_WALKDIR_REGKEY "jotlib.walkdir"

int
jot_walkdir(lua_State *L)
{
  struct walk *pwalk;
  const char *path = luaL_checkstring(L, 1);
  int flags = lua_tointeger(L, 3);

  pwalk = lua_newuserdata(L, sizeof(*pwalk));
  assert(pwalk != NULL); // TODO null or error on ENOMEM?

  if (walkdir(pwalk, path, flags) != 0)
    return jot_error(L, "walkdir: %s", strerror(errno));

  luaL_getmetatable(L, JOTLIB_WALKDIR_REGKEY);
  lua_setmetatable(L, -2); /* udata's metatable; from now on __gc may be called */

  /* the iterator fun's upvalue, the struct, is already on stack */
  lua_pushcclosure(L, jot_walkdir_iter, 1);
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
  {"getcwd",    jot_getcwd},
  {"mkdir",     jot_mkdir},
  {"rmdir",     jot_rmdir},
  {"listdir",   jot_listdir},
  {"touch",     jot_touch},
  {"rename",    jot_rename},
  {"remove",    jot_remove},
  {"exists",    jot_exists},
  {"getinfo",   jot_getinfo},
  {"getenv",    jot_getenv},
  {"tempdir",   jot_tempdir},
  {"walkdir",   jot_walkdir},
  {"pikchr",    jot_pikchr},
  {0, 0}
};

extern int luaopen_loglib(lua_State *);
extern int luaopen_pathlib(lua_State *);

int
luaopen_jotlib(lua_State *L)
{
  char dirsep = getdirsep(L);
  log_trace("using '%c' as dirsep", dirsep);

  luaL_newmetatable(L, JOTLIB_WALKDIR_REGKEY);
  lua_pushcfunction(L, jot_walkdir_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  luaL_newlib(L, jotlib);

  luaopen_loglib(L);
  lua_setfield(L, -2, "log");

  luaopen_pathlib(L);
  lua_setfield(L, -2, "path");

  lua_pushstring(L, VERSION);
  lua_setfield(L, -2, "VERSION");

  return 1;
}
