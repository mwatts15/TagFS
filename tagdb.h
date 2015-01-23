#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "sql.h"
#include "types.h"
#include "file_cabinet.h"
#include "file.h"
#include "tag.h"
#include "abstract_file.h"

/* a flag indicating that a database file should be cleared
 */
#define TAGDB_CLEAR 1

typedef GHashTable TagBucket;

typedef struct TagDB
{
    /* The tables which store File objects and Tag objects each.
       named below */
    TagBucket *tags;

    /* Stores the translations from tag id to tag name */
    GHashTable *tag_codes;

    /* A storage container for files */
    FileCabinet *files;

    /* Stores files indexed by their ID number */
    GHashTable *files_by_id;

    /* A shared sqlite database for the DB.
     */
    sqlite3 *sqldb;

    /* Prepared statements for the sqldb
     */
    sqlite3_stmt *sql_stmts[16];

    /* The file name of the database.
     */
    char *sqlite_db_fname;

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

/* tagdb_new and tagdb_new0 do database initialization as well.
 * Preserved for backwards compatability with testing code.
 */
TagDB *tagdb_new (const char *db_fname);
TagDB *tagdb_new0 (const char *db_fname, int flags);
TagDB *tagdb_new1 (sqlite3 *sqldb, int flags);

void tagdb_save (TagDB *db, const char* db_fname);
void tagdb_destroy (TagDB *db);

/* Inserts a File object into the FileTrie
   The File object goes into the root by default if it has no tags.
   If the file has tags, it will be inserted into the correct position in
   the FileTrie.

   Sets the file id if it hasn't been set (i.e. equals 0) */
void insert_file (TagDB *db, File *f);
void set_file_name (TagDB *db, File *f, const char *new_name);
void set_tag_name (TagDB *db, Tag *t, const char *new_name);

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id);
/* Adds a tag to a file with the given value for the tag.
   - If the Tag does not exist, then the tag isn't added to the File.
   - If the Tag exists, and the File already has the tag, the value is changed,
   - If the tag exists, but the value is NULL, the value will be set to the
   default for that tag.
   The File must already be inserted into the database
 */
void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *value);
void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id);

/* Retrieves a File from the TagDB which has the given tags and name */
File *tagdb_lookup_file (TagDB *db, tagdb_key_t keys, const char *name);

/* Retrieve file by id */
File *retrieve_file (TagDB *db, file_id_t id);

Tag *retrieve_tag (TagDB *db, file_id_t id);

/* Removes the File from the FileCabinet but does not destroy it */
void remove_file (TagDB *db, File *f);

/* Removes and destroys the File */
void delete_file (TagDB *db, File *f);

/* Removes the Tag object from the database and destroys it
 * returns FALSE on failure
 */
gboolean delete_tag (TagDB *db, Tag *t);

/* Indicates whether a remove_tag operation will succeed
 */
gboolean can_remove_tag(TagDB *db, Tag *t);

/* retrieve by fully specified tag name */
Tag *lookup_tag (TagDB *db, const char *tag_name);

/* Makes a tag with the given name
 *
 * The caller is obliged to regard the returned Tag as the tag object
 * corresponding to the tag_path passed in. Various renamings and
 * deletion may, however, occur and the Tag does not persist past the
 * life of the TagDB.
 */
Tag *tagdb_make_tag(TagDB *db, const char *tag_path);
File *tagdb_make_file(TagDB *db, const char *file_name);

/* Returns the files associated to a tag */
GList *tagdb_tag_files(TagDB *db, Tag *t);

void tagdb_tag_set_subtag(TagDB *db, Tag *sup, Tag *sub);

gulong tagdb_ntags (TagDB *db);
GList *tagdb_tag_ids (TagDB *db);
GList *tagdb_tags (TagDB *db);

GList *tagdb_untagged_items (TagDB *db);
GList *tagdb_all_files (TagDB *db);

void tagdb_begin_transaction (TagDB *db);
void tagdb_end_transaction (TagDB *db);

#endif /* TAGDB_H */
