
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
#include "blob.h"
#include "markdown.h"
#include "pikchr.h"
#include "utils.h"
#include "walkdir.h"
#include "wildmatch.h"


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
  luaL_pushfail(L);
  lua_pushvfstring(L, fmt, ap);
  va_end(ap);
  return 2;
}


/* fs.readfile(fn): string | nil errmsg errno */
static int
fs_readfile(lua_State *L)
{
  size_t r;
  luaL_Buffer buf;
  const char *fn = luaL_checkstring(L, 1);
  FILE *fp = fopen(fn, "rb");
  if (!fp)
    return luaL_fileresult(L, 0, fn);
  luaL_buffinit(L, &buf);
  do {  /* read entire file in chunks */
    char *p = luaL_prepbuffsize(&buf, LUAL_BUFFERSIZE);
    r = fread(p, sizeof(char), LUAL_BUFFERSIZE, fp);
    luaL_addsize(&buf, r);
  } while (r == LUAL_BUFFERSIZE);
  if (ferror(fp)) {
    int n = luaL_fileresult(L, 0, fn);
    fclose(fp);
    return n;
  }
  fclose(fp);
  luaL_pushresult(&buf);
  return 1;
}


/* fs.writefile(fn, s...): true | nil errmsg errno */
static int
fs_writefile(lua_State *L)
{
  int ok = 1, arg, nargs = lua_gettop(L);
  const char *fn = luaL_checkstring(L, 1);
  FILE *fp = fopen(fn, "wb");
  if (!fp)
    return luaL_fileresult(L, 0, fn);
  for (arg = 2; arg <= nargs; arg++) {
    size_t len;
    const char *s = lua_tolstring(L, arg, &len);
    ok = ok && fwrite(s, sizeof(char), len, fp) == len;
  }
  if (ferror(fp)) {
    int n = luaL_fileresult(L, 0, fn);
    fclose(fp);
    return n;
  }
  fclose(fp);
  lua_pushboolean(L, 1);
  return 1;
}


/** fs.getcwd(): path */
static int
fs_getcwd(lua_State *L)
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


/** fs.mkdir(path): true | nil errmsg */
static int
fs_mkdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  // TODO create missing parent dirs (like mkdir -p)
  log_trace("calling mkdir %s", path);
  if (mkdir(path, 0775) < 0)
    return failed(L, "mkdir %s: %s", path, strerror(errno));
  return ok(L);
}


/** fs.rmdir(path): true | nil errmsg */
static int
fs_rmdir(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  if (!lua_isnone(L, 2))
    return jot_error(L, "too many arguments");
  log_trace("calling rmdir %s", path);
  if (rmdir(path) < 0)
    return failed(L, "rmdir %s: %s", path, strerror(errno));
  return ok(L);
}


/** fs.listdir(path): table | nil errmsg */
static int
fs_listdir(lua_State *L)
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
    lua_pop(L, 1);  /* the table */
    return failed(L, "listdir %s: %s", path, strerror(errno));
  }

  closedir(dp);
  return 1;  /* the table */
}


/** fs.touch(path): true | nil errmsg */
static int
fs_touch(lua_State *L)
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


/** fs.remove(path): true | nil errmsg */
static int
fs_remove(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  if (lua_gettop(L) > 1)
    return jot_error(L, "too many arguments");
  if (remove(path) != 0)
    return failed(L, "remove %s: %s", path, strerror(errno));
  return ok(L);
}


/** fs.rename(old, new): true | nil errmsg */
static int
fs_rename(lua_State *L) {
  const char *oldname = luaL_checkstring(L, 1);
  const char *newname = luaL_checkstring(L, 2);
  if (lua_gettop(L) > 2)
    return jot_error(L, "too many arguments");
  if (rename(oldname, newname))
    return failed(L, "rename: %s", strerror(errno));
  return ok(L);
}


/** fs.exists(path, type=any): true|false */
static int
fs_exists(lua_State *L)
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


/** fs.getinfo(path, table=nil): table (type, size, mtime) */
static int
fs_getinfo(lua_State *L)
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


/** fs.tempdir(template): path | nil errmsg */
static int
fs_tempdir(lua_State *L)
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
fs_walkdir_gc(lua_State *L)
{
  struct walk *pwalk = lua_touserdata(L, 1);
  if (pwalk) {
    log_trace("fs_walkdir_gc");
    walkdir_free(pwalk);
  }
  return 0;
}


static int
fs_walkdir_iter(lua_State *L)
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

