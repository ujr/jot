#define _POSIX_C_SOURCE 200112  /* readlink in unistd.h */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glob.h>

#include "jot.h"
#include "jotlib.h"
#include "log.h"
#include "blob.h"
#include "cmdargs.h"
#include "pikchr.h"
#include "markdown.h"

static void fatal(void);
#define BUF_ABORT fatal();
#include "buf.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


static const char *me = "jot";
static int verbosity = 2;  /* WARN and higher */


static void
fatal(void)
{
  if (errno)
    log_panic("Fatal error: %s", strerror(errno));
  else
    log_panic("Fatal error: cannot continue");
  exit(FAILSOFT);
}


static bool
get_exe_path(char *buf, int size)
{
#if __linux__
  char path[128];
  sprintf(path, "/proc/%d/exe", getpid());
  int len = readlink(path, buf, size - 1);
  if (len < 0 || len > size-1) return false;
  buf[len] = '\0';
  return true;
#else
  #error "get_exe_path for non-Linux not implemented"
#endif
}


static int
identify(void)
{
  printf("This is %s version %s using Lua %s.%s.%s\n",
  PRODUCT, VERSION,
  LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE);
  return SUCCESS;
}


static int
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
  else {
    fprintf(fp, "\nThis is %s version %s,\n"
    "the static site generator for the unpretentious,\n"
    "using Lua scripting and mustache-like templates.\n\n",
    PRODUCT, VERSION);
  }

  fprintf(fp, "Usage: %s [opts] <cmd> [opts] [args]\n", me);
  if (fmt)
    fprintf(fp, "Run %s with option -h for detailed usage.\n", me);
  else {
    fprintf(fp, "\nCommands:\n"
    "  new <path>      create initial site structure in <path>\n"
    "  build [path]    build or rebuild site in path (or .)\n"
    "  render [file]   render file (or stdin) to stdout\n"
    "  markdown [file] process Markdown to HTML on stdout\n"
    "  pikchr [file]   process Pikchr to SVG on stdout\n"
    "  checks          run some self checks and quit\n"
    "  trials          experimental code while in dev\n"
    "  help            show this help text\n"
    "\nGeneral options:\n"
    "  -v              increase verbosity\n"
    "  -q              quiet (log only errors)\n"
    "  -p num          flags for markdown/pikchr renderer\n"
    "  -x              allow unsafe functions (io.* etc.)\n"
    "  -h              show this help and quit\n"
    "  -V              show version and quit\n"
    "\nBuild options:\n"
    "  -c FILE         override config file location\n"
    "  -s DIR          source: build from DIR (override config)\n"
    "  -t DIR          target: build to DIR (override config)\n"
    "  -d              build draft posts\n"
    "\nRender options:\n"
    "  -i FILE.lua     load FILE.lua to init render env\n"
    "  -p FILE.tmpl    make FILE.tmpl available as {{>FILE.tmpl}}\n"
    "  -o FILE         render to FILE instead of stdout\n\n");
  }

  return fmt ? FAILHARD : SUCCESS;
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


static int
setup_lua(lua_State *L, const char *exepath)
{
  assert(L != NULL);

  log_trace("checking Lua app/core versions");
  lua_pushcfunction(L, check_lua_version);
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    log_panic("cannot initialize Lua: %s", err);
    return FAILHARD;
  }

  luaL_openlibs(L); // tentative: open all standard libraries

  lua_pushstring(L, VERSION);
  lua_setglobal(L, "JOT_VERSION");

  lua_pushfstring(L, "%s.%s.%s", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE);
  lua_setglobal(L, "LUA_VERSION");

  if (exepath) {
    lua_pushstring(L, exepath);
    lua_setglobal(L, "EXEPATH");
  }

  // lua_newtable(L);
  // for (i = 1; (arg = cmdargs_getarg(args)); i++) {
  //   lua_pushstring(L, arg);
  //   lua_rawseti(L, -2, i);
  // }
  // lua_setglobal(L, "ARGS");

  int r = luaL_dostring(L,
    "pcall(function()\n"
    "  EXEDIR = string.match(EXEPATH, \"^(.+)[/\\\\].*$\")\n"
    "  package.path = EXEDIR .. '/lua/?.lua;' .. EXEDIR .. '/lua/?/init.lua'\n"
    "  package.cpath = EXEDIR .. '/lua/?.so'\n"
    "end)");
  if (r) {
    log_error("error setting package.path per Lua");
    return FAILHARD;
  }

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

  return SUCCESS;
}


