#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "tagdb.h"
#include "file_cabinet.h"
#include "abstract_file.h"
#include "file.h"
#include "tag.h"
#include "tagdb_util.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"
#include "util.h"

enum { NEWTAG ,
    NEWFIL ,
    RENFIL ,
    RENTAG ,
    DELFIL ,
    DELTAG ,
    STAGID ,
    STAGNM ,
    SFILID ,
    SFILNM ,
    TALIAS ,
    TUNALI ,
    NUMBER_OF_STMTS };
#define STMT(_db,_i) ((_db)->sql_stmts[(_i)])
#define STMT_SEM(_db,_i) (&((_db)->stmt_semas[(_i)]))
void _sqlite_newtag_stmt(TagDB *db, Tag *t);
void _sqlite_newfile_stmt(TagDB *db, File *t);
void _sqlite_rename_file_stmt(TagDB *db, File *f, const char *new_name);
void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, const char *new_name);
void _sqlite_delete_file_stmt(TagDB *db, File *t);
void _sqlite_delete_tag_stmt(TagDB *db, Tag *t);
void _sqlite_tag_alias_ins_stmt(TagDB *db, Tag *t, const char *alias);
void _sqlite_tag_alias_rem_stmt(TagDB *db, Tag *t, const char *alias);

file_id_t tag_name_to_id (TagDB *db, const char *tag_name);
/* retrieves a root tag by its name using the tag_codes table */
Tag *retrieve_root_tag_by_name (TagDB *db, const char *tag_name);
/* Deletes the tag_codes entry for named tag */
void clear_root_tag_by_name (TagDB *db, const char *tag_name);
/* Deletes the tag_codes entry for the tag */
void clear_root_tag (TagDB *db, Tag *tag);

/* Inserts the tag into the tag bucket as well as creating a file slot in the
 * file bucket. If the Tag is already in the db, then the given tag will not be
 * added. The ID of the tag will be set on the Tag passed in if it was indeed
 * added to the db */
void insert_tag (TagDB *db, Tag *t);

/* Ensures that the database remains consistent for a tag deletion */
int delete_tag0 (TagDB *db, Tag *t);

GList *tagdb_untagged_items (TagDB *db)
{
    return file_cabinet_get_untagged_files(db->files);
}

GList *tagdb_all_files (TagDB *db)
{
    return g_hash_table_get_values(db->files_by_id);
}

GList *tagdb_all_tags (TagDB *db)
{
    return g_hash_table_get_values(db->tags);
}

void set_file_name (TagDB *db, File *f, const char *new_name)
{
    abstract_file_set_name(f, new_name);
    _sqlite_rename_file_stmt(db, f, new_name);
}

void set_tag_name (TagDB *db, Tag *t, const char *new_name)
{
    Tag *maybe_existing_tag = lookup_tag(db, new_name);
    /* We don't overwrite existing tags */
    if (maybe_existing_tag)
    {
        return;
    }
    clear_root_tag(db, t);
    tag_set_name(t, new_name);
    g_hash_table_insert(db->tag_codes, (gpointer) tag_name(t), TO_SP(tag_id(t)));
    _sqlite_rename_tag_stmt(db, t, new_name);
}

void remove_file (TagDB *db, File *f)
{
    file_cabinet_remove_all(db->files, f);
}

TagBucket *tag_bucket_new ()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
}

void tag_bucket_remove (TagDB *db, Tag *t)
{
    g_hash_table_remove(db->tags, TO_SP(tag_id(t)));
}

void tag_bucket_insert (TagDB *db, Tag *t)
{
    g_hash_table_insert(db->tags, TO_SP(tag_id(t)), t);
}

gulong tag_bucket_size (TagDB *db)
{
    return (gulong) g_hash_table_size(db->tags);
}

gulong tagdb_ntags (TagDB *db)
{
    return tag_bucket_size(db);
}

gulong tagdb_nfiles (TagDB *db)
{
    return db->nfiles;
}

