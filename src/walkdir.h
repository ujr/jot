#ifndef WALKDIR_H
#define WALKDIR_H

#include <sys/stat.h>


/* return values */

#define WALK_ERR    -1
#define WALK_OK      0
#define WALK_DONE    WALK_OK

#define WALK_F       1   /* regular (or other) file */
#define WALK_D       2   /* directory (opening, pre-order) */
#define WALK_DP      3   /* directory (closing, post-order) */
#define WALK_SL      4   /* symbolic link */
#define WALK_NS      8   /* stat failed */
#define WALK_DNR     9   /* directory, but opendir failed */


/* flags */

#define WALK_FILE    1   /* yield (regular) files */
#define WALK_PRE     2   /* yield directories when opening them */
#define WALK_POST    4   /* yield directories when closing them */
#define WALK_LINK    8   /* yield symbolic links */

#define WALK_FOLLOW 16   /* follow symlinks (default: physical walk) */
#define WALK_MOUNT  32   /* do not cross mount points */
#define WALK_ADORN  64   /* adorn directories with a trailing '/' */


struct walk {
  int flags;
  int type;
  struct stat statbuf;
  struct wdir *top;
  char *path;
  size_t len;
  size_t size;
  size_t inilen;
};


#define walkdir_path(pwalk)  ((pwalk)->path)
#define walkdir_size(pwalk)  ((pwalk)->statbuf.st_size)
#define walkdir_mtime(pwalk) ((pwalk)->statbuf.st_mtime)

int walkdir(struct walk *pwalk, const char *path, int flags);
int walkdir_next(struct walk *pwalk);
void walkdir_free(struct walk *pwalk);


/* Usage: call walkdir() with desired starting path and flags;
   then repeatedly call waldir_next() to advance the walk;
   call walkdir_path(), walkdir_size(), walkdir_mtime() to
   retrieve information on the current file or directory;
   when done (even if by error) always call walkdir_free() */

#endif
