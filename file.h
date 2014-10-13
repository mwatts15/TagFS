#ifndef FILE_H
#define FILE_H
#include <glib.h>
#include "key.h"
#include "abstract_file.h"
#include "types.h"

typedef GHashTable TagTable;

extern GHashTable *files_g;

/* Representation of a file in the database. Contains the file name, unique id
   number and a table of tags and associated values */
typedef struct File
{
    AbstractFile base;

    /* File's tags
       A table of tags with the value for the tag. */
    TagTable *tags;
} File;

#define file_tags(f) (f->tags)
/* Creates the global file table files_g.
   Must be called before any file operations are used */
void file_initialize ();

/* Returns a new file object. The id will not be set */
File *new_file (char *name);

/* The file is only destroyed if its refcount is zero. Calling
   file_destroy otherwise does nothing */
gboolean file_destroy (File *f);
/* file_destroy without freeing memory -- allows decoulping de-allocation
 * from destruction of contained data structures
 */
gboolean file_destroy0 (File *f);

void file_destroy_unsafe (File *f);

/* Extracts a key vector for lookup in the FileTrie. The return
 * value must be freed
 */
tagdb_key_t file_extract_key (File *f);

/* convenience macro that makes the key buffer for you */

gboolean file_has_tags (File *f, tagdb_key_t tags);
gboolean file_only_has_tags (File *f, tagdb_key_t tags);
gboolean file_is_untagged (File *f);
TagTable *tag_table_new();
void file_remove_tag (File *f, file_id_t tag_id);
void file_add_tag (File *f, file_id_t tag_id, tagdb_value_t *v);
gboolean file_equal (gconstpointer a, gconstpointer b);
guint file_hash (gconstpointer file);
void file_init (File *f, char *name);

#define file_to_string(_t, _buf) abstract_file_to_string((AbstractFile*)_t, _buf)
#define file_id(_f) (((AbstractFile*)_f)->id)
#define file_name(_f) abstract_file_get_name((AbstractFile*)_f)

#endif /* FILE_H */
