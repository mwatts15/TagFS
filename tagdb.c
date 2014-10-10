#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tagdb.h"
#include "tagdb_priv.h"
#include "file_drawer.h"
#include "file_cabinet.h"
#include "abstract_file.h"
#include "file.h"
#include "tag.h"
#include "scanner.h"
#include "tagdb_util.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"

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
    NUMBER_OF_STMTS };
#define STMT(_db,_i) ((_db)->sql_stmts[(_i)])
void _sqlite_newtag_stmt(TagDB *db, Tag *t);
void _sqlite_newfile_stmt(TagDB *db, File *t);
void _sqlite_rename_file_stmt(TagDB *db, File *f, char *new_name);
void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, char *new_name);
void _sqlite_delete_file_stmt(TagDB *db, File *t);
void _sqlite_delete_tag_stmt(TagDB *db, Tag *t);

GList *tagdb_untagged_items (TagDB *db)
{
    return file_cabinet_get_drawer_l(db->files, UNTAGGED);
}

GList *tagdb_all_files (TagDB *db)
{
    return g_hash_table_get_values(db->files_by_id);
}

GList *tagdb_all_tags (TagDB *db)
{
    return g_hash_table_get_values(db->tags);
}

void set_file_name (File *f, char *new_name, TagDB *db)
{
    remove_file(db, f);
    set_name(f, new_name);
    insert_file(db, f);
    _sqlite_rename_file_stmt(db, f, new_name);
}

void set_tag_name (Tag *t, char *new_name, TagDB *db)
{
    g_hash_table_remove(db->tag_codes, tag_name(t));
    set_name(t, new_name);
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

GList *tagdb_tag_names (TagDB *db)
{
    return g_hash_table_get_keys(db->tags);
}

void insert_tag (TagDB *db, Tag *t)
{
    if (!tag_id(t))
        tag_id(t) = ++db->tag_max_id;
    g_hash_table_insert(db->tag_codes, (gpointer) tag_name(t), TO_SP(tag_id(t)));
    tag_bucket_insert(db, t);
    _sqlite_newtag_stmt(db, t);
    file_cabinet_new_drawer(db->files, tag_id(t));
}

void delete_file_flip (File *f, TagDB *db)
{
    delete_file(db, f);
}

void delete_file (TagDB *db, File *f)
{
    db->nfiles--;
    g_hash_table_remove(db->files_by_id, TO_SP(file_id(f)));
    file_cabinet_remove_all(db->files, f);
    if (!file_destroy(f))
    {
        error("Could not destroy file: %s", file_name(f));
    }
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
    }

    g_hash_table_insert(db->files_by_id, TO_SP(file_id(f)), f);
    if (file_is_untagged(f))
    {
        file_cabinet_insert(db->files, UNTAGGED, f);
    }
    else
    {
        file_cabinet_insert_v(db->files, key, f);
    }
    key_destroy(key);
}

void put_file_in_untagged(TagDB *db, File *f)
{
    file_cabinet_insert(db->files, UNTAGGED, f);
}

gboolean file_is_homeless(TagDB *db, File *f)
{
    gboolean res = FALSE;
    HL(file_tags(f),it,k,v)
    {
        /* check if the file exists */
        Tag *t = retrieve_tag(db, TO_S(k));
        if (t!= NULL)
        {
            res = TRUE;
            break;
        }
    } HL_END;

    return res;
}

File *retrieve_file (TagDB *db, file_id_t id)
{
    return g_hash_table_lookup(db->files_by_id, TO_SP(id));
}

GList *tag_files(TagDB *db, Tag *t)
{
    return file_cabinet_get_drawer_l(db->files, tag_id(t));

}

File *lookup_file (TagDB *db, tagdb_key_t keys, char *name)
{
    return file_cabinet_lookup_file(db->files, keys, name);
}

Tag *retrieve_tag (TagDB *db, file_id_t id)
{
    return (Tag*) g_hash_table_lookup(db->tags, TO_SP(id));
}

