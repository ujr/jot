/* cmdargs - very similar to getopt(3) but:
 *
 * - keeps state in a user-provided struct
 * - does not print any error messages
 * - returns ':' for missing argument and '?' for invalid option
 * - can resume parsing after a non-option argument
 */
#ifndef CMDARGS_H
#define CMDARGS_H

struct cmdargs {
  char **argv;
  int argc;
  int optind;
  int optpos;
  int optopt;
  const char *optarg;
};

void cmdargs_init(struct cmdargs *opts, int argc, char **argv);
const char *cmdargs_getprog(struct cmdargs *opts);
int cmdargs_getopt(struct cmdargs *opts, const char *optspec);
const char *cmdargs_getarg(struct cmdargs *opts);
void cmdargs_reset(struct cmdargs *args);

#endif /* CMDARGS_H */
