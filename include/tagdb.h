#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "sql.h"
#include "types.h"
#include "file_cabinet.h"
#include "file.h"
#include "tag.h"
#include "abstract_file.h"

/** In general, Tag and File objects passed to TagDB procedures are not checked
 * for validity. If such objects are retrieved from TagDB passing, then they
 * will be valid, but objects constructed otherwise will result in undefined
 * behavior.
 */

/** a flag indicating that a database file should be cleared */
#define TAGDB_CLEAR 1

typedef GHashTable TagBucket;

typedef struct TagDB
{
    /** The tables which store File objects and Tag objects each.
     *  named below
     */
    TagBucket *tags;

    /** Stores the translations from tag id to tag name */
    GHashTable *tag_codes;

    /** A storage container for files */
    FileCabinet *files;

    /** Stores files indexed by their ID number */
    GHashTable *files_by_id;

    /** A shared sqlite database for the DB. */
    sqlite3 *sqldb;

    /** Prepared statements for the sqldb */
    sqlite3_stmt *sql_stmts[16];

    /** Semaphores for sqlite3 prepared statements */
    sem_t stmt_semas[16];

    /** The file name of the database. */
    char *sqlite_db_fname;

    /** The name of the database file from which this TagDB was loaded. The
     *  default value for tagdb_save
     */
    gchar *db_fname;

    /** The highest file ID assigned. Used for assigning new ones.
     *  Updated each time a new file is added to the file table and generated
     *  anew each time the database is loaded from disk.
     */
    file_id_t file_max_id;

    /** With our structure, it's hard to just enmuerate. */
    gulong nfiles;

    /** The highest tag ID assigned. */
    file_id_t tag_max_id;

    /** Flag for mt locking */
    int locked;
} TagDB;

/** Make a new TagDB with its SQL database stored in db_fname */
TagDB *tagdb_new (const char *db_fname);
/** Like tagdb_new, but flags can be passed in.
 *  @param flags doesn't do anything
 */
TagDB *tagdb_new0 (const char *db_fname, int flags);

/** Create a new TagDB and the SQL tables */
TagDB *tagdb_new1 (sqlite3 *sqldb, int flags);

/** Write to disk any necessary data in the DB */
void tagdb_save (TagDB *db, const char* db_fname);

/** Free resources owned by the DB */
void tagdb_destroy (TagDB *db);

/** Inserts a File object into the FileTrie
 *  The File object goes into the root by default if it has no tags.
 *  If the file has tags, it will be inserted into the correct position in
 *  the FileTrie.
 *
 *  Sets the file id if it hasn't been set (i.e. equals 0)
 */
void insert_file (TagDB *db, File *f);
#define tagdb_insert_file(__db, __f) insert_file((__db), (__f))

void set_file_name (TagDB *db, File *f, const char *new_name);
#define tagdb_set_file_name(__db, __f, __name) set_file_name((__db), (__f), (__name))

void set_tag_name (TagDB *db, Tag *t, const char *new_name);
#define tagdb_set_tag_name(__db, __f, __name) set_tag_name((__db), (__f), (__name))

/** Remove an alias for a tag */
void tagdb_tag_remove_alias (TagDB *db, Tag *t, const char *name);

/** Adds a tag to a file with the given value for the tag.
 *  - If the Tag does not exist, then the tag isn't added to the File.
 *  - If the Tag exists, and the File already has the tag, the value is changed,
 *  - If the tag exists, but the value is NULL, the value will be set to the
 *  default for that tag.
 *  The File must already be inserted into the database
 */
void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *value);
#define tagdb_add_tag_to_file(__db, __f, __id, __val) add_tag_to_file((__db), (__f), (__id), (__val))

/** Remove a tag from the file if it has that tag */
void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id);
#define tagdb_remove_tag_from_file(__db, __f, __id) remove_tag_from_file((__db), (__f), (__id))

/** Retrieves a File from the TagDB which has the given tags and name */
File *tagdb_lookup_file (TagDB *db, tagdb_key_t keys, const char *name);

/** Retrieve file by id */
File *retrieve_file (TagDB *db, file_id_t id);
#define tagdb_retrieve_file(__db, __id) retrieve_file((__db), (__id))

/** Retrieve a file from the DB by its identifier */
Tag *retrieve_tag (TagDB *db, file_id_t id);
#define tagdb_retrieve_tag(__db, __id) retrieve_tag((__db), (__id))

/** Removes and destroys the given file */
void delete_file (TagDB *db, File *f);
#define tagdb_delete_file(__db, __f) delete_file((__db), (__f))

/** Removes the Tag object from the database and destroys it
 * returns FALSE on failure
 */
gboolean delete_tag (TagDB *db, Tag *t);
#define tagdb_delete_tag(__db, __t) delete_tag((__db), (__t))

/** Indicates whether a remove_tag operation will succeed */
gboolean can_remove_tag(TagDB *db, Tag *t);
#define tagdb_can_remove_tag(__db, __t) can_remove_tag((__db), (__t))

/** retrieve by fully specified tag name */
Tag *lookup_tag (TagDB *db, const char *tag_name);
#define tagdb_lookup_tag(__db, __name) lookup_tag((__db), (__name))

/** Alias the given tag to the name.
 *
 * Returns TRUE if the tag alias succeeded
 */
gboolean tagdb_alias_tag(TagDB *db, Tag *t, const char *alias);

/** Makes a tag with the given name
 *
 * The caller is obliged to regard the returned Tag as the tag object
 * corresponding to the tag_path passed in. Various renamings and
 * deletion may, however, occur and the Tag does not persist past the
 * life of the TagDB.
 */
Tag *tagdb_make_tag(TagDB *db, const char *tag_path);

/** Make a file object with the given name and insert it into the database.
 *
 * @return The file if it was successfully added, otherwise NULL
 */
File *tagdb_make_file(TagDB *db, const char *file_name);

/** Return the files associated to a tag */
GList *tagdb_tag_files(TagDB *db, Tag *t);

/** Return the tags associated to a tag */
GList *tagdb_tag_tags(TagDB *db, Tag *t);

/** Set a parent/child relationship between two tags
 *
 *  @param sup The parent tag
 *  @param sub The chlild tag
 */
void tagdb_tag_set_subtag(TagDB *db, Tag *sup, Tag *sub);

/** Get the number of tags in the DB */
gulong tagdb_ntags (TagDB *db);

/** Get a list of tag IDs in the DB */
GList *tagdb_tag_ids (TagDB *db);

/** Get a list of tags in the DB */
GList *tagdb_tags (TagDB *db);

/** Get a list of files that have no tags attached */
GList *tagdb_untagged_items (TagDB *db);

/** Get the number of files in the DB. */
gulong tagdb_nfiles (TagDB *db);

/** Get a list of all the files in the DB.
 *
 * Please, call g_list_free on the result of this procedure.
 */
GList *tagdb_all_files (TagDB *db);

void tagdb_begin_transaction (TagDB *db);
void tagdb_end_transaction (TagDB *db);

#endif /* TAGDB_H */
