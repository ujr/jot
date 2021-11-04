#define _POSIX_C_SOURCE 200112L  /* for lstat(2) */

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


/** convenience around strcmp(3) */
bool
streq(const char *s, const char *t)
{
  if (!s && !t) return true;
  if (!s || !t) return false;
  return strcmp(s, t) == 0;
}


/** strdup(s), which is not in ANSI C */
static char *
strcopy(const char *s)
{
  if (!s) return 0;
  size_t l = strlen(s);
  char *t = malloc(l+1);
  if (!t) return 0;
  return memcpy(t, s, l+1);
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
touchfile(const char *path, time_t ttime)
{
  struct utimbuf times, *ptimes;
  mode_t mode = 0666; /* rw-rw-rw- */
  log_trace("touchfile: ttime=%ld, path=%s", (long)ttime, path);
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
  log_trace("calling makedir %s", path);
  if (makedir(path) < 0)
    return failed(L, "mkdir %s: %s", path, strerror(errno));
  return ok(L);
}


static int
jot_rmdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  log_trace("calling dropdir %s", path);
  if (dropdir(path) < 0)
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


#define JOTLIB_WALKDIR_REGKEY "jotlib.walkdir"

//  Usage: for path, type, size, mtime in walkdir(path, flags, pat) do ... end
//
//  path:  entry point directory path
//  pat:   wildcard pattern
//  flags:
//    - MOUNT  -- do not cross mount points
//    - POST   -- traverse in post-order (default: pre-order)
//    - REG    -- yield regular files only (no directories etc)
//    - DIR    -- yield directories only (no files etc)
//    - FOLLOW -- follow symlinks (default: physical walk)

#include "pathbuf.h"

struct wdir { DIR *dp; struct wdir *next; };

struct walk {
  const char *root;
  const char *pat;
  int flags;
  struct pathbuf buf;
  struct wdir *top;
};

#define IS_DOT_OR_DOTDOT(s) (s[0]=='.'&&(s[1]==0||(s[1]=='.'&&s[2]==0)))

static bool pushdir(struct walk *pwalk, const char *path) {
  struct wdir *pw = malloc(sizeof(*pw));
  if (!pw) return false;
  pw->dp = opendir(path);
  if (!pw->dp) { free(pw); return false; }
  pw->next = pwalk->top;
  pwalk->top = pw;
  return true;
}

static void popdir(struct walk *pwalk) {
  struct wdir *top = pwalk->top;
  assert(top != NULL);
  (void) closedir(top->dp);
  pwalk->top = top->next;
  free(top);
}

static int
jot_walkdir_gc(lua_State *L)
{
  struct walk *pwalk = lua_touserdata(L, 1);
  if (pwalk) {
    log_trace("jot_walkdir_gc");
    if (pwalk->root) free((char*)pwalk->root);
    if (pwalk->pat) free((char*)pwalk->pat);
    pathbuf_free(&pwalk->buf);
    while (pwalk->top)
      popdir(pwalk);
  }
  return 0;
}

static int
jot_walkdir_iter(lua_State *L)
{
  const char *name, *path; // current base name and full path
  struct stat statbuf;
  struct dirent *e;
  struct walk *pwalk = lua_touserdata(L, lua_upvalueindex(1));

  // TODO match against pat and flags
  // TODO honor flag to not cross device boundary
  // TODO how to detect/break cycles?
  // TODO limit depth (and thus open DIRs)?

  int initialized = pwalk->buf.path != 0;
  if (!initialized) {
    name = "";
    path = pathbuf_init(&pwalk->buf, pwalk->root);
    log_trace("walkdir_iter: initialize (path=%s)", path);
    if (!path) jot_error(L, "walkdir: %s", strerror(errno));
    goto start;
  }

  while (pwalk->top) {
    errno = 0;
    if (!(e = readdir(pwalk->top->dp))) {
      if (errno)
        jot_error(L, "readdir: %s", strerror(errno));
      popdir(pwalk);
      pathbuf_pop(&pwalk->buf);
      continue;
    }
    name = e->d_name;
    if (IS_DOT_OR_DOTDOT(name)) continue;
    path = pathbuf_push(&pwalk->buf, name);

start:
    lua_pushstring(L, path);

    if (lstat(path, &statbuf) < 0) {
      if (errno != EACCES)
        jot_error(L, "walkdir: stat %s: %s", path, strerror(errno));
      lua_pushstring(L, "NS");
    }
    else if (S_ISDIR(statbuf.st_mode)) {
      bool ok = pushdir(pwalk, path);
      lua_pushstring(L, ok ? "D" : "DNR");
    }
    else {
      pathbuf_pop(&pwalk->buf);
      lua_pushstring(L, S_ISLNK(statbuf.st_mode) ? "SL" : "F");
    }

    lua_pushinteger(L, statbuf.st_size);
    lua_pushinteger(L, statbuf.st_mtime);
    return 4;
  }

  return 0; /* iteration done */
}

static int
jot_walkdir(lua_State *L)
{
  struct walk *pwalk;
  const char *path = luaL_checkstring(L, 1);
  int flags = lua_tointeger(L, 2);
  const char *pat = lua_tostring(L, 3);

  if (lua_gettop(L) > 1) // TODO relax for flags&pat
    return jot_error(L, "too many arguments");

  pwalk = lua_newuserdata(L, sizeof(*pwalk));
  assert(pwalk != NULL); // TODO null or error on ENOMEM?

  memset(pwalk, 0, sizeof(*pwalk));
  pwalk->root = strcopy(path);
  if (pat) pwalk->pat = strcopy(pat);
  pwalk->flags = flags;
  if (!pwalk->root || (pat && !pwalk->pat))
    return jot_error(L, "walkdir: %s", strerror(errno));

  luaL_getmetatable(L, JOTLIB_WALKDIR_REGKEY);
  lua_setmetatable(L, -2); /* udata's metatable; from now on __gc may be called */

  /* the iterator fun's upvalue, the struct, is already on stack */
  lua_pushcclosure(L, jot_walkdir_iter, 1);
  return 1;
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
  {"walkdir",   jot_walkdir},
  {"pikchr",    jot_pikchr},
  {0, 0}
};


int
luaopen_jotlib(lua_State *L)
{
  J.dirsep = getdirsep(L);
  log_trace("using '%c' as dirsep", J.dirsep);

  luaL_newmetatable(L, JOTLIB_WALKDIR_REGKEY);
  lua_pushcfunction(L, jot_walkdir_gc);
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