static const char *
strip_where(lua_State *L, const char *msg, int level)
{
  /* Hack: Lua's error() prepends "file:line" to the message,
     which is redundant to what our logging does; strip it */
  const char *w;
  size_t len;

  luaL_where(L, level);
  w = lua_tolstring(L, -1, &len);
  lua_pop(L, 1);

  if (strncmp(msg, w, len) == 0) {
    msg += len;
    while (*msg == ' ') msg++;
  }

  return msg;
}


/* Message handler: append a backtrace and log an error */
static int
msghandler(lua_State *L)
{
  const char *msg;
  const int level = 2;
  const char *file = "?";
  int line = 0;

  msg = lua_tostring(L, 1);
  if (msg) {
    msg = strip_where(L, msg, level);
    luaL_traceback(L, L, msg, level);
    msg = luaL_checkstring(L, 2);
  }
  else {
    /* error object is not a string: try convert via __tostring? */
    const char *tname = luaL_typename(L, 1);
    msg = lua_pushfstring(L, "(error object is a %s value)", tname);
    luaL_traceback(L, L, msg, level);
    msg = luaL_checkstring(L, 3);
  }

  lua_Debug debug;
  if (lua_getstack(L, level, &debug)) {
    lua_getinfo(L, "Sl", &debug);
    if (debug.currentline > 0) {
      file = basename(debug.short_src);
      line = debug.currentline;
    }
    if (debug.source && *debug.source != '@' && *debug.source != '=') {
      file = "(built-in code)";
    }
  }

  log_log(LOG_ERROR, file, line, "%s", msg);
  return 1;
}


/* run the code in the given Lua file (file name only, no dir, no ext) */
static int
runfile(lua_State *L, const char *module_name)
{
  // Here we do: dofile(package.searchpath(module_name, package.path))
  // We could also: package.loaded[module_name] = nil -- force module reload
  // and then: require(module_name)
  // but that would be misleading as we want to run code, not load a module

  int r, top;

  assert(module_name != NULL);

  top = lua_gettop(L);

  lua_pushstring(L, module_name);
  lua_setglobal(L, "JOT_MODULE"); // pops the stack

  lua_pushcfunction(L, msghandler);

  r = luaL_loadstring(L,
    "local jot = require 'jotlib'\n"
    "local path = assert(package.searchpath(JOT_MODULE, package.path))\n"
    "jot.log.debug(\"resolved '\" .. JOT_MODULE .. \"' as \" .. path)\n"
    "dofile(path)\n"
    );

  if (r != LUA_OK) {
    log_panic("Error loading built-in Lua code!");
    lua_settop(L, top);
    return r;
  }

  r = lua_pcall(L, 0, 0, -2);
  if (r != LUA_OK)
    log_error("Error running code in %s (err=%d)", module_name, r);

  lua_settop(L, top);
  return r;
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


/** read the named file into the given blob */
static int
readfile(const char *fn, Blob *blob)
{
  char buf[2048];
  size_t n;
  assert(blob != 0);

  if (fn && !streq(fn, "-")) {
    FILE *fp = fopen(fn, "r");
    if (!fp) {
      log_error("read file %s: %s", fn, strerror(errno));
      return FAILSOFT;
    }
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
      blob_addbuf(blob, buf, n);
    }
    fclose(fp);
  }
  else {
    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
      blob_addbuf(blob, buf, n);
    }
  }

  return SUCCESS;
}