/** fs.walkdir(path, flags): iterator */
static int
fs_walkdir(lua_State *L)
{
  struct walk *pwalk;
  const char *path = luaL_checkstring(L, 1);
  int flags = lua_tointeger(L, 2);

  pwalk = lua_newuserdata(L, sizeof(*pwalk));
  assert(pwalk != NULL);

  if (walkdir(pwalk, path, flags) != 0)
    return jot_error(L, "walkdir: %s", strerror(errno));

  luaL_getmetatable(L, JOTLIB_WALKDIR_REGKEY);
  lua_setmetatable(L, -2); /* udata's metatable; from now on __gc may be called */

  /* the iterator fun's upvalue, the struct, is already on stack */
  lua_pushcclosure(L, fs_walkdir_iter, 1);
  return 1;
}


/* fs.glob(table, pat...): table | nil errmsg */
static int
fs_glob(lua_State *L)
{
  struct walk walk;
  int wflags = WALK_FILE | WALK_PRE | WALK_ADORN;
  const char *pat;
  const char *dir;
  size_t len, j;
  int i, n;

  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkstring(L, 2);

  /* get num of args before pushing ref to table as retval */
  n = lua_gettop(L);
  lua_pushvalue(L, 1);

  for (i = 2; i <= n; i++) {
    pat = luaL_checklstring(L, i, &len);
    /* find last dirsep before first wildcard */
    j = strcspn(pat, "*?[");
    if (j < len)
      while (j > 0 && pat[j-1] != '/') j--;
    if (j == len) {
      j = 0;
      dir = pat;  pat = "**";
    }
    else if (j > 0) {
      ((char*)pat)[j-1] = '\0';  /* HACK but works with Lua 5.4 */
      dir = pat;  pat = pat+j;
    }
    else { dir = "."; j=2; }
    log_debug("glob: walkdir(%s) and match against %s", dir, pat);
    if (walkdir(&walk, dir, wflags) != 0)
      return jot_error(L, "walkdir: %s", strerror(errno));

    int type;
    while ((type = walkdir_next(&walk)) > 0) {
      const char *path = walkdir_path(&walk);
      len = strlen(path);
      //log_debug("matching: %s against %s", pat, path+j);
      if (!*pat || (j < len && wildmatch(pat, path+j, WILD_PATHNAME | WILD_PERIOD))) {
        lua_pushstring(L, path);
        lua_seti(L, -2, luaL_len(L, -2)+1);
        //log_debug("matched: %s", path);
      }
    }
    walkdir_free(&walk);
    if (type < 0)
      return jot_error(L, "walkdir: %s", strerror(errno));
  }
  return 1;
}


static int
jot_split_iter(lua_State *L)
{
  const char *t, *sep;
  size_t tlen, seplen;
  int drop, trim;
  size_t index;
  int max, count;

  t = luaL_checklstring(L, lua_upvalueindex(1), &tlen);
  sep = luaL_checklstring(L, lua_upvalueindex(2), &seplen);
  drop = lua_toboolean(L, lua_upvalueindex(3));
  trim = lua_toboolean(L, lua_upvalueindex(4));
  max = lua_tointeger(L, lua_upvalueindex(5));
  index = luaL_checkinteger(L, lua_upvalueindex(6));
  count = luaL_checkinteger(L, lua_upvalueindex(7));

  while (index < tlen) {
    size_t end;
    const char *p = strstr(t+index, sep);
    end = p ? (size_t)(p-t) : tlen;

    if (max > 0 && ++count >= max) {
      lua_pushinteger(L, tlen);
      lua_replace(L, lua_upvalueindex(6));  /* update index */
      lua_pushlstring(L, t+index, tlen-index);
      return 1;
    }

    if (trim) {
      while (index < end && isSpace(t[index])) index++;
      while (end > index+1 && isSpace(t[end-1])) end--;
    }

    if (drop && index == end) {
      index = (p-t)+seplen;
      continue;
    }

    lua_pushinteger(L, (p-t)+seplen);
    lua_replace(L, lua_upvalueindex(6));  /* update index */

    lua_pushinteger(L, count);
    lua_replace(L, lua_upvalueindex(7));  /* update count */

    lua_pushlstring(L, t+index, end-index);
    return 1;
  }

  return 0;  /* no more parts */
}


