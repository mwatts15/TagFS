#ifndef SUBFS_H
#define SUBFS_H
#include <glib.h>
#include "params.h"

/* Tells us if the component should handle the path
 * returns 1 if it handles the path, 0 otherwise */
typedef int (*subfs_path_check_fn) (const char *path);

struct subfs_component {
    struct fuse_operations operations;
    subfs_path_check_fn path_checker;
};

typedef struct subfs_component subfs_component;

/* returns the fuse_operations that should be used for a path
 */
struct fuse_operations subfs_get_opstruct (const char *path_to_check);
/* Checks the subfs_path_check_fn for each component and returns the index of the
 * first one that handles the path or -1 if none of them do */
int subfs_get_path_handler (const char *path);
gboolean subfs_component_handles_path (subfs_component c, const char *path);
subfs_path_check_fn subfs_component_get_path_matcher (subfs_component comp);

#endif /* SUBFS_H */
