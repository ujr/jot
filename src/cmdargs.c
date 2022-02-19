
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "cmdargs.h"


void
cmdargs_init(struct cmdargs *args, int argc, char **argv)
{
  assert(args != NULL);
  args->argc = argc;
  args->argv = argv;
  args->optind = 1;
  args->optpos = 1;
  args->optopt = 0;
  args->optarg = 0;
}


const char *
cmdargs_getprog(struct cmdargs *args)
{
  const char *s = 0;
  register const char *p;
  assert(args != NULL);
  if (args->argc > 0 && args->argv && *args->argv) {
    p = s = *args->argv;
    while (*p) {
      if (*p++ == '/') {
        if (p) s = p;
      }
    }
  }
  return s;
}


int
cmdargs_getopt(struct cmdargs *args, const char *optspec)
{
  const char dash = '-';
  const char colon = ':';

  const char *arg, *opt, *spec;

  assert(args != NULL);

  arg = args->argv[args->optind];

  args->optopt = 0;
  args->optarg = 0;

  if (!arg) {
    return -1;  /* no more args */
  }

  if (arg[0] != dash || arg[1] == 0) {
    return -1;  /* no more opts */
  }

  if (arg[0] == dash && arg[1] == dash && arg[2] == 0) {
    args->optind += 1;
    return -1;  /* no more opts after -- */
  }

  opt = arg + args->optpos;
  spec = strchr(optspec, opt[0]);
  args->optopt = opt[0];

  if (!spec || !spec[0]) {
    args->optind += 1;
    args->optpos = 1;
    return '?';  /* invalid option */
  }

  if (spec[1] == colon) {
    if (opt[1]) {
      args->optarg = opt + 1;
      args->optind += 1;
      args->optpos = 1;
      return args->optopt;
    }
    if (args->argv[args->optind+1]) {
      args->optarg = args->argv[args->optind + 1];
      args->optind += 2;
      args->optpos = 1;
      return args->optopt;
    }
    args->optind += 1;
    args->optpos = 1;
    return ':';  /* option requires an argument */
  }

  if (opt[1]) {
    args->optpos += 1;
  }
  else {
    args->optind += 1;
    args->optpos = 1;
  }
  return args->optopt;
}


const char *
cmdargs_getarg(struct cmdargs *args)
{
  assert(args != NULL);
  const char *arg = args->argv[args->optind];
  args->optpos = 1;
  if (arg) args->optind += 1;
  return arg;
}


int
cmdargs_numleft(struct cmdargs *args)
{
  assert(args != NULL);
  return args->argc - args->optind;
}


void
cmdargs_reset(struct cmdargs *args)
{
  assert(args != NULL);
  args->optind = 1;
  args->optpos = 1;
  args->optarg = 0;
  args->optopt = 0;
}