/** jot.split(s, sep, opts): iterator */
static int
jot_split(lua_State *L)
{
  const char *sep;
  int drop, trim, max;
  int arg, nargs;

  luaL_checkstring(L, 1);
  sep = luaL_checkstring(L, 2);
  if (!*sep)
    return luaL_argerror(L, 2, "separator must not be empty");

  nargs = lua_gettop(L);
  drop = trim = false;
  max = -1;

  for (arg = 3; arg <= nargs; arg++) {
    if (lua_isinteger(L, arg) && max < 0) {
      max = lua_tointeger(L, arg);
      if (max < 0) max = 0;
      continue;
    }
    if (lua_isstring(L, arg)) {
      const char *opt = lua_tostring(L, arg);
      if (streq(opt, "trim")) trim = true;
      else if (streq(opt, "notrim")) trim = false;
      else if (streq(opt, "drop") || streq(opt, "dropempty")) drop = true;
      else if (streq(opt, "nodrop") || streq(opt, "nodropempty")) drop = false;
      else return luaL_argerror(L, arg, "expect 'dropempty' or 'trim'");
    }
    else return luaL_argerror(L, arg, "expect 'dropempty' or 'trim' or max (integer)");
  }

  log_trace("split: drop=%s, trim=%s, max=%d",
    drop ? "true" : "false", trim ? "true" : "false", max);

  luaL_checkstack(L, 7, "cannot push 7 args in split()");

  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_pushboolean(L, drop);
  lua_pushboolean(L, trim);
  lua_pushinteger(L, max);
  lua_pushinteger(L, 0);  /* index into string */
  lua_pushinteger(L, 0);  /* count against max */
  lua_pushcclosure(L, jot_split_iter, 7);
  return 1;
}


/** jot.getenv(name): string */
static int
jot_getenv(lua_State *L)
{
  const char *name = luaL_checkstring(L, 1);
  const char *value = getenv(name);
  lua_pushstring(L, value); /* pushes nil if value is null */
  return 1;
}


/** jot.pikchr(str): string wd ht | nil errmsg */
static int
jot_pikchr(lua_State *L)
{
  const char *s, *t;
  const char *class = "pikchr";
  int w, h, darkmode, r;
  unsigned int flags;

  s = luaL_checkstring(L, 1);
  darkmode = lua_toboolean(L, 2);

  flags = PIKCHR_PLAINTEXT_ERRORS;
  if (darkmode) flags |= PIKCHR_DARK_MODE;

  log_trace("calling pikchr()");
  t = pikchr(s, class, flags, &w, &h);

  if (!t) {
    return failed(L, "pikchr() returns null; out of memory?");
  }

  if (w < 0) {
    r = failed(L, "pikchr: %s", t);
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


/** jot.markdown(str): string */
static int
jot_markdown(lua_State *L)
{
  Blob blob = BLOB_INIT;
  Blob *pout = &blob;
  const char *s;
  size_t len;
  int pretty;

  s = luaL_checklstring(L, 1, &len);
  pretty = luaL_optinteger(L, 2, 0);
  log_trace("calling mkdnhtml()");
  mkdnhtml(pout, s, len, 0, pretty);

  s = blob_str(pout);
  len = blob_len(pout);
  lua_pushlstring(L, s, len);
  blob_free(pout);
  return 1;
}


/** jot.checkblob(boolean): true | nil errmsg */
static int
jot_checkblob(lua_State *L)
{
  int harder = lua_toboolean(L, 1);
  int ok = blob_check(harder);
  if (ok) {
    lua_pushboolean(L, 1);
    return 1;
  }
  return failed(L, "Blob self checks failed");
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
  lua_pop(L, 2);  /* package and config */
  return dirsep && *dirsep ? *dirsep : '/';
}


static const struct luaL_Reg fslib[] = {
  {"getcwd",    fs_getcwd    },
  {"mkdir",     fs_mkdir     },
  {"rmdir",     fs_rmdir     },
  {"touch",     fs_touch     },
  {"rename",    fs_rename    },
  {"remove",    fs_remove    },
  {"exists",    fs_exists    },
  {"getinfo",   fs_getinfo   },
  {"tempdir",   fs_tempdir   },
  {"listdir",   fs_listdir   },
  {"walkdir",   fs_walkdir   },
  {"glob",      fs_glob      },
  {"readfile",  fs_readfile  },
  {"writefile", fs_writefile },
  {0, 0}
};


static const struct luaL_Reg jotlib[] = {
  {"split",     jot_split     },
  {"getenv",    jot_getenv    },
  {"pikchr",    jot_pikchr    },
  {"markdown",  jot_markdown  },
  {"checkblob", jot_checkblob },
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
  lua_pushcfunction(L, fs_walkdir_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  luaL_newlib(L, jotlib);

  luaopen_loglib(L);
  lua_setfield(L, -2, "log");

  luaopen_pathlib(L);
  lua_setfield(L, -2, "path");

  luaL_newlib(L, fslib);
  lua_setfield(L, -2, "fs");

  lua_pushstring(L, VERSION);
  lua_setfield(L, -2, "VERSION");

  return 1;
}
