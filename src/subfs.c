#include "subfs.h"

static int next_component_id = 0;
int subfs_number_of_components = 0;

subfs_component **subfs_comps = NULL;

void subfs_init (void)
{
    subfs_comps = g_malloc0_n(sizeof(subfs_component*), 20);
}

void subfs_init_components(void)
{
    for (int  i = 0; i < subfs_number_of_components; i++)
    {
        if (subfs_comps[i]->init)
        {
            subfs_comps[i]->init();
        }
    }
}

gboolean subfs_component_handles_path (subfs_component *comp, const char *path)
{
    return subfs_component_get_path_matcher(comp)(path);
}

int subfs_get_path_handler (const char *path)
{
    int i;
    for (i = 0; i < subfs_number_of_components; i++)
    {
        if (subfs_component_handles_path(subfs_comps[i], path))
        {
            return i;
        }
    }
    return -1;
}

struct fuse_operations *subfs_get_opstruct (const char *path_to_check)
{
    struct subfs_component *comp = subfs_get_by_path(path_to_check);
    if (comp != NULL)
    {
        return &comp->operations;
    }
    else
    {
        return NULL;
    }
}

struct subfs_component *subfs_get_by_path (const char *path_to_check)
{
    int component_number = subfs_get_path_handler(path_to_check);
    if (component_number >= 0)
    {
        return subfs_comps[component_number];
    }
    else
    {
        return NULL;
    }
}

subfs_path_check_fn subfs_component_get_path_matcher (subfs_component *comp)
{
    return comp->path_checker;
}

void subfs_register_component (subfs_component *comp)
{
    subfs_comps[next_component_id] = comp;
    next_component_id++;
    subfs_number_of_components++;
}
