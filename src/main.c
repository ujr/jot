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
#if defined(__linux__)
  char path[128];
  sprintf(path, "/proc/%d/exe", getpid());
  int len = readlink(path, buf, size - 1);
  if (len < 0 || len > size-1) return false;
  buf[len] = '\0';
  return true;
#elif defined(_WIN32)
  /* note: _WIN32 is also defined for 64bit Windows */
  #error "get_exe_path for Windows not yet implemented"
#elif defined(__APPLE__)
  #error "get_exe_path for macOS not yet implemented"
#elif defined(__unix__)
  /* note: readlink(2) is POSIX, but /proc/PID/exe is Linux */
  #error "get_exe_path for Unix (not Linux) not yet implemented"
#else
  strcpy(buf, "./jot");  /* last resort? */
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
    "  -l FILES        load Lua file(s) to init render env\n"
    "  -p GLOBS        load partials and expose through {{>FILE}}\n"
    "  -o FILE         render to FILE instead of stdout\n"
    "\nWhere supported, multiple files or globs are ;-separated\n\n");
  }

  return fmt ? FAILHARD : SUCCESS;
}


static int
help(const char *cmd)
{
  FILE *fp = stdout;
  if (!cmd || !*cmd) {
    fprintf(fp,
      "\nThis is %s version %s,\n"
      "the static site generator for the unpretentious,\n"
      "using Lua scripting and mustache-like templates.\n\n",
      PRODUCT, VERSION);
    fprintf(fp, "Usage: %s [opts] <cmd> [opts] [args]\n", me);
    fprintf(fp, "Run  %s <cmd> -h  for help on <cmd>\n", me);
  }
  else if (streq(cmd, "checks") || streq(cmd, "check")) {
    fprintf(fp,
      "\nRun built-in self checks. Depending on verbosity (as\n"
      "controlled by the options -v and -q) this may produce\n"
      "from no output to lots of output. If (and only if) all\n"
      "checks are successful, %s returns status %d.\n\n",
      me, SUCCESS);
  }
  // TODO help on other commands
  else {
    fprintf(fp,
      "\nCommand %s has no help available, I'm sorry.\n\n", cmd);
  }
  return ferror(fp) ? FAILSOFT : SUCCESS;
}


/** set log level from given verbosity */
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


/** map Lua status to exit code */
static int
exitcode(int status)
{
  if (status == LUA_OK) return SUCCESS;
  if (status == LUA_YIELD) return SUCCESS;
  if (status == LUA_ERRMEM) return FAILSOFT;
  if (status == LUA_ERRFILE) return FAILSOFT;
  return FAILHARD; /* LUA_{ERRRUN, ERRERR, ERRSYNTAX} */
}