GList *tagdb_tag_ids (TagDB *db)
{
    return g_hash_table_get_keys(db->tags);
}

GList *tagdb_tags (TagDB *db)
{
    return g_hash_table_get_values(db->tags);
}

Tag *tagdb_make_tag (TagDB *db, const char *tname)
{
    Tag *res = NULL;

    Tag *root = retrieve_root_tag_by_name(db, tname);
    if (root)
    {
        res = root;
    }
    else
    {
        res = new_tag(tname, 0, 0);
        insert_tag(db, res);
    }

    return res;
}

File *tagdb_make_file (TagDB *db, const char *file_name)
{
    File *f = new_file(file_name);
    insert_file(db, f);
    return f;
}

gboolean tagdb_alias_tag (TagDB *db, Tag *t, const char *alias)
{
    Tag *preexisting_tag = retrieve_root_tag_by_name(db, alias);

    if (preexisting_tag)
    {
        if (tag_id(t) != tag_id(preexisting_tag))
        {
            warn("A tag name (%s) for the tag %s(%d) already exists and differs"
                    " from the tag to be aliased %s(%d)", alias,
                    tag_name(preexisting_tag), tag_id(preexisting_tag),
                    tag_name(t), tag_id(t));
        }
        return FALSE;
    }
    const char *alias_copy = tag_add_alias(t, alias);
    g_hash_table_insert(db->tag_codes, (gpointer) alias_copy, TO_SP(tag_id(t)));
    _sqlite_tag_alias_ins_stmt(db, t, alias_copy);
    return TRUE;
}

void tagdb_begin_transaction (TagDB *db)
{
    sql_begin_transaction(db->sqldb);
}

void tagdb_end_transaction (TagDB *db)
{
    sql_commit(db->sqldb);
}

void insert_tag (TagDB *db, Tag *t)
{
    Tag *preexisting_tag = lookup_tag(db, tag_name(t));
    if (!preexisting_tag)
    {
        if (!tag_id(t))
            tag_id(t) = ++db->tag_max_id;

        g_hash_table_insert(db->tag_codes, (gpointer) tag_name(t), TO_SP(tag_id(t)));
        tag_bucket_insert(db, t);
        _sqlite_newtag_stmt(db, t);
    }
}

void tagdb_tag_remove_alias (TagDB *db, Tag *t, const char *name)
{
    g_hash_table_remove(db->tag_codes, name);
    tag_remove_alias(t, name);
    _sqlite_tag_alias_rem_stmt(db, t, name);
}

void delete_file_flip (File *f, TagDB *db)
{
    delete_file(db, f);
}

void delete_file (TagDB *db, File *f)
{
    db->nfiles--;
    /* file_cabinet_remove_all removes the references
     * SQL tables referencs for the file cabinet.
     */
    file_cabinet_remove_all(db->files, f);
    _sqlite_delete_file_stmt(db, f);
    /* file_cabinet_delete_file deletes the file
     * data, so it has to be last
     */
    file_cabinet_delete_file(db->files, f);
}

void insert_file (TagDB *db, File *f)
{
    tagdb_key_t key = file_extract_key(f);
    /* If the file's id is unset (i.e. 0) then
     * we mint a new one and set it
     */
    if (!file_id(f))
    {
        db->nfiles++;
        file_id(f) = ++db->file_max_id;
        _sqlite_newfile_stmt(db, f);
        g_hash_table_insert(db->files_by_id, TO_SP(file_id(f)), f);
    }

    file_cabinet_insert_v(db->files, key, f);
    key_destroy(key);
}

File *retrieve_file (TagDB *db, file_id_t id)
{
    return g_hash_table_lookup(db->files_by_id, TO_SP(id));
}

GList *tagdb_tag_files(TagDB *db, Tag *t)
{
    return file_cabinet_get_drawer_l(db->files, tag_id(t));
}

