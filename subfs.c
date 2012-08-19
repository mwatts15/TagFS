#include "subfs.h"
#include "search_fs.h"
#include "result_to_fs.h"

#define NUM_OF_COMPONENTS 2
#include "components.c"

static subfs_component subfs_comps[NUM_OF_COMPONENTS] = {
    tagdb_subfs,
    query_subfs,
};

gboolean subfs_component_handles_path (subfs_component comps, const char *path)
{
    return subfs_component_get_path_matcher(comp)(path);
}

struct fuse_operations subfs_get_opstruct (const char *path_to_check)
{
    int component_number = subfs_get_path_handler(path_to_check);
    return subfs_comps[component_number].operations;
}

int subfs_get_path_handler (const char *path)
{
    int i;
    for (i = 0; i < NUM_OF_COMPONENTS; i++)
    {
        if (subfs_component_handles_path(subfs_comps[i], path))
        {
            return i;
        }
    }
}

subfs_path_check_fn subfs_component_get_path_matcher (subfs_component comp)
{
    return comp.path_checker;
}
