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

/* Returns a copy of the default value for the tag, or if the default isn't set
   (i.e. equals NULL) returns a copy of the default for the tag type */
tagdb_value_t *tag_new_default (Tag *t);
void tag_destroy (Tag *t);
Tag *new_tag (const char *name, int type, tagdb_value_t *default_value);
void tag_set_subtag (Tag *t, Tag *child);
char *tag_to_string (Tag *t, buffer_t buffer);
void tag_set_name (Tag *t, const char *name);
unsigned long tag_number_of_children(Tag *t);
#define tag_name(_t) abstract_file_get_name((AbstractFile*) _t)
#define tag_id(_t) (((AbstractFile*)_t)->id)
#define tag_parent(__t) ((__t)->parent)
#define tag_get_child(__t, __child_name) g_hash_table_lookup((__t)->children_by_name, (__child_name))
#define tag_has_child(__t, __child_name) g_hash_table_lookup_extended((__t)->children_by_name, (__child_name), NULL, NULL)

#endif /* TAG_H */