tagdb_key_t tagdb_tag_tags(TagDB *db, Tag *t)
{
    return file_cabinet_get_drawer_tags_a(db->files, tag_id(t));
}

File *tagdb_lookup_file (TagDB *db, tagdb_key_t keys, const char *name)
{
    if (!keys)
    {
        return NULL;
    }
    return file_cabinet_lookup_file(db->files, keys, name);
}

Tag *retrieve_tag (TagDB *db, file_id_t id)
{
    return (Tag*) g_hash_table_lookup(db->tags, TO_SP(id));
}

file_id_t tag_name_to_id (TagDB *db, const char *tag_name)
{
    file_id_t id = TO_S(g_hash_table_lookup(db->tag_codes, tag_name));
    return id;
}

Tag *retrieve_root_tag_by_name (TagDB *db, const char *tag_name)
{
    file_id_t id = TO_S(g_hash_table_lookup(db->tag_codes, tag_name));
    if (id)
    {
        return retrieve_tag(db, id);
    }
    else
    {
        return NULL;
    }
}

void clear_root_tag (TagDB *db, Tag *t)
{
    g_hash_table_remove(db->tag_codes, (gpointer) tag_name(t));
}

void clear_root_tag_by_name (TagDB *db, const char *tag_name)
{
    g_hash_table_remove(db->tag_codes, (gpointer) tag_name);
}

int delete_tag0 (TagDB *db, Tag *t)
{
    if (!can_remove_tag(db, t))
    {
        return FALSE;
    }
    tag_bucket_remove(db, t);
    clear_root_tag(db, t);
    file_cabinet_remove_drawer(db->files, tag_id(t));
    _sqlite_delete_tag_stmt(db, t);
    return TRUE;
}

gboolean delete_tag (TagDB *db, Tag *t)
{
    if (delete_tag0(db, t))
    {
        return tag_destroy(t);
    }

    return FALSE;
}

gboolean can_remove_tag (TagDB *db, Tag *t)
{
    /* TODO: Considering having this return FALSE if there are files tagged
     * with the given tag
     */
    return TRUE;
}

Tag *lookup_tag (TagDB *db, const char *tag_name)
{
    return retrieve_root_tag_by_name(db, tag_name);
}

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id)
{
    file_remove_tag(f, tag_id);
    file_cabinet_remove(db->files, tag_id, f);
}

void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *v)
{
    /* Look up the tag */
    Tag *t = retrieve_tag(db, tag_id);
    if (t == NULL || !retrieve_file(db, file_id(f)))
    {
        return;
    }

    /* If it is found, insert the value */
    if (v == NULL)
    {
        if (file_tag_value(f, tag_id))
        {
            return;
        }
        v = tag_new_default(t);
    }
    else
    {
        v = copy_value(v);
    }
    file_add_tag(f, tag_id, v);
    file_cabinet_insert (db->files, tag_id, f);
}

void tagdb_save (G_GNUC_UNUSED TagDB *db, G_GNUC_UNUSED const char *db_fname)
{}

void tagdb_destroy (TagDB *db)
{
    if (db->tags)
    {
        HL(db->tags, it, k, v)
        {
            tag_destroy((Tag*) v);
        } HL_END;
    }

    g_free(db->sqlite_db_fname);
    if (db->files)
    {
        file_cabinet_destroy(db->files);
        debug("deleted file cabinet");
    }

    /* Delete the files */
    if (db->files_by_id)
    {
        HL (db->files_by_id, it, k, v)
        {
            file_destroy_unsafe((File*) v);
        } HL_END;
    }

    g_hash_table_destroy(db->files_by_id);

    for (int i = 0; i < NUMBER_OF_STMTS; i++)
    {
        if (STMT(db, i))
        {
            sqlite3_finalize(STMT(db, i));
            sem_destroy(&(db->stmt_semas[i]));
        }
    }

    sqlite3_close(db->sqldb);
    /* Files have to be deleted after the file cabinet
     * a memory leak/invalid read here is a problem with
     * the file_cabinet algorithms
     */
    g_hash_table_destroy(db->tags);
    g_hash_table_destroy(db->tag_codes);
    g_free(db);
}

