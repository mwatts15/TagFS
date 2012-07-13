#ifndef FILE_H
#define FILE_H
#include <glib.h>
#include "abstract_file.h"
#include "tagdb_util.h"
#include "types.h"

typedef GHashTable TagTable;

extern GHashTable *files_g;

/* Representation of a file in the database. Contains the file name, unique id
   number and a table of tags and associated values */
typedef struct File
{
    file_id_t id;

    /* The file name
       Previously stored in in the TagTable under the "name" tag but moved for
       easier access. File names don't have to be unique to the file. */
    char *name;

    /* File's tags
       A table of tags with the value for the tag. */
    TagTable *tags;

    /* File drawer refcount
       This is updated at the same time as the tag counts in a
       file drawer.
       When the refcount == 0, we delete the file proper*/
    int refcount;
} File;

/* Creates the global file table files_g.
   Must be called before any file operations are used */
void file_initialize ();

/* Returns a new file object. The id will not be set */
File *new_file (char *name);

/* The file is only destroyed if its refcount is zero. Calling
   file_destroy otherwise does nothing */
void file_destroy (File *f);

/* Extracts a key vector for lookup in the FileTrie
   keybuf must be large enough to hold all of the tag IDs in the File's
   TagTable plus one for a terminating NULL */
tagdb_key_t file_extract_key (File *f);

/* convenience macro that makes the key buffer for you */

gboolean file_has_tags (File *f, tagdb_key_t tags);
gboolean file_is_untagged (File *f);
TagTable *tag_table_new();
void file_remove_tag (File *f, file_id_t tag_id);
void file_add_tag (File *f, file_id_t tag_id, tagdb_value_t *v);
gboolean file_equal (gconstpointer a, gconstpointer b);
guint file_hash (gconstpointer file);
#endif /* FILE_H */