file_id_t tag_name_to_id (TagDB *db, char *tag_name)
{
    file_id_t id = TO_S(g_hash_table_lookup(db->tag_codes, tag_name));
    return id;
}

void remove_tag (TagDB *db, Tag *t)
{
    tag_bucket_remove(db, t);
    g_hash_table_remove(db->tag_codes, tag_name(t));
    file_cabinet_remove_drawer(db->files, tag_id(t));
    _sqlite_delete_tag_stmt(db, t);
}

void delete_tag (TagDB *db, Tag *t)
{
    remove_tag(db, t);
    tag_destroy(t);
}

Tag *lookup_tag (TagDB *db, char *tag_name)
{
    return retrieve_tag(db, tag_name_to_id(db, tag_name));
}

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id)
{
    file_remove_tag(f, tag_id);
}

void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *v)
{
    /* Look up the tag */
    Tag *t = retrieve_tag(db, tag_id);
    if (t == NULL)
    {
        /* return if the tag isn't found */
        result_destroy(v);
        return;
    }

    /* If it is found, insert the value */
    if (v == NULL)
        v = tag_new_default(t);
    else
        v = copy_value(v);
    file_add_tag(f, tag_id, v);
}

void tagdb_save (TagDB *db, const char *db_fname)
{
    if (db_fname == NULL)
    {
        db_fname = db->db_fname;
    }
    FILE *f = fopen(db_fname, "w");
    if (f == NULL)
    {
        error("Couldn't open db file for save");
    }

    tags_to_file(db, f);
    files_to_file(db, f);
    fclose(f);
}

void tagdb_destroy (TagDB *db)
{
    if (db->tags)
    {
        HL(db->tags, it, k, v)
        {
            tag_destroy((Tag*) v);
        } HL_END;
    }

    g_free(db->db_fname);
    g_free(db->sqlite_db_fname);
    if (db->files)
    {
        file_cabinet_destroy(db->files);
        debug("deleted file cabinet");
    }

    for (int i = 0; i < NUMBER_OF_STMTS; i++)
    {
        if (STMT(db, i))
        {
            sqlite3_finalize(STMT(db, i));
        }
    }

    sqlite3_close(db->sqldb);
    /* Files have to be deleted after the file cabinet
     * a memory leak/invalid read here is a problem with
     * the file_cabinet algorithms
     */
    if(db->files_by_id)
    {
        HL(db->files_by_id, it, k, v)
        {
            file_destroy_unsafe((File*) v);
        } HL_END;
    }
    g_hash_table_destroy(db->files_by_id);
    g_hash_table_destroy(db->tags);
    g_hash_table_destroy(db->tag_codes);
    g_free(db);
}

void _sqlite_newtag_stmt(TagDB *db, Tag *t)
{
    sqlite3_stmt *stmt = STMT(db, NEWTAG);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    /* XXX: Tag name is transient because tags may be destroyed
     * within the run of the program and we don't want to have
     * to manage sqlite's memory
     */
    sqlite3_bind_text(stmt, 2, tag_name(t), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
}

void _sqlite_newfile_stmt(TagDB *db, File *t)
{
    sqlite3_stmt *stmt = STMT(db, NEWFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(t));
    /* XXX: Tag name is transient because tags may be destroyed
     * within the run of the program and we don't want to have
     * to manage sqlite's memory
     */
    sqlite3_bind_text(stmt, 2, file_name(t), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
}

void _sqlite_delete_file_stmt(TagDB *db, File *f)
{
    sqlite3_stmt *stmt = STMT(db, DELFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(f));
    sqlite3_step(stmt);
}

void _sqlite_delete_tag_stmt(TagDB *db, Tag *t)
{
    sqlite3_stmt *stmt = STMT(db, DELTAG);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(t));
    sqlite3_step(stmt);
}

void _sqlite_rename_file_stmt(TagDB *db, File *f, char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, file_id(f));
    sqlite3_step(stmt);
}

void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENTAG);
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, tag_id(t));
    sqlite3_step(stmt);
}

TagDB *tagdb_new (char *db_fname)
{
    return tagdb_new0(db_fname, 0);
}

