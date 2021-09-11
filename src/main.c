#define _POSIX_C_SOURCE 200112  /* readlink in unistd.h */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "jot.h"
#include "jotlib.h"
#include "log.h"
#include "cmdargs.h"

static void panic(void);
#define BUF_ABORT panic();
#include "buf.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


static const char *me = "jot";
static int verbosity = 2;  /* WARN and higher */


static void
panic(void)
{
  if (errno)
    log_panic("Fatal error: %s", strerror(errno));
  else
    log_panic("Fatal error: cannot continue");
  exit(FAILSOFT);
}


static void
get_exe_path(char *buf, int size)
{
#if __linux__
  char path[128];
  sprintf(path, "/proc/%d/exe", getpid());
  int len = readlink(path, buf, size - 1);
  buf[len] = '\0';
#else
  #error "get_exe_path for non-Linux not implemented"
#endif
}


static bool
streq(const char *s, const char *t)
{
  if (!s && !t) return true;
  if (!s || !t) return false;
  return strcmp(s, t) == 0;
}


static void
identify(void)
{
  printf("This is %s version %s\n", PRODUCT, VERSION);
}


static void
usage(const char *fmt, ...)
{
  FILE *fp = fmt ? stderr : stdout;

  if (fmt) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(fp, "%s: ", me);
    vfprintf(fp, fmt, ap);
    fprintf(fp, "\n");
    va_end(ap);
  }

  fprintf(fp, "Usage: %s [opts] <cmd> [opts] [args]\n", me);
  fprintf(fp, "Commands:\n"
  "  new <path>     create initial site structure in <path>\n"
  "  build [path]   build or rebuild site in path (or .)\n"
  "  render [file]  render file (or stdin) to stdout\n"
  "  trials         experimental code while in dev\n"
  "  help           show this help text\n");
  fprintf(fp, "General options:\n"
  "  -v             increase verbosity\n"
  "  -q             quiet (log only errors)\n"
  "  -x             allow unsafe functions (io.* etc.)\n"
  "  -h             show this help and quit\n"
  "  -V             show version and quit\n");
  fprintf(fp, "Build options:\n"
  "  -c FILE        override config file location\n"
  "  -s DIR         source: build from DIR (override config)\n"
  "  -t DIR         target: build to DIR (override config)\n"
  "  -d             build draft posts\n");
  fprintf(fp, "Render options:\n"
  "  -o FILE        render to FILE instead of stdout\n"
  "  -i FILE.lua    load FILE.lua to init render env\n\n");
}


static void
set_log_level(int verbosity)
{
  static int level_map[] = {
    LOG_PANIC, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG, LOG_TRACE
  };

  const int n = sizeof(level_map)/sizeof(level_map[0]);

  if (verbosity >= n) verbosity = n - 1;
  else if (verbosity < 0) verbosity = 0;

  int level = level_map[verbosity];

  log_set_level(level);
  log_debug("verbosity=%d, loglevel=%s", verbosity, log_level_name(level));
}


static int
exitcode(int status)
{
  if (status == LUA_OK) return SUCCESS;
  if (status == LUA_YIELD) return SUCCESS;
  if (status == LUA_ERRMEM) return FAILSOFT;
  if (status == LUA_ERRFILE) return FAILSOFT;
  return FAILHARD; /* LUA_{ERRRUN, ERRERR, ERRSYNTAX} */
}


static int
check_lua_version(lua_State *L)
{
  // raises an error on app/core mismatch,
  // so call in protected mode (lua_pcall)
  luaL_checkversion(L);
  return 0;
}


static void
setup_lua(lua_State *L, struct cmdargs *args, const char *exepath)
{
  const char *arg;
  int i, r;

  assert(L != NULL);
  assert(args != NULL);
  assert(exepath != NULL);

  luaL_openlibs(L); // tentative: open all standard libraries

  lua_pushstring(L, VERSION);
  lua_setglobal(L, "JOT_VERSION");

  lua_pushfstring(L, "%s.%s.%s", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE);
  lua_setglobal(L, "LUA_VERSION");

  lua_pushstring(L, exepath);
  lua_setglobal(L, "EXEPATH");

  lua_newtable(L);
  for (i = 1; (arg = cmdargs_getarg(args)); i++) {
    lua_pushstring(L, arg);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "ARGS");

  r = luaL_dostring(L,
    "pcall(function()\n"
    "  EXEDIR = string.match(EXEPATH, \"^(.+)[/\\\\].*$\")\n"
    "  package.path = EXEDIR .. '/lua/?.lua;' .. EXEDIR .. '/lua/?/init.lua'\n"
    "  package.cpath = EXEDIR .. '/lua/?.so'\n"
    "end)");
  if (r) log_error("error setting package.path per Lua");

  // Preload jotlib, so Lua code can `require "jotlib"`
  // (note that luaL_requiref() does require, not only preload)
  log_trace("preload jotlib into Lua");
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  lua_pushcfunction(L, luaopen_jotlib);
  lua_setfield(L, -2, "jotlib");
  lua_pop(L, 1);

  // TODO prepare environment:
  // _G.options.*       (from cmd line, overrides config file)
  // _G.site.config.*   (from _config.lua file)
  // _G.site.{data,posts,layouts,partials,etc.}
  // Shall be read-only for page templates!
}


