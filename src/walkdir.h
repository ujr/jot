#ifndef WALKDIR_H
#define WALKDIR_H

#include <sys/stat.h>
#include "pathbuf.h"


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
  struct pathbuf buf;
  struct stat statbuf;
  struct wdir *top;
};


#define walkdir_path(pwalk) pathbuf_path(&(pwalk)->buf)
#define walkdir_size(pwalk) ((pwalk)->statbuf.st_size)
#define walkdir_mtime(pwalk) ((pwalk)->statbuf.st_mtime)

int walkdir(struct walk *pwalk, const char *path, int flags);
int walkdir_next(struct walk *pwalk);
void walkdir_free(struct walk *pwalk);


#endif
