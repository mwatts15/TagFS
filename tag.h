#ifndef TAG_H
#define TAG_H
#include <glib.h>
#include "util.h"
#include "types.h"
#include "abstract_file.h"

/* Tags are assigned to files and have a specific type associated with them.
   They take the place of directories in the file system, but represent
   attributes possessed by the files. */
typedef struct Tag
{
    AbstractFile base;

    /* The type of values for this tag
       See types.h for the types enum */
    int type;

    /* Default value for the tag */
    tagdb_value_t *default_value;

    /* Range of values allowed for the Integer typed value. Meaningless if the
       tag type is not Integer. */
    int min_value;
    int max_value;
    /* The super-tag as in the subtag table */
    struct Tag *parent;
    /* A map to child ids from tag names */
    GHashTable *children_by_name;
} Tag;

/* TagPathInfo is a list for which each entry is populated with the name of
 * a component in the source path and whether or not the component is a root
 * component
 */
typedef struct TPI TagPathInfo;

/* Information about a component of a
 * tag path
 */
typedef struct TPEI TagPathElementInfo;

/* Copying the way it's done in glib.
 * The struct has to be visible in order to do
 * stack allocation, but we just hide all of the names
 */
struct _TPIIterator
{
    /* private. */
    gpointer dummy1;
    int dummy2;
};

typedef struct _TPIIterator TagPathInfoIterator;

typedef void (*TagTraverseFunc)(Tag *, gpointer data);
void tag_traverse (Tag *t, TagTraverseFunc f, gpointer data);

#define TAG_PATH_SEPARATOR "::"
#define TPS TAG_PATH_SEPARATOR
#define TPS_LENGTH 2

/* Returns a copy of the default value for the tag, or if the default isn't set
   (i.e. equals NULL) returns a copy of the default for the tag type */
tagdb_value_t *tag_new_default (Tag *t);
gboolean tag_destroy (Tag *t);
Tag *new_tag (const char *name, int type, tagdb_value_t *default_value);
/* Establishes the subtag relationship, telling `child` its parent and
 * telling `t` its child.
 */
gboolean tag_set_subtag (Tag *t, Tag *child);
/* Removes the subtag relationship */
gboolean tag_remove_subtag (Tag *t, Tag *child);
/* Removes the subtag relationship, using the child's name */
gboolean tag_remove_subtag_s (Tag *t, const char *child_name);
/* Breaks apart a path and returns tag path info (actually a list) that
 * can be used to get at actual tags
 */
TagPathInfo *tag_process_path(const char *path);
/* Split at the right-most tag in the path. If the path separator can't be found
 * then NULL is returned and the path is left un-modified
 *
 * Modifies the string in-place.*/
char *tag_path_split_right1(char *path);

/* Frees the resources of the TagPathInfo pointer */
void tag_path_info_destroy(TagPathInfo *tpi);

gboolean tag_path_info_is_empty(TagPathInfo *tpi);
/* Returns TRUE if all of the tags in the path were resolved. `last` is
 * the last resolved tag in the path */
gboolean tag_path_info_add_tags (TagPathInfo *tpi, Tag *t, Tag **last);

/* Iterating through TagPathInfo */
void tag_path_info_iterator_init(TagPathInfoIterator *it, TagPathInfo *tpi);
/* Returns 0 if there isn't a next element */
gboolean tag_path_info_iterator_next(TagPathInfoIterator *it, TagPathElementInfo **store);
#define TPIL(__tpi, __it, __v) \
{ \
    if (__tpi != NULL) {\
    TagPathElementInfo *__v; \
    TagPathInfoIterator it; \
    tag_path_info_iterator_init(&__it, __tpi); \
    while (tag_path_info_iterator_next(&__it, &__v))

#define TPIL_END } }
#define TSUBL(__t, __it, __v) HL(tag_children(__t), __it, __, __v)
#define TSUBL_END HL_END
gboolean tag_path_element_info_has_parent(TagPathElementInfo *tpei);
const char *tag_path_element_info_name(TagPathElementInfo *tpei);
Tag *tag_path_element_info_get_tag(TagPathElementInfo *tpei);
void tag_path_element_info_set_tag(TagPathElementInfo *tpei, Tag *t);

TagPathElementInfo *tag_path_info_first_element(TagPathInfo *tpi);


/* The path has to start with the name of the current file */
Tag *tag_evaluate_path(Tag *t, const char *path);
Tag *tag_evaluate_path0(Tag *t, TagPathInfo *tpi);
char *tag_to_string (Tag *t, buffer_t buffer);
char *tag_to_string1 (Tag *t, char *buffer, size_t buffer_size);
void tag_set_name (Tag *t, const char *name);
unsigned long tag_number_of_children(Tag *t);
#define tag_name(_t) abstract_file_get_name((AbstractFile*) _t)
#define tag_id(_t) (((AbstractFile*)_t)->id)
#define tag_parent(__t) (((Tag*)__t)->parent)
#define tag_get_child(__t, __child_name) g_hash_table_lookup((__t)->children_by_name, (__child_name))
#define tag_has_child(__t, __child_name) g_hash_table_lookup_extended((__t)->children_by_name, (__child_name), NULL, NULL)
#define tag_children(__t) ((__t)->children_by_name)
#define tag_lock abstract_file_lock
#define tag_unlock abstract_file_unlock
#define tag_is_tag(__t) (abstract_file_get_type((__t))==abstract_file_tag_type)

#endif /* TAG_H */