TagDB *tagdb_new0 (char *db_fname, int flags)
{
    TagDB *db = calloc(1,sizeof(struct TagDB));
    db->db_fname = db_fname;
    db->sqlite_db_fname = g_strdup_printf("%s.sqldb", db_fname);

    if (flags & TAGDB_CLEAR)
    {
        unlink(db->db_fname);
        unlink(db->sqlite_db_fname);
    }

    int sqlite_flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX;
    /* 256 MB mmap file */
    if (sqlite3_open_v2(db->sqlite_db_fname, &db->sqldb, sqlite_flags, NULL) != SQLITE_OK)
    {
        const char *msg = sqlite3_errmsg(db->sqldb);
        error(msg);
        tagdb_destroy(db);
        return NULL;
    }

    char *errmsg = NULL;
    int sqlite_res = 0;
    sqlite_res = sqlite3_exec(db->sqldb, "create table tag(id integer, name varchar(255), primary key(id,name))", NULL, NULL, &errmsg);
    if (sqlite_res != 0)
    {
        error(errmsg);
        sqlite3_free(errmsg);
    }
    sqlite_res = sqlite3_exec(db->sqldb, "create table file(id integer primary key, name varchar(255))", NULL, NULL, &errmsg);
    if (sqlite_res != 0)
    {
        error(errmsg);
        sqlite3_free(errmsg);
    }
    /* new tag statement */
    sqlite3_prepare_v2(db->sqldb, "insert into tag(id,name) values(?,?)", -1, &STMT(db,NEWTAG), NULL);
    /* new file statement */
    sqlite3_prepare_v2(db->sqldb, "insert into file(id,name) values(?,?)", -1, &STMT(db,NEWFIL), NULL);
    /* delete tag statement */
    sqlite3_prepare_v2(db->sqldb, "delete from tag where id = ?", -1, &STMT(db,DELTAG), NULL);
    /* delete file statement */
    sqlite3_prepare_v2(db->sqldb, "delete from file where id = ?", -1, &STMT(db,DELFIL), NULL);
    /* rename file statement */
    sqlite3_prepare_v2(db->sqldb, "update or ignore file set name = ? where id = ?", -1, &STMT(db,RENFIL), NULL);
    /* rename tag statement */
    sqlite3_prepare_v2(db->sqldb, "update or ignore tag set name = ? where id = ?", -1, &STMT(db,RENTAG), NULL);

    /* lookup tag by id statement */
    sqlite3_prepare_v2(db->sqldb, "select name from tag where id = ?", -1, &STMT(db,STAGID), NULL);

    /* lookup tag by name statement */
    sqlite3_prepare_v2(db->sqldb, "select id from tag where name = ?", -1, &STMT(db,STAGNM), NULL);

    /* lookup file by id statement */
    sqlite3_prepare_v2(db->sqldb, "select name from file where id = ?", -1, &STMT(db,SFILID), NULL);

    /* lookup file by name statement */
    sqlite3_prepare_v2(db->sqldb, "select id from file where name = ?", -1, &STMT(db,SFILNM), NULL);

    db->files = file_cabinet_new_sqlite(db->sqldb);
    file_cabinet_new_drawer(db->files, UNTAGGED);

    db->tags = tag_bucket_new();
    db->tag_codes = g_hash_table_new(g_str_hash, g_str_equal);
    db->files_by_id = g_hash_table_new(g_direct_hash, g_direct_equal);
    db->file_max_id = 0;
    db->tag_max_id = 0;
    db->nfiles = 0;
    return db;
}

TagDB *tagdb_load (char *db_fname)
{
    TagDB *db = tagdb_new(db_fname);

    const char *seps[] = {"\0", NULL};
    Scanner *scn = scanner_new_v(seps);
    if (scanner_set_file_stream(scn, db_fname) == -1)
    {
        fprintf(stderr, "Couldn't open db file\n");
        return NULL;
    }

    tags_from_file(db, scn);
    files_from_file(db, scn);
    scanner_destroy(scn);
    return db;
}
