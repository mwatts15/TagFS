#include "params.h"
/* UntagFS is structured like this:
   renaming (moving) a file to the :X directory will remove all
   of the tags on the file. Listing :X shows the tags that would
   normally be listed one directory up from :X but also including
   the ones that would be excluded for being in the preceding path
   Ex:
     $ pwd
      /path/from/mount
     $ ls -1
      tag1
      tag2
      tag3
      some_file
      another_file
     $ ls -1 :X
      from      <
      mount     < from the path
      path      <
      tag1
      tag2
      tag3
   Moving a file to a directory under :X would remove only those tags
   which came after the :X (Ex: `mv some_file :X/tag2` removes tag2
   from some_file). Moving to :X would, of course, remove no tags.

   UntagFS is effectively read-only -- a user cannot delete or rename
   or create anything in UntagFS. */

int untag_fs_handles_path (const char *path)
{
    return path_has_component(path, UNTAG_FH);
}

int untag_fs_rename (const char *old_path, const char *new_path)
{
    /* get the original file or fail if it can't be found */
    /* get the tags to remove */
    /* remove the file from the DB remove the tags and add the file back */
}

int untag_fs_getattr (const char *path, struct stat *statbuf)
{
    statbuf->st_mode = DIR_PERMS;
    return 0;
}

int untag_fs_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *f_info)
{
    /* Get the path up to the UNTAG_FH, extract the key, and get the tags list.
       Get the path after the UNTAG_FH, ''             , ''
       Intersect the two tag lists.
       Remove the tags that have already been added _after_ the UNTAG_FH */

}