void _sqlite_newtag_stmt(TagDB *db, Tag *t)
{
    /* This function is idempotent with respect to the data */

    sqlite3_stmt *stmt = STMT(db, NEWTAG);
    sem_wait(STMT_SEM(db, NEWTAG));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    /* XXX: Tag name is transient because tags may be destroyed
     * within the run of the program and we don't want to have
     * to manage sqlite's memory
     */
    sqlite3_bind_text(stmt, 2, tag_name(t), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, NEWTAG));
}

void _sqlite_newfile_stmt(TagDB *db, File *t)
{
    sqlite3_stmt *stmt = STMT(db, NEWFIL);
    sem_wait(STMT_SEM(db, NEWFIL));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(t));
    /* XXX: File name is transient because tags may be destroyed
     * within the run of the program and we don't want to have
     * to manage sqlite's memory
     */
    sqlite3_bind_text(stmt, 2, file_name(t), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, NEWFIL));
}

void _sqlite_delete_file_stmt(TagDB *db, File *f)
{
    sqlite3_stmt *stmt = STMT(db, DELFIL);
    sem_wait(STMT_SEM(db, DELFIL));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(f));
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, DELFIL));
}

void _sqlite_delete_tag_stmt(TagDB *db, Tag *t)
{
    sqlite3_stmt *stmt = STMT(db, DELTAG);
    sem_wait(STMT_SEM(db, DELTAG));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, DELTAG));
}

void _sqlite_rename_file_stmt(TagDB *db, File *f, const char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENFIL);
    sem_wait(STMT_SEM(db, RENFIL));
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, file_id(f));
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, RENFIL));
}

void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, const char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENTAG);
    sem_wait(STMT_SEM(db, RENTAG));
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, tag_id(t));
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, RENTAG));
}

void _sqlite_tag_alias_ins_stmt(TagDB *db, Tag *t, const char *alias)
{
    sqlite3_stmt *stmt = STMT(db, TALIAS);
    sem_wait(STMT_SEM(db, TALIAS));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    sqlite3_bind_text(stmt, 2, alias, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, TALIAS));
}

