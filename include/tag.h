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

    /** The type of values for this tag
       See types.h for the types enum */
    int type;

    /** Default value for the tag */
    tagdb_value_t *default_value;

    /** Range of values allowed for the Integer typed value. Meaningless if the
       tag type is not Integer. */
    int min_value;
    int max_value;

    /** Aliases for the tag */
    GSList *aliases;
} Tag;

/* Returns a copy of the default value for the tag, or if the default isn't set
   (i.e. equals NULL) returns a copy of the default for the tag type */
tagdb_value_t *tag_new_default (Tag *t);
gboolean tag_destroy (Tag *t);
void tag_destroy0 (Tag *);

Tag *new_tag (const char *name, int type, tagdb_value_t *default_value);

char *tag_to_string (Tag *t, buffer_t buffer);
char *tag_to_string1 (Tag *t, char *buffer, size_t buffer_size);
void tag_set_name (Tag *t, const char *name);
void tag_set_default_explanation (Tag *t, const char *explanation);
void tag_remove_alias (Tag *t, const char *alias);
gboolean tag_has_alias(Tag *t, const char *name);
/** Add an alias for the tag.
 *
 * Returns the internal copy of the alias string
 */
const char *tag_add_alias (Tag *t, const char *alias);
#define tag_name(_t) abstract_file_get_name((AbstractFile*) _t)
#define tag_default_explanation(_t) ((_t)->default_value)
#define tag_id(_t) (((AbstractFile*)_t)->id)
#define tag_lock abstract_file_lock
#define tag_unlock abstract_file_unlock
#define tag_is_tag(__t) (abstract_file_get_type((__t))==abstract_file_tag_type)
#define tag_has_aliases(__t) ((__t)->aliases != NULL)
#define tag_aliases(t) (((Tag*)t)->aliases)

#endif /* TAG_H */
