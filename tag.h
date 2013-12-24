#ifndef TAG_H
#define TAG_H
#include <glib.h>
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
} Tag;

/* Returns a copy of the default value for the tag, or if the default isn't set
   (i.e. equals NULL) returns a copy of the default for the tag type */
tagdb_value_t *tag_new_default (Tag *t);
void tag_destroy (Tag *t);
Tag *new_tag (char *name, int type, tagdb_value_t *default_value);
#define tag_name(_t) abstract_file_get_name((AbstractFile*) _t)
#define tag_id(_t) (((AbstractFile*)_t)->id)

#define tag_to_string(_t, _buf) abstract_file_to_string((AbstractFile*)_t, _buf)

#endif /* TAG_H */