static void
setup_sandbox(lua_State *L)
{
  // TODO drop os.execute() and file system manipulation routines
  // http://lua-users.org/wiki/SandBoxes
  // https://github.com/kikito/sandbox.lua
  UNUSED(L);
  log_debug("sandbox not yet implemented");
}


static int
render(lua_State *L, struct cmdargs *args, const char *exepath)
{
  const char *infn, *outfn = 0;
  const char **initbuf = 0;
  bool sandbox = true;
  int opt, r;

  // jot render {-i luafile} [-o outfile] [file] [args]
  while ((opt = cmdargs_getopt(args, "i:o:qvx")) >= 0) {
    switch (opt) {
      case 'o':
        outfn = args->optarg;
        break;
      case 'i':
        buf_push(initbuf, args->optarg);
        break;
      case 'q':
        verbosity = 0;
        break;
      case 'v':
        verbosity += 1;
        break;
      case 'x':
        sandbox = false;
        break;
      case ':':
        usage("option -%c requires an argument", args->optopt);
        return FAILHARD;
      case '?':
        usage("invalid option: -%c", args->optopt);
        return FAILHARD;
    }
  }

  set_log_level(verbosity);

  infn = cmdargs_getarg(args);

  setup_lua(L, args, exepath);
  if (sandbox) setup_sandbox(L);
  else log_warn("sandbox disabled by option -x");

  for (size_t i = 0; i < buf_size(initbuf); i++) {
    const char *fn = initbuf[i];
    if ((r = luaL_dofile(L, fn)) != LUA_OK) {
      const char *err = lua_tostring(L, -1);
      if (!err) err = "luaL_dofile() failed";
      log_error("cannot load %s: %s", fn, err);
      return exitcode(r);
    }
  }

  buf_free(initbuf);

  r = luaL_dostring(L,
    "xpcall(function()\n"
    "  render = require 'render'\n"
    "end, function(err)\n"
    "  print('Error: ' .. tostring(err))\n"
    "  print(debug.traceback(nil, 2))\n"
    "end)\n");

  if (r == LUA_OK) {
    lua_getglobal(L, "render");
    lua_pushstring(L, infn); // pushes nil if infn is null
    lua_pushstring(L, outfn); // ditto
    lua_pushnil(L); // partials (but not today)
    r = lua_pcall(L, 3, 0, 0);
    if (r == LUA_ERRRUN) {
      const char *err = lua_tostring(L, 1);
      log_error("render failed: %s", err);
    }
  }

  return exitcode(r);
}


static int
trials(lua_State *L, struct cmdargs *args, const char *exepath)
{
  int r;
  setup_lua(L, args, exepath);
  r = luaL_dostring(L,
    "xpcall(function()\n"
    "  require 'trials'\n"
    "end, function(err)\n"
    "  print('Error: ' .. tostring(err))\n"
    "  print(debug.traceback(nil, 2))\n"
    "end)\n");
  return exitcode(r);
}


int
main(int argc, char **argv)
{
  struct cmdargs args;
  const char *cmd;
  int opt, r = SUCCESS;
  char exepath[1024];

  cmdargs_init(&args, argc, argv);
  me = cmdargs_getprog(&args);
  if (!me) return FAILHARD;

  /* Parse general options */

  while ((opt = cmdargs_getopt(&args, "c:do:s:t:hqvVx")) >= 0) {
    switch (opt) {
      case 'h':
        identify();
        usage(0);
        return SUCCESS;
      case 'q':
        verbosity = 0;
        break;
      case 'v':
        verbosity += 1;
        break;
      case 'V':
        identify();
        return SUCCESS;
      case ':':
        usage("option -%c requires an argument", args.optopt);
        return FAILHARD;
      case '?':
        usage("invalid option: -%c", args.optopt);
        return FAILHARD;
      default:
        fprintf(stderr, "%s: not yet implemented: -%c\n", me, args.optopt);
        return FAILSOFT;
    }
  }

  cmd = cmdargs_getarg(&args);
  if (!cmd) {
    usage("no command specified");
    return FAILHARD;
  }

  get_exe_path(exepath, sizeof(exepath));

  log_use_ansi(isatty(2));
  set_log_level(verbosity);
  log_debug("me = %s, exepath=%s", me, exepath);

  log_trace("creating Lua state");
  lua_State *L = luaL_newstate();
  if (!L) {
    const char *err = errno ? strerror(errno) : "unspecified error";
    log_panic("cannot initialize Lua: %s", err);
    return FAILSOFT;
  }

  log_trace("checking Lua app/core versions");
  lua_pushcfunction(L, check_lua_version);
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    log_panic("cannot initialize Lua: %s", err);
    return FAILHARD;
  }

  if (streq(cmd, "new")) {
    log_error("command not yet implemented: %s", cmd);
    r = FAILSOFT;
  }
  else if (streq(cmd, "render")) {
    r = render(L, &args, exepath);
  }
  else if (streq(cmd, "build")) {
    log_error("command not yet implemented: %s", cmd);
    r = FAILSOFT;
  }
  // TODO nice to have: shell
  else if (streq(cmd, "help")) {
    identify();
    usage(0);
    r = SUCCESS;
  }
  else if (streq(cmd, "trial") || streq(cmd, "trials")) {
    r = trials(L, &args, exepath);
  }
  else {
    usage("invalid command: %s", cmd);
    r = FAILHARD;
  }

  log_trace("closing Lua state");
  lua_close(L);

  return r;
}
