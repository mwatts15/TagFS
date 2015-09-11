#ifndef SUBFS_H
#define SUBFS_H
#include <glib.h>
#include "params.h"

/* Tells us if the component should handle the path
 * returns 1 if it handles the path, 0 otherwise */
typedef int (*subfs_path_check_fn) (const char *path);

struct subfs_component {
    /** The name of the component */
    char *name;
    /** The file system operations that this component implements */
    struct fuse_operations operations;
    /** The component's path checker */
    subfs_path_check_fn path_checker;
    /** Init the subfs. Primarily, for initializing data that aren't shared by
     * other components
     */
    void (*init) (void);
};

typedef struct subfs_component subfs_component;

void subfs_init (void);

/** Calls the subfs initialization functions */
void subfs_init_components(void);

/** Register a subfs component */
void subfs_register_component (subfs_component *comp);

/** returns the fuse_operations that should be used for a path */
struct fuse_operations *subfs_get_opstruct (const char *path_to_check);

/** Checks the subfs_path_check_fn for each component and returns the index of
 * the first one that handles the path or -1 if none of them do */
int subfs_get_path_handler (const char *path);
gboolean subfs_component_handles_path (subfs_component *c, const char *path);
subfs_path_check_fn subfs_component_get_path_matcher (subfs_component *comp);
struct subfs_component *subfs_get_by_path (const char *path_to_check);
#define subfs_name(subfs_comp) ((subfs_comp)->name)

#endif /* SUBFS_H */
