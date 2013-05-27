#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "types.h"
#include "file_drawer.h"
#include "file_cabinet.h"
#include "file.h"
#include "tag.h"
#include "abstract_file.h"

typedef GHashTable TagBucket;

typedef struct TagDB
{
    /* The tables which store File objects and Tag objects each.
       named below */
    TagBucket *tags;

    /* Stores the translations from tag id to tag name */
    GHashTable *tag_codes;

    /* A trie with keys derived from Tag IDs sorted numerically.
       The buckets contain file names (for easy comparison) which must
       be unique within a bucket. Each bucket contains all files with
       the tag IDs used as keys. */
    FileCabinet *files;

    /* Stores files for lookup by id */
    GHashTable *files_by_id;

    /* The name of the database file from which this TagDB was loaded. The
       default value for tagdb_save */
    gchar *db_fname;

    /* The highest file ID assigned. Used for assigning new ones.
       Updated each time a new file is added to the file table and generated
       anew each time the database is loaded from disk. */
    file_id_t file_max_id;

    /* With our structure, it's hard to just enmuerate. */
    gulong nfiles;

    /* Ditto for tags. */
    file_id_t tag_max_id;

    /* Flag for mt locking */
    int locked;
} TagDB;

TagDB *tagdb_new (const char *db_fname);
TagDB *tagdb_load (const char *db_fname);
void tagdb_save (TagDB *db, const char* db_fname);
void tagdb_destroy (TagDB *db);

/* Inserts a File object into the FileTrie
   The File object goes into the root by default if it has no tags.
   If the file has tags, it will be inserted into the correct position in
   the FileTrie.

   Sets the file id if it hasn't been set (i.e. equals 0) */
void insert_file (TagDB *db, File *f);
void set_file_name (File *f, char *new_name, TagDB *db);
void set_tag_name (Tag *t, char *new_name, TagDB *db);

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id);
/* Adds a tag to a file with the given value for the tag.
   - If the Tag does not exist, then the tag isn't added to the File.
   - If the Tag exists, and the File already has the tag, the value is changed,
   - If the tag exists, but the value is NULL, the value will be set to the
   default for that tag. */
void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *value);
void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id);

/* Retrieves a File from the TagDB which has the given tags and name */
File *lookup_file (TagDB *db, tagdb_key_t tag, char *name);

/* Retrieve file by id */
File *retrieve_file (TagDB *db, file_id_t id);

Tag *retrieve_tag (TagDB *db, file_id_t id);

/* Removes the File from the FileCabinet but does not destroy it */
void remove_file (TagDB *db, File *f);

/* Removes and destroys the File */
void delete_file (TagDB *db, File *f);

/* Removes the Tag from the database but doesn't destroy the Tag object */
void remove_tag (TagDB *db, Tag *t);

/* retrieve by tag name */
Tag *lookup_tag (TagDB *db, char *tag_name);

/* Inserts the tag into the tag bucket as well as creating a file slot
   in the file bucket */
void insert_tag (TagDB *db, Tag *t);

gulong tagdb_ntags (TagDB *db);

GList *tagdb_untagged_items (TagDB *db);
GList *tagdb_all_files (TagDB *db);

#endif /* TAGDB_H */
