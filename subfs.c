#include "subfs.h"
#include "search_fs.h"
#include "result_fs.h"

static int num_of_components = 3;
struct subfs_component subfs_comps[num_of_components] = {
    tagdb_oper,
    search_oper,
    result_oper,
};

gboolean subfs_component_handles_path (subfs_component comp, const char *path)
{
    return subfs_component_get_path_matcher(comp)(path);
}

subfs_component subfs_get_path_handler (const char *path)
{
    int i;
    for (i = 0; i < num_of_components; i++)
    {
        if (subfs_component_handles_path(subfs_comps[i], path))
        {
            return subfs_comps[i];
        }
    }
}
