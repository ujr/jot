#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef struct {
  void *userdata;
  struct tm *time;
  const char *file;
  const char *fmt;
  va_list ap;
  int line;
  int level;
} log_Event;

enum {
  LOG_TRACE = 0,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_PANIC,
  LOG_LEVELCOUNT
};

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN , __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_panic(...) log_log(LOG_PANIC, __FILE__, __LINE__, __VA_ARGS__)

void log_log(int level, const char *file, int line, const char *fmt, ...);
void log_set_level(int level);  /* all writers */
void log_set_quiet(bool quiet); /* silence stderr only */
void log_use_ansi(bool enable);
int log_add_stream(FILE *fp, int level);
const char *log_level_name(int level);

#endif
