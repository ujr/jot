/* Walk a directory tree */

#define _POSIX_C_SOURCE 200112L  /* for lstat(2) */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "walkdir.h"

#define DIRSEP      '/'
#define INITSIZE    512
#define WALK_START  999  /* distinct from other WALK_xxx constants */
#define IS_DOT_OR_DOTDOT(s) (s[0]=='.'&&(s[1]==0||(s[1]=='.'&&s[2]==0)))


struct wdir {
  struct wdir *next;
  DIR *dp;
  dev_t dev;
  ino_t ino;
  size_t pathlen;
};


static bool
pushdir(struct walk *pwalk, const char *path, dev_t dev, ino_t ino)
{
  struct wdir *pw = malloc(sizeof(*pw));
  if (!pw) return false;
  log_trace("walkdir: opendir %s", path);
  pw->dp = opendir(path);
  if (!pw->dp) { free(pw); return false; }
  pw->dev = dev;
  pw->ino = ino;
  pw->next = pwalk->top;
  pwalk->top = pw;
  return true;
}


static void
popdir(struct walk *pwalk)
{
  struct wdir *top = pwalk->top;
  assert(top != NULL);
  (void) closedir(top->dp);
  pwalk->top = top->next;
  free(top);
}


static bool
looping(struct walk *pwalk, struct stat *pstat)
{
  struct wdir *p;
  for (p = pwalk->top; p; p = p->next) {
    if (p->ino == pstat->st_ino && p->dev == pstat->st_dev)
      return true;
  }
  return false;
}


static bool
crossdev(struct walk *pwalk, struct stat *pstat)
{
  struct wdir *p = pwalk->top;
  return p && p->dev != pstat->st_dev;
}


static char *
pathgrow(struct walk *pwalk, size_t more)
{
  char *p;
  size_t n = pwalk->len + more + 1;
  if (n < 2*pwalk->size) n = 2*pwalk->size;
  p = realloc(pwalk->path, n);
  if (!p) return 0;
  pwalk->path = p;
  pwalk->size = n;
  return p;
}


static char *
pathappend(struct walk *pwalk, const char *name)
{
  size_t len;
  while (*name == DIRSEP) ++name;
  len = strlen(name);
  while (len && name[len-1] == DIRSEP) --len;
  if (pwalk->len + 1 + len + 1 > pwalk->size)
    if (!pathgrow(pwalk, 1+len))
      return 0;
  /* undo an eventual adorn (trailing DIRSEP) */
  if (pwalk->len > pwalk->inilen && pwalk->path[pwalk->len-1] == DIRSEP)
    pwalk->len--;
  /* append a DIRSEP, then the new name */
  pwalk->path[pwalk->len++] = DIRSEP;
  memcpy(pwalk->path + pwalk->len, name, len);
  pwalk->len += len;
  pwalk->path[pwalk->len] = '\0';
  return pwalk->path;
}


static char *
pathadorn(struct walk *pwalk)
{
  if (pwalk->len > pwalk->inilen && pwalk->path[pwalk->len-1] == DIRSEP)
    return pwalk->path;
  if (pwalk->len + 1 + 1 > pwalk->size)
    if (!pathgrow(pwalk, 1))
      return 0;
  pwalk->path[pwalk->len++] = DIRSEP;
  pwalk->path[pwalk->len] = '\0';
  return pwalk->path;
}


/** prepare for file tree walk at given path */
int
walkdir(struct walk *pwalk, const char *path, int flags)
{
  size_t len;
  const int mask = WALK_FILE|WALK_LINK|WALK_PRE|WALK_POST;

  if (!pwalk) {
    errno = EINVAL;
    return WALK_ERR;
  }

  if (!path || !*path) path = ".";
  if (!(flags & mask)) flags |= mask;

  len = strlen(path);
  while (len && path[len-1] == DIRSEP) --len;

  memset(pwalk, 0, sizeof(*pwalk));
  pwalk->type = WALK_START;
  pwalk->flags = flags;

  pwalk->size = len+51;  /* some extra space */
  if (pwalk->size < INITSIZE)
    pwalk->size = INITSIZE;
  pwalk->path = malloc(pwalk->size);
  if (!pwalk->path)
    return WALK_ERR;
  memcpy(pwalk->path, path, len);
  pwalk->path[len] = '\0';
  pwalk->inilen = len;
  pwalk->len = len;

  return WALK_OK;
}


/** advance to next; return one of WALK_xxx constants */
int
walkdir_next(struct walk *pwalk)
{
  const char *path;
  struct dirent *e;
  struct stat *pstat;
  bool follow;

  if (!pwalk) {
    errno = EINVAL;
    return WALK_ERR;
  }

  follow = pwalk->flags & WALK_FOLLOW;
  pstat = &pwalk->statbuf;

  if (pwalk->type == WALK_START) {
    path = pwalk->path;
    goto start;
  }

  while (pwalk->top) {
    /* pop path unless we just entered a directory */
    if (pwalk->type != WALK_D) {
      pwalk->len = pwalk->top->pathlen;
      pwalk->path[pwalk->len] = '\0';
    }
skip:
    errno = 0;
    if (!(e = readdir(pwalk->top->dp))) {
      if (errno) return pwalk->type = WALK_ERR;
      popdir(pwalk);
      if (pwalk->flags & WALK_ADORN)
        if (!pathadorn(pwalk))
          return WALK_ERR;
      pwalk->type = WALK_DP;
      if (pwalk->flags & WALK_POST)
        return pwalk->type;
      continue;
    }
    if (IS_DOT_OR_DOTDOT(e->d_name)) goto skip;

    path = pathappend(pwalk, e->d_name);
    if (!path) return WALK_ERR;
start:
    log_trace("walkdir: stat %s", path);
    if ((follow ? stat(path, pstat) : lstat(path, pstat)) < 0) {
      bool denied = errno == EACCES;
      return pwalk->type = denied ? WALK_NS : WALK_ERR;
    }

    if ((pwalk->flags & WALK_MOUNT) && crossdev(pwalk, pstat))
      continue;
    if (looping(pwalk, pstat))
      continue;

    if (S_ISDIR(pstat->st_mode)) {
      bool ok = pushdir(pwalk, path, pstat->st_dev, pstat->st_ino);
      if (ok) pwalk->top->pathlen = pwalk->len;
      if (pwalk->flags & WALK_ADORN)
        if (!pathadorn(pwalk))
          return WALK_ERR;
      pwalk->type = ok ? WALK_D : WALK_DNR;
      if (!ok || pwalk->flags & WALK_PRE)
        return pwalk->type;
    }
    else if (S_ISLNK(pstat->st_mode)) {
      pwalk->type = WALK_SL;
      if (pwalk->flags & WALK_LINK)
        return pwalk->type;
    }
    else {
      pwalk->type = WALK_F;
      if (pwalk->flags & WALK_FILE)
        return pwalk->type;
    }
  }

  return WALK_DONE;
}


/** release all memory and handles */
void
walkdir_free(struct walk *pwalk)
{
  if (!pwalk) return;
  free(pwalk->path);
  pwalk->path = 0;
  pwalk->size = 0;
  pwalk->len = pwalk->inilen = 0;
  while (pwalk->top)
    popdir(pwalk);
  pwalk->type = WALK_DONE;
}
