#define subfs_do_oper(path, oper_name, ...) \
    subfs_component _component = subfs_get_path_handler(path) \
    if (_component) \
        _component.operations.oper_name(__VA_ARGS__)

/* Tells us if the component should handle the path
 * returns 1 if it handles the path, 0 otherwise */
typedef int (*subfs_path_check_fn) (const char *path);

typedef struct {
    struct fuse_operations operations;
    subfs_path_check_fn path_checker;
} subfs_component;

/* Checks the subfs_path_check_fn for each component and returns the index of the
 * first one that handles the path or -1 if none of them do */
subfs_component subfs_get_path_handler (const char *path);
gboolean subfs_component_handles_path (subfs_component c, const char *path);
subfs_path_check_fn subfs_component_get_path_matcher (subfs_component comp);
