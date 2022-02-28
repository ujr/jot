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
#include "pathbuf.h"
#include "walkdir.h"


#define WALK_START  999  /* distinct from other WALK_xxx constants */
#define IS_DOT_OR_DOTDOT(s) (s[0]=='.'&&(s[1]==0||(s[1]=='.'&&s[2]==0)))


struct wdir {
  struct wdir *next;
  DIR *dp;
  dev_t dev;
  ino_t ino;
  //int level;
  // TODO index into strbuf instead of pathbuf
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


/** prepare for file tree walk at given path */
int
walkdir(struct walk *pwalk, const char *path, int flags)
{
  int mask = WALK_FILE|WALK_LINK|WALK_PRE|WALK_POST;

  if (!pwalk || !path) {
    errno = EINVAL;
    return WALK_ERR;
  }

  if (!(flags & mask)) flags |= mask;

  memset(pwalk, 0, sizeof(*pwalk));
  pwalk->type = WALK_START;
  pwalk->flags = flags;

  if (!pathbuf_init(&pwalk->buf, path))
    return WALK_ERR;

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
    path = pathbuf_path(&pwalk->buf);
    goto start;
  }

  while (pwalk->top) {
    /* pop path unless we just entered a directory */
    if (pwalk->type != WALK_D)
      pathbuf_pop(&pwalk->buf);
skip:
    errno = 0;
    if (!(e = readdir(pwalk->top->dp))) {
      if (errno) return pwalk->type = WALK_ERR;
      popdir(pwalk);
      if (pwalk->flags & WALK_ADORN)
        pathbuf_adorn(&pwalk->buf);
      pwalk->type = WALK_DP;
      if (pwalk->flags & WALK_POST)
        return pwalk->type;
      continue;
    }
    if (IS_DOT_OR_DOTDOT(e->d_name)) goto skip;

    path = pathbuf_push(&pwalk->buf, e->d_name);
start:
    log_trace("walkdir: stat %s", path);
    if ((follow ? stat(path, pstat) : lstat(path, pstat)) < 0) {
      bool denied = errno == EACCES;
      return pwalk->type = denied ? WALK_NS : WALK_ERR;
    }

    if ((pwalk->flags & WALK_MOUNT) && crossdev(pwalk, pstat)) {
      continue;
    }
    if (looping(pwalk, pstat)) {
      continue;
    }

    if (S_ISDIR(pstat->st_mode)) {
      bool ok = pushdir(pwalk, path, pstat->st_dev, pstat->st_ino);
      if (pwalk->flags & WALK_ADORN)
        pathbuf_adorn(&pwalk->buf);
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
  pathbuf_free(&pwalk->buf);
  while (pwalk->top) popdir(pwalk);
  pwalk->type = WALK_DONE;
}
