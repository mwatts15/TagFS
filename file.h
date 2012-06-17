#ifndef FILE_H
#define FILE_H
#include <glib.h>
#include "types.h"

typedef GHashTable TagTable;

/* Representation of a file in the database. Contains the file name, unique id
   number and a table of tags and associated values */
typedef struct File
{
    gulong id;

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

/* Returns a new file object. The id will not be set */
File *new_file (char *name);

/* The file is only destroyed if its refcount is zero. Calling
   file_destroy otherwise does nothing */
void file_destroy (File *f);

/* Extracts a key vector for lookup in the FileTrie 
   keybuf must be large enough to hold all of the tag IDs in the File's
   TagTable plus one for a terminating NULL */
void file_extract_key0 (File *f, gulong *buf);

/* convenience macro that makes the key buffer for you */
#define file_extract_key(file, key_buf) \
    gulong key_buf[g_hash_table_size(f->tags) + 2]; \
file_extract_key0 (file, key_buf)

gboolean file_has_tags (File *f, gulong *tags);
void file_remove_tag (File *f, gulong tag_id);
void file_add_tag (File *f, gulong tag_id, tagdb_value_t *v);
#endif /* FILE_H */