void _sqlite_tag_alias_rem_stmt(TagDB *db, Tag *t, const char *alias)
{
    sqlite3_stmt *stmt = STMT(db, TUNALI);
    sem_wait(STMT_SEM(db, TUNALI));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    sqlite3_bind_text(stmt, 2, alias, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sem_post(STMT_SEM(db, TUNALI));
}

TagDB *tagdb_new (const char *db_fname)
{
    return tagdb_new0(db_fname, 0);
}

void _tagdb_init_tags(TagDB *db);
void _tagdb_init_files(TagDB *db);

TagDB *tagdb_new0 (const char *db_fname, int flags)
{
    sqlite3 *db = sql_init(db_fname);
    if (!db)
    {
        error("Couldn't initialize the sqlite database. Aborting");
        return NULL;
    }
    else
    {
        TagDB *res = tagdb_new1(db, flags);
        if (res)
        {
            return res;
        }
        else
        {
            error("TagDB initialization failed");
            sqlite3_close(db);
            return NULL;
        }
    }
}

TagDB *tagdb_new1 (sqlite3 *sqldb, G_GNUC_UNUSED int flags)
{
    TagDB *db = calloc(1, sizeof(struct TagDB));
    db->sqldb = sqldb;
    db->sqlite_db_fname = g_strdup(sqlite3_db_filename(sqldb, "main"));

    for (int i = 0; i < NUMBER_OF_STMTS; i++)
    {
        sem_init(&(db->stmt_semas[i]), 0, 1);
    }

    /* new tag statement */
    sql_prepare(db->sqldb, "insert into tag(id,name) values(?,?)", STMT(db, NEWTAG));
    /* new file statement */
    sql_prepare(db->sqldb, "insert into file(id,name) values(?,?)", STMT(db, NEWFIL));
    /* alias statement */
    sql_prepare(db->sqldb, "insert into tag_alias(id, name) values(?,?)", STMT(db, TALIAS));
    /* remove alias statement */
    sql_prepare(db->sqldb, "delete from tag_alias where id=? and name=?", STMT(db, TUNALI));
    /* delete tag statement */
    sql_prepare(db->sqldb, "delete from tag where id = ?", STMT(db, DELTAG));
    /* delete file statement */
    sql_prepare(db->sqldb, "delete from file where id = ?", STMT(db, DELFIL));
    /* rename file statement */
    sql_prepare(db->sqldb, "update or ignore file set name = ? where id = ?", STMT(db, RENFIL));
    /* rename tag statement */
    sql_prepare(db->sqldb, "update or ignore tag set name = ? where id = ?", STMT(db, RENTAG));

    /* lookup tag by id statement */
    sql_prepare(db->sqldb, "select name from tag where id = ?", STMT(db, STAGID));

    /* lookup tag by name statement */
    sql_prepare(db->sqldb, "select id from tag where name = ?", STMT(db, STAGNM));

    /* lookup file name by id statement */
    sql_prepare(db->sqldb, "select name from file where id = ?", STMT(db, SFILID));

    /* lookup file by name statement */
    sql_prepare(db->sqldb, "select id from file where name = ?", STMT(db, SFILNM));

    db->files_by_id = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);


    db->tags = tag_bucket_new();
    db->tag_codes = g_hash_table_new(g_str_hash, g_str_equal);
    db->tag_max_id = 0;

    _tagdb_init_tags(db);

    db->file_max_id = 0;
    db->nfiles = 0;
    _tagdb_init_files(db);
    db->files = file_cabinet_new0(db->sqldb, db->files_by_id);

    db->nfiles = g_hash_table_size(db->files_by_id);
    return db;
}

void _tagdb_init_files(TagDB *db)
{
    /* Reads in the files from the sql database */
    sqlite3_stmt *stmt;
    sql_prepare(db->sqldb, "select distinct * from file", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        File *f = new_file((const char*)name);
        file_id(f) = id;
        if (db->file_max_id < id)
            db->file_max_id = id;
        g_hash_table_insert(db->files_by_id, TO_SP(file_id(f)), f);
    }
    sqlite3_finalize(stmt);
    sql_prepare(db->sqldb, "select distinct * from file_tag order by file", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t file_id = sqlite3_column_int64(stmt, 0);
        file_id_t tag_id = sqlite3_column_int64(stmt, 1);
        File *f = g_hash_table_lookup(db->files_by_id, TO_SP(file_id));
        file_add_tag(f, tag_id, g_strdup(""));
    }
    sqlite3_finalize(stmt);
}

void _tagdb_init_tags(TagDB *db)
{
    /* Reads in the files from the sql database */
    sqlite3_stmt *stmt;

    sql_prepare(db->sqldb, "select distinct tag.id, tag.name from tag", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        if (id > db->tag_max_id)
            db->tag_max_id = id;
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        Tag *t = new_tag((const char*)name, TAGDB_INT_TYPE, 0);
        tag_id(t) = id;
        tag_bucket_insert(db, t);
        g_hash_table_insert(db->tag_codes, (gpointer)tag_name(t), TO_SP(id));
    }
    sqlite3_finalize(stmt);

    sql_prepare(db->sqldb, "select distinct id, name from tag_alias", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        char *name_copy = g_strdup((const char*)name);
        Tag *t = retrieve_tag(db, id);
        t->aliases = g_slist_append(t->aliases, name_copy);
        g_hash_table_insert(db->tag_codes, (gpointer)name_copy, TO_SP(id));
    }
    sqlite3_finalize(stmt);
}
