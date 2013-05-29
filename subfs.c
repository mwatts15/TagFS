#include "subfs.h"
#include "search_fs.h"
#include "result_to_fs.h"

#include "components.c"

gboolean subfs_component_handles_path (subfs_component comp, const char *path)
{
    return subfs_component_get_path_matcher(comp)(path);
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

struct fuse_operations subfs_get_opstruct (const char *path_to_check)
{
    int component_number = subfs_get_path_handler(path_to_check);
    return subfs_comps[component_number].operations;
}

subfs_path_check_fn subfs_component_get_path_matcher (subfs_component comp)
{
    return comp.path_checker;
}