static int
render(lua_State *L, struct cmdargs *args)
{
  const char *infn, *outfn = 0;
  const char **initbuf = 0;
  const char **partbuf = 0;
  bool sandbox = true;
  int opt, r, top;

  /* jot render {-i luafile} {-p partial} [-o outfile] [file] [args] */
  while ((opt = cmdargs_getopt(args, "i:o:p:qvx")) >= 0) {
    switch (opt) {
      case 'o':
        outfn = args->optarg;
        break;
      case 'i':
        buf_push(initbuf, args->optarg);
        break;
      case 'p':
        buf_push(partbuf, args->optarg);
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

  if (sandbox) setup_sandbox(L);
  else log_warn("sandbox disabled by option -x");

  // TODO should do things from about here in Lua!

  for (size_t i = 0; i < buf_size(initbuf); i++) {
    const char *fn = initbuf[i];
    log_debug("loading init file %s", fn);
    if ((r = luaL_dofile(L, fn)) != LUA_OK) {
      const char *err = lua_tostring(L, -1);
      if (!err) err = "luaL_dofile() failed";
      log_error("cannot load %s: %s", fn, err);
      return exitcode(r);
    }
  }

  top = lua_gettop(L);

  // This glob partial loading would be easier with Lua;
  // what is missing is glob() or fnmatch() and dirwalk().
  glob_t globbuf;
  globbuf.gl_offs = globbuf.gl_pathc = 0;
  for (size_t i = 0; i < buf_size(partbuf); i++) {
    const char *pat = partbuf[i];
    int flags = GLOB_MARK;
    if (i > 0) flags |= GLOB_APPEND;
    glob(pat, flags, 0, &globbuf);
  }

  lua_newtable(L);
  for (size_t i = 0; i < globbuf.gl_pathc; i++) {
    log_trace("found partial: %s", globbuf.gl_pathv[i]);
    lua_pushstring(L, globbuf.gl_pathv[i]);
    lua_rawseti(L, -2, 1+i);
  }

  if (globbuf.gl_pathc > 0)
    globfree(&globbuf);

  buf_free(initbuf);
  buf_free(partbuf);

  lua_pushcfunction(L, msghandler);
  lua_getglobal(L, "require");
  lua_pushstring(L, "render");
  r = lua_pcall(L, 1, 1, -3);
  if (r != LUA_OK)
    goto bail;
  lua_pushstring(L, infn);
  lua_pushstring(L, outfn);
  // stack: outfn infn chunk msghandler table
  lua_rotate(L, -5, -1); // pull partials table to top
  r = lua_pcall(L, 3, 0, -5);

bail:
  dump_stack(L, "stk.");
  lua_settop(L, top);
  return exitcode(r);
}


static int
render_markdown(lua_State *L, struct cmdargs *args)
{
  Blob input = BLOB_INIT;
  Blob output = BLOB_INIT;
  const char *infn;
  int opt, pretty = 0;
  int r = SUCCESS;
  UNUSED(L);

  /* jot markdown [-p pretty] [file] */
  while ((opt = cmdargs_getopt(args, "p:qv")) >= 0) {
    switch (opt) {
      case 'p': pretty = atoi(args->optarg); break;
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      default:
        return usage("invalid option: -%c", args->optopt);
    }
  }

  set_log_level(verbosity);

  infn = cmdargs_getarg(args);
  if (cmdargs_getarg(args))
    return usage("markdown: too many arguments");

  r = readfile(infn, &input);
  if (r != SUCCESS) goto done;
  if (!infn) infn = "(stdin)";

  mkdnhtml(&output, blob_str(&input), blob_len(&input), 0, pretty);

  fputs(blob_str(&output), stdout);
  fflush(stdout);

done:
  blob_free(&input);
  blob_free(&output);
  return r;
}


static int
render_pikchr(lua_State *L, struct cmdargs *args)
{
  const char *infn, *pik, *svg;
  const char *class = "pikchr";
  Blob input = BLOB_INIT;
  int opt, w, h, pretty = 0;
  int r = SUCCESS;
  unsigned flags;
  UNUSED(L);

  while ((opt = cmdargs_getopt(args, "p:qv")) >= 0) {
    switch (opt) {
      case 'p': pretty = atoi(args->optarg); break;
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      default:
        return usage("invalid option -%c", args->optopt);
    }
  }

  set_log_level(verbosity);

  infn = cmdargs_getarg(args);
  if (cmdargs_getarg(args))
    return usage("pikchr: too many arguments");

  r = readfile(infn, &input);
  if (r != SUCCESS) goto done;
  if (!infn) infn = "(stdin)";

  flags = PIKCHR_PLAINTEXT_ERRORS;
  if (pretty & 1) flags |= PIKCHR_DARK_MODE;

  pik = blob_str(&input);
  svg = pikchr(pik, class, flags, &w, &h);

  if (svg) {
    if (w >= 0) {
      log_debug("pikchr: w=%d h=%d", w, h);
      fputs(svg, stdout);
      fflush(stdout);
    }
    else log_error("pikchr error in %s:\n%s", infn, svg);

    free((void*)svg);
  }
  else {
    /* out of memory */
    perror(infn);
    r = FAILSOFT;
  }

done:
  blob_free(&input);
  return r;
}


static int
checks(lua_State *L, struct cmdargs *args)
{
  int opt;
  verbosity += 1;
  while ((opt = cmdargs_getopt(args, "qv")) >= 0) {
    switch (opt) {
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      default:
        return usage("invalid option -%c", args->optopt);
    }
  }
  set_log_level(verbosity);
  return exitcode(runfile(L, "checks"));
}


static int
trials(lua_State *L, struct cmdargs *args)
{
  int opt;
  verbosity += 1;
  while ((opt = cmdargs_getopt(args, "qv")) >= 0) {
    switch (opt) {
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      default:
        return usage("invalid option -%c", args->optopt);
    }
  }
  set_log_level(verbosity);
  return exitcode(runfile(L, "trials"));
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
        return usage(0);
      case 'q':
        verbosity = 0;
        break;
      case 'v':
        verbosity += 1;
        break;
      case 'V':
        return identify();
      case ':':
        return usage("option -%c requires an argument", args.optopt);
      case '?':
        return usage("invalid option: -%c", args.optopt);
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

  if (!get_exe_path(exepath, sizeof(exepath))) {
    if (errno)
      log_panic("Cannot determine path of executable: %s", strerror(errno));
    else log_panic("Cannot determine path of executable: cannot continue");
    return FAILHARD;
  }

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

  r = setup_lua(L, exepath);
  if (r) goto cleanup;

  if (streq(cmd, "new")) {
    log_error("command not yet implemented: %s", cmd);
    r = FAILSOFT;
  }
  else if (streq(cmd, "render")) {
    r = render(L, &args);
  }
  else if (streq(cmd, "build")) {
    log_error("command not yet implemented: %s", cmd);
    r = FAILSOFT;
  }
  else if (streq(cmd, "markdown") || streq(cmd, "mkdn")) {
    r = render_markdown(L, &args);
  }
  else if (streq(cmd, "pikchr")) {
    r = render_pikchr(L, &args);
  }
  else if (streq(cmd, "help")) {
    r = usage(0);
  }
  else if (streq(cmd, "check") || streq(cmd, "checks")) {
    r = checks(L, &args);
  }
  else if (streq(cmd, "trial") || streq(cmd, "trials")) {
    r = trials(L, &args);
  }
  else {
    r = usage("invalid command: %s", cmd);
  }

cleanup:
  log_trace("closing Lua state");
  lua_close(L);

  return r;
}
