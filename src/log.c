
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "log.h"


typedef void (*log_WriteFn)(log_Event *evt);

typedef struct {
  log_WriteFn fn;
  void *userdata;
  int threshold;
} Writer;


#define MAX_WRITERS 16

static struct {
  int threshold;
  int quiet;
  int use_ansi;
  Writer writers[MAX_WRITERS];
} Log;


#define ANSIOFF      "\x1b[0m"
#define ANSIDIM      "\x1b[90m"
#define ANSIRED      "\x1b[31m"
#define ANSIGREEN    "\x1b[32m"
#define ANSIYELLOW   "\x1b[33m"
#define ANSIBLUE     "\x1b[34m"
#define ANSIMAGENTA  "\x1b[35m"
#define ANSICYAN     "\x1b[36m"


static struct {
  const char *name;
  const char *ansi;
} Levels[LOG_LEVELCOUNT] = {
  { "TRACE", ANSIBLUE    },
  { "DEBUG", ANSICYAN    },
  { "INFO",  ANSIGREEN   },
  { "WARN",  ANSIYELLOW  },
  { "ERROR", ANSIRED     },
  { "PANIC", ANSIMAGENTA }
};


static inline int
clamp(int val, int min, int max) {
  if (val < min) return min;
  if (val > max) return max;
  return val;
}


static void
write_console(log_Event *evt)
{
  FILE *fp = stderr;
  int lvl = clamp(evt->level, LOG_TRACE, LOG_PANIC);
  char buf[32];
  buf[strftime(buf, sizeof buf, "%H:%M:%S", evt->time)] = '\0';
  if (Log.use_ansi) {
    fprintf(fp, "%s %s%-5s%s %s%s:%d:%s ",
      buf, Levels[lvl].ansi, Levels[lvl].name, ANSIOFF,
      ANSIDIM, evt->file, evt->line, ANSIOFF);
  }
  else {
    fprintf(fp, "%s %-5s %s:%d: ",
      buf, Levels[lvl].name, evt->file, evt->line);
  }
  vfprintf(fp, evt->fmt, evt->ap);
  fprintf(fp, "\n");
  fflush(fp);
}


static void
write_stream(log_Event *evt)
{
  FILE *fp = evt->userdata;
  int lvl = clamp(evt->level, LOG_TRACE, LOG_PANIC);
  char buf[64];
  buf[strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", evt->time)] = '\0';
  fprintf(fp, "%s %-5s %s:%d: ",
    buf, Levels[lvl].name, evt->file, evt->line);
  vfprintf(fp, evt->fmt, evt->ap);
  fprintf(fp, "\n");
  fflush(fp);
}


int
log_get_level(void)
{
  return Log.threshold;
}


void
log_set_level(int level)
{
  Log.threshold = level;
}


void
log_set_quiet(bool quiet)
{
  Log.quiet = quiet;
}


void
log_use_ansi(bool enable)
{
  Log.use_ansi = enable;
}


const char *
log_level_name(int level)
{
  return Levels[clamp(level, LOG_TRACE, LOG_PANIC)].name;
}


int
log_add_writer(log_WriteFn fn, void *userdata, int level)
{
  assert(fn != NULL);
  for (int i = 0; i < MAX_WRITERS; i++) {
    if (!Log.writers[i].fn) { // free slot
      Log.writers[i] = (Writer) { fn, userdata, level };
      return 0;
    }
  }
  return -1; // table full
}


int
log_add_stream(FILE *fp, int level)
{
  assert(fp != NULL);
  return log_add_writer(write_stream, fp, level);
}


void
log_log(int level, const char *file, int line, const char *fmt, ...)
{
  log_Event evt = {
    .level = level, .fmt = fmt, .file = file, .line = line
  };

  time_t t = time(0);
  evt.time = localtime(&t);
  evt.userdata = 0;

  if (!Log.quiet && level >= Log.threshold) {
    va_start(evt.ap, fmt);
    write_console(&evt);
    va_end(evt.ap);
  }

  for (int i = 0; i < MAX_WRITERS; i++) {
    Writer *writer = &Log.writers[i];
    if (!writer->fn) break;
    if (level >= writer->threshold) {
      evt.userdata = writer->userdata;
      va_start(evt.ap, fmt);
      writer->fn(&evt);
      va_end(evt.ap);
    }
  }
}
