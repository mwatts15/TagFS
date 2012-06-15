#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "code_table.h"
#include "types.h"
#include "trie.h"

typedef GHashTable FileCabinet;

typedef struct
{
    /* The actual files keyed by their names */
    GHashTable *table;

    /* The union of tags possessed by files in this drawer */
    GHashTable *tags;
} FileDrawer;

typedef GHashTable TagTable;
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

    /* The name of the database file from which this TagDB was loaded. The
       default value for tagdb_save */
    gchar *db_fname;

    /* The highest file ID assigned. Used for assigning new ones.
       Updated each time a new file is added to the file table and generated
       anew each time the database is loaded from disk. */
    gulong file_max_id;

    /* With our structure, it's hard to just enmuerate. */
    gulong nfiles;

    /* Ditto for tags. */
    gulong tag_max_id;
} TagDB;

typedef struct AbstractFile
{
    gulong id;
    char *name;
} AbstractFile;

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

/* Tags are assigned to files and have a specific type associated with them.
   They take the place of directories in the file system, but represent
   attributes possessed by the files. */
typedef struct Tag
{
    gulong id;

    /* Tag's name
       A string representation of the tag. Unlike file names, tag names must be
       unique within the database. */
    char *name;

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

TagDB *tagdb_load (const char *fname);
void tagdb_save (TagDB *db, const char* db_fname);
void tagdb_destroy (TagDB *db);

/* key for untagged files */
#define UNTAGGED 0

#define KL(key, i) \
int i = 0; \
do { \

#define KL_END(key, i) i++; } while (key[i] != 0)
/* Inserts a File object into the FileTrie
   The File object goes into the root by default if it has no tags.
   If the file has tags, it will be inserted into the correct position in
   the FileTrie. 

   Sets the file id if it hasn't been set (i.e. equals 0) */
void insert_file (TagDB *db, File *f);
void set_name (AbstractFile *f, char *new_name);
void set_file_name (File *f, char *new_name, TagDB *db);
void set_tag_name (Tag *t, char *new_name, TagDB *db);

/* Extracts a key vector for lookup in the FileTrie 
   keybuf must be large enough to hold all of the tag IDs in the File's
   TagTable plus one for a terminating NULL */
void file_extract_key0 (File *f, gulong *keybuf);

/* convenience macro that makes the key buffer for you */
#define file_extract_key(file, key_buf) \
    gulong key_buf[g_hash_table_size(f->tags) + 2]; \
file_extract_key0 (file, key_buf)

/* Adds a tag to a file with the given value for the tag.
   - If the Tag does not exist, then the tag isn't added to the File.
   - If the Tag exists, and the File already has the tag, the value is changed,
   - If the tag exists, but the value is NULL, the value will be set to the 
   default for that tag. */
void add_tag_to_file (TagDB *db, File *f, gulong tag_id, tagdb_value_t *value);

/* Removes a file from a single slot */
void file_cabinet_remove (TagDB *db, gulong slot_id, File *f);

/* Removes from all of the slots. All of them */
void file_cabinet_remove_v (TagDB *db, gulong *slot_ids, File *f);

/* Inserts a file into a single slot */
void file_cabinet_insert (TagDB *db, gulong slot_id, File *f);

/* Inserts into all of the slots. All of them */
void file_cabinet_insert_v (TagDB *db, gulong *slot_ids, File *f);

void file_cabinet_remove_all (TagDB *db, File *f);

void add_new_file_drawer (TagDB *db, gulong slot_id);

/* Returns the slot as a GList of its contents */
GList *file_drawer_as_list (FileDrawer *s);

/* Returns the keyed file slot */
FileDrawer *retrieve_file_drawer (TagDB *db, gulong slot_id);
/* Returns the keyed file slot as a GList */
GList *retrieve_file_drawer_l (TagDB *db, gulong slot_id);

/* Retrieves a File from the TagDB which has the given tags and name */
File *retrieve_file (TagDB *db, gulong *tag, char *name);
Tag *retrieve_tag (TagDB *db, gulong id);

/* The file is only destroyed if its refcount is zero. Calling
   file_destroy otherwise does nothing */
void file_destroy (File *f);
void tag_destroy (Tag *t);

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

/* Returns a copy of the default value for the tag, or if the default isn't set
   (i.e. equals NULL) returns a copy of the default for the tag type */
tagdb_value_t *tag_new_default (Tag *t);

/* Returns a new file object. The id will not be set */
File *new_file (char *name);
Tag *new_tag (char *name, int type, gpointer default_value);
GList *get_files_list (TagDB *db, gulong *tags);

GList *get_tags_list (TagDB *db, gulong *key, GList *files_list);

gulong tagdb_ntags(TagDB *db);
int file_id_cmp (File *f1, File *f2);
char *file_to_string (gpointer f);
#define tag_to_string(t) file_to_string(t)
void print_key (gulong *k);

#endif /*TAGDB_H*/