/** raise an error on Lua app/core mismatch */
static int
check_lua_version(lua_State *L)
{
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


static void
loadargs(lua_State *L, struct cmdargs *args)
{
  const char *arg;
  lua_Integer i;

  /* expect table on top of stack */
  luaL_checktype(L, -1, LUA_TTABLE);

  /* set remaining args in table as 1, 2, ... */
  for (i = 1; (arg = cmdargs_getarg(args)); i++) {
    lua_pushstring(L, arg);
    lua_rawseti(L, -2, i);
  }
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


/* like luaL_dostring but allow passing nargs and nrets */
static void
runcode(lua_State *L, int nargs, int nrets, const char *lua)
{
  int rc = luaL_loadstring(L, lua);
  if (rc == LUA_OK) {
    lua_rotate(L, -(nargs+1), 1);  /* arg1 .. argN chunk => chunk arg1 .. argN */
    lua_call(L, nargs, nrets);
  }
  else luaL_error(L, "error loading built-in Lua code (err=%d)", rc);
}


/* run the code in the given Lua file (file name only, no dir, no ext) */
static void
runfile(lua_State *L, const char *module_name, int nargs, int nrets)
{
  // Here we do: dofile(package.searchpath(module_name, package.path))
  // Could also: package.loaded[module_name] = nil -- force module reload
  //   and then: require(module_name)
  // but that would be misleading as we want to run code, not load a module

  const char *path;
  int rc;

  assert(module_name != NULL);
  lua_pushstring(L, module_name);

  runcode(L, 1, 1,
    "local name = ...\n"  /* get argument from stack */
    "return assert(package.searchpath(name, package.path))\n");

  path = luaL_checkstring(L, -1);
  log_debug("resolved '%s' as %s", module_name, path);
  rc = luaL_loadfile(L, path);
  if (rc == LUA_OK) {
    /* arg1 .. argN path chunk => chunk arg1 .. argN path, then pop path */
    lua_rotate(L, -(nargs+2), 1);
    lua_pop(L, 1);
    lua_call(L, nargs, nrets);
  }
  else luaL_error(L, "error loading Lua code from %s (err=%d)", path, rc);
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


/** write the blob to the named file (overwrite if exists) */
static int
writefile(const char *fn, const char *text)
{
  FILE *fp = stdout;
  assert(text != 0);

  if (fn && !streq(fn, "-")) {
    fp = fopen(fn, "w");
    if (!fp) {
      log_error("write file %s: %s", fn, strerror(errno));
      return FAILSOFT;
    }
  }
  fputs(text, fp);
  fflush(fp);
  if (fp != stdout)
    fclose(fp);
  if (ferror(fp)) {
    log_error("write file %s: %s", fn,
      errno ? strerror(errno) : "unspecified error");
    return FAILSOFT;
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
  int opt, num, top, r;

  /* jot render {-i luafile} {-p partial} [-o outfile] [file] [args] */
  while ((opt = cmdargs_getopt(args, "i:o:p:qvx")) >= 0) {
    switch (opt) {
      case 'o': outfn = args->optarg; break;
      case 'i': buf_push(initbuf, args->optarg); break;
      case 'p': buf_push(partbuf, args->optarg); break;
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      case 'x': sandbox = false; break;
      case ':':
        return usage("option -%c requires an argument", args->optopt);
      case '?':
        return usage("invalid option: -%c", args->optopt);
    }
  }

  set_log_level(verbosity);

  infn = cmdargs_getarg(args);
  num = cmdargs_numleft(args);

  lua_createtable(L, num, 0);
  loadargs(L, args);
  lua_setglobal(L, "ARGS");
  // TODO expose through jot.args[]

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


/** jot markdown [-o outfile] [-p pretty] [file] */
static int
domarkdown(lua_State *L)
{
  Blob input = BLOB_INIT;
  Blob output = BLOB_INIT;
  const char *infn, *outfn;
  int r, pretty = 0;

  runcode(L, 1, 4,
    "local args = ...\n"
    "if type(args) ~= 'table' then args = {} end\n"
    "local infn = args[1]\n"
    "local outfn = args['o']\n"
    "local pretty = args['p'] or '0'\n"
    "local extra = #args > 1 and true or false\n"
    "return infn, outfn, pretty");

  infn = lua_tostring(L, -4);
  outfn = lua_tostring(L, -3);
  pretty = atoi(luaL_checkstring(L, -2));
  if (lua_toboolean(L, -1))
    return usage("markdown: too many arguments");

  r = readfile(infn, &input);
  if (r != SUCCESS) goto done;
  if (!infn) infn = "(stdin)";

  log_trace("calling mkdnhtml()");
  mkdnhtml(&output, blob_str(&input), blob_len(&input), 0, pretty);

  r = writefile(outfn, blob_str(&output));

done:
  blob_free(&input);
  blob_free(&output);
  return r;
}


/** jot pikchr [-o outfile] [-p pretty] [file] */
static int
dopikchr(lua_State *L)
{
  const char *infn, *outfn, *pik, *svg;
  const char *class = "pikchr";
  Blob input = BLOB_INIT;
  int r, w, h, pretty = 0;
  unsigned flags;

  runcode(L, 1, 4,
    "local args = ...\n"
    "if type(args) ~= 'table' then args = {} end\n"
    "local infn = args[1]\n"
    "local outfn = args['o']\n"
    "local pretty = args['p'] or '0'\n"
    "local extra = #args > 1 and true or false\n"
    "return infn, outfn, pretty, extra");

  infn = lua_tostring(L, -4);
  outfn = lua_tostring(L, -3);
  pretty = atoi(luaL_checkstring(L, -2));
  if (lua_toboolean(L, -1))
    return usage("pikchr: too many arguments");

  r = readfile(infn, &input);
  if (r != SUCCESS) goto done;
  if (!infn) infn = "(stdin)";

  flags = PIKCHR_PLAINTEXT_ERRORS;
  if (pretty & 1) flags |= PIKCHR_DARK_MODE;

  pik = blob_str(&input);
  log_trace("calling pikchr()");
  svg = pikchr(pik, class, flags, &w, &h);

  if (svg) {
    if (w >= 0) {
      log_debug("pikchr: w=%d h=%d", w, h);
      r = writefile(outfn, svg);
    }
    else {
      log_error("pikchr error in %s:\n%s", infn, svg);
      r = FAILHARD;
    }
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


/** jot checks */
static int
dochecks(lua_State *L)
{
  runfile(L, "checks", 1, 0);
  return 0;
}


/** jot trials */
static int
dotrials(lua_State *L)
{
  runfile(L, "trials", 1, 0);
  return 0;
}


static int
docmd(
  lua_State *L, const char *cmd, int (*fun)(lua_State*),
  struct cmdargs *args, const char *optspec)
{
  const char *arg;
  int opt, index, r;

  /* process args into a mixed table like this:
     { cmd, arg1, ..., ["opt"] = optarg, ... } */

  lua_newtable(L);

  index = 0;
  lua_pushstring(L, cmd);
  lua_rawseti(L, -2, index++);

  while ((opt = cmdargs_getopt(args, optspec)) >= 0) {
    switch (opt) {
      case 'h': return help(cmd);
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      case ':': return usage("option -%c requires an argument", args->optopt);
      case '?': return usage("invalid option -%c", args->optopt);
      default:
        lua_pushfstring(L, "%c", args->optopt);  /* key */
        lua_pushstring(L, args->optarg);  /* value (may be null) */
        lua_rawset(L, -3);
    }
  }

  while ((arg = cmdargs_getarg(args))) {
    lua_pushstring(L, arg);
    lua_rawseti(L, -2, index++);
  }

  set_log_level(verbosity);

  log_debug("doing pcall(%s)", cmd);
  lua_pushcfunction(L, msghandler);
  lua_pushcfunction(L, fun);
  lua_rotate(L, -3, 2);  /* reorder to msgh-fun-argtab */
  r = lua_pcall(L, 1, 0, -3);
  log_trace("pcall(%s) returned %d", cmd, r);

  return exitcode(r);
}


PUBLIC int
main(int argc, char **argv)
{
  struct cmdargs args;
  const char *cmd;
  int opt, s = SUCCESS;
  char exepath[1024];

  cmdargs_init(&args, argc, argv);
  me = cmdargs_getprog(&args);
  if (!me) return FAILHARD;

  /* parse general options */
  while ((opt = cmdargs_getopt(&args, "c:do:s:t:hqvVx")) >= 0) {
    switch (opt) {
      case 'h': return usage(0);
      case 'q': verbosity = 0; break;
      case 'v': verbosity += 1; break;
      case 'V': return identify();
      case ':': return usage("option -%c requires an argument", args.optopt);
      case '?': return usage("invalid option: -%c", args.optopt);
      default:  return usage("option -%c is not yet implemented", args.optopt);
    }
  }

  /* command is a required argument */
  cmd = cmdargs_getarg(&args);
  if (!cmd)
    return usage("no command specified");

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

  s = setup_lua(L, exepath);
  if (s) goto cleanup;

  if (streq(cmd, "new")) {
    log_error("command not yet implemented: %s", cmd);
    s = FAILSOFT;
  }
  else if (streq(cmd, "build")) {
    log_error("command not yet implemented: %s", cmd);
    s = FAILSOFT;
  }
  else if (streq(cmd, "render")) {
    // TODO s = docmd(L, cmd, render, &args, "l:p:o:hqv");
    s = render(L, &args);
  }
  else if (streq(cmd, "markdown") || streq(cmd, "mkdn")) {
    s = docmd(L, cmd, domarkdown, &args, "o:p:hqv");
  }
  else if (streq(cmd, "pikchr")) {
    s = docmd(L, cmd, dopikchr, &args, "o:p:hqv");
  }
  else if (streq(cmd, "help")) {
    s = help(0);
  }
  else if (streq(cmd, "check") || streq(cmd, "checks")) {
    s = docmd(L, cmd, dochecks, &args, "hqv");
  }
  else if (streq(cmd, "trial") || streq(cmd, "trials")) {
    s = docmd(L, cmd, dotrials, &args, "hqv");
  }
  else {
    s = usage("invalid command: %s", cmd);
  }

cleanup:
  log_trace("closing Lua state");
  lua_close(L);

  return s;
}
