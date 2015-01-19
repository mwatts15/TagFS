#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <assert.h>
#include "sql.h"
#include "log.h"
#include "util.h"
#include "tagdb_util.h"
#include "file.h"
#include "file_cabinet.h"
#include "set_ops.h"

enum {INSERT,
    REMOVE,
    REMNUL,
    GETFIL,
    GETUNT,
    TAGUNI,
    RMTAGU,
    RMDRWR,
    INSUNT,
    RALLTU,
    RTUDWR,
    LOOKUP,
    LOOKUT,
    TAGUNL,
    NUMBER_OF_STMTS
};

#define STMT(_db,_i) ((_db)->stmts[(_i)])
#define STMT_SEM(_db,_i) (&((_db)->stmt_semas[(_i)]))

struct FileCabinet {
    GHashTable *files;
    sqlite3 *sqlitedb;
    sqlite3_stmt *stmts[NUMBER_OF_STMTS];
    sem_t stmt_semas[NUMBER_OF_STMTS];
    file_id_t max_id;
};

FileCabinet *file_cabinet_new (sqlite3 *db)
{
    FileCabinet *res = calloc(1,sizeof(FileCabinet));
    res->sqlitedb = db;
    return file_cabinet_init(res);
}

void _file_cabinet_init_files(FileCabinet*);
FileCabinet *file_cabinet_init (FileCabinet *res)
{
    res->files = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    sqlite3 *db = res->sqlitedb;
    assert(db);
    for (int i = 0; i < NUMBER_OF_STMTS; i++)
    {
        sem_init(&(res->stmt_semas[i]), 0, 1);
    }

    /* insert statement */
    sql_prepare(db, "insert into file_tag(file, tag) values(?,?)", STMT(res, INSERT));
    /* insert statement */
    sql_prepare(db, "insert into file_tag(file, tag) values(?,NULL)", STMT(res, INSUNT));
    /* insert into tag union */
    sql_prepare(db, "insert into tag_union(tag, assoc, file) values(?,?,?)", STMT(res, TAGUNI));
    /* remove from tag union */
    sql_prepare(db, "delete from tag_union where tag=? and assoc=? and file=?", STMT(res, RMTAGU));
    /* remove all from tag union */
    sql_prepare(db, "delete from tag_union where tag=? and file=?", STMT(res, RALLTU));
    /* remove a tag from the tag union */
    sql_prepare(db, "delete from tag_union where tag=? or assoc=?", STMT(res, RTUDWR));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where file=? and tag is ?", STMT(res, REMOVE));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where file=? and tag is NULL", STMT(res, REMNUL));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where tag is ?", STMT(res, RMDRWR));
    /* files-with-tag statement */
    sql_prepare(db, "select distinct file from file_tag where tag is ?", STMT(res, GETFIL));
    sql_prepare(db, "select distinct file from file_tag where tag is NULL", STMT(res, GETUNT));
    sql_prepare(db, "select distinct F.id from file_tag Z,file F where Z.tag is ? and Z.file=F.id and F.name=?", STMT(res, LOOKUP));
    sql_prepare(db, "select distinct F.id from file_tag Z,file F where Z.tag is NULL and Z.file=F.id and F.name=?", STMT(res, LOOKUT));
    sql_prepare(db, "select distinct assoc from tag_union where tag=?", STMT(res, TAGUNL));
    _file_cabinet_init_files(res);
    return res;
}

void _file_cabinet_init_files(FileCabinet *fc)
{
    /* Reads in the files from the sql database */
    sqlite3_stmt *stmt;
    sql_prepare(fc->sqlitedb, "select distinct * from file", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        File *f = new_file((const char*)name);
        file_id(f) = id;
        if (fc->max_id < id)
            fc->max_id = id;
        g_hash_table_insert(fc->files, TO_SP(file_id(f)), f);
    }
    sqlite3_finalize(stmt);
    sql_prepare(fc->sqlitedb, "select distinct * from file_tag order by file", stmt);
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t file_id = sqlite3_column_int64(stmt, 0);
        file_id_t tag_id = sqlite3_column_int64(stmt, 1);
        File *f = g_hash_table_lookup(fc->files, TO_SP(file_id));
        file_add_tag(f, tag_id, g_strdup(""));
    }
    sqlite3_finalize(stmt);
}

file_id_t file_cabinet_max_id (FileCabinet *fc)
{
    return fc->max_id;
}

void file_cabinet_destroy (FileCabinet *fc)
{
    if (fc)
    {
        for (int i = 0; i < NUMBER_OF_STMTS; i++)
        {
            sqlite3_finalize(STMT(fc,i));
            sem_destroy(&(fc->stmt_semas[i]));
        }

        /* Delete the files */
        if(fc->files)
        {
            HL(fc->files, it, k, v)
            {
                file_destroy_unsafe((File*) v);
            } HL_END;
        }
        g_hash_table_destroy(fc->files);
        free(fc);
    }
}

GList *_sqlite_getfile_stmt(FileCabinet *fc, file_id_t key);
GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id)
{
    GList *res = _sqlite_getfile_stmt(fc, slot_id);
    return res;
}

FilesIter _sqlite_lookup_stmt (FileCabinet *fc, tagdb_key_t key, char *name)
{
    sqlite3_stmt *stmt = NULL;
    if (key_is_empty(key))
    {
        stmt = STMT(fc, LOOKUT);
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    }
    else
    {
        stmt = STMT(fc, LOOKUP);
        file_id_t tag_id = key_ref(key, 0);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, tag_id);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    }
    FilesIter fi = { .stmt = stmt, .fc = fc };
    return fi;
}

File *find_file(FileCabinet *fc, tagdb_key_t key, char *name)
{
    FilesIter it = _sqlite_lookup_stmt(fc, key, name);
    FILES_LOOP(it, f)
    {
        if (f && file_has_tags(f, key))
        {
            return f;
        }
    } FILES_LOOP_END

    return NULL;
}

void _sqlite_rm_drawer_stmt(FileCabinet *fc, file_id_t key);
void _sqlite_remove_tag_from_tag_unions(FileCabinet *fc, file_id_t key);
void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id)
{
    _sqlite_rm_drawer_stmt(fc, slot_id);
    _sqlite_remove_tag_from_tag_unions(fc, slot_id);
}

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, GETFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    int sum = 0;
    int status;
    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        sum++;
    }

    if (status != SQLITE_DONE)
    {
        const char* msg = sqlite3_errmsg(fc->sqlitedb);
        error("We didn't finish the count SQLite statemnt: %s(%d)", msg, status);
    }
    return sum;
}

GList *_sqlite_getfile_stmt(FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt;
    int status;
    int stmt_code;
    if (key)
    {
        stmt = STMT(fc, GETFIL);
        stmt_code = GETFIL;
    }
    else
    {
        stmt = STMT(fc, GETUNT);
        stmt_code = GETUNT;
    }

    sem_wait(STMT_SEM(fc,stmt_code));
    sqlite3_reset(stmt);

    if (key)
    {
        sqlite3_bind_int(stmt, 1, key);
    }

    GList *res = NULL;

    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        /* get the actual file */
        File *f = g_hash_table_lookup(fc->files, TO_P(id));
        if (f)
        {
            res = g_list_prepend(res, f);
        }
    }

    if (status != SQLITE_DONE)
    {
        const char* msg = sqlite3_errmsg(fc->sqlitedb);
        error("We didn't finish the getfile SQLite statemnt: %s(%d)", msg, status);
    }
    sem_post(STMT_SEM(fc,stmt_code));
    return res;
}

void _sqlite_rm_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = NULL;
    if (key)
    {
        stmt = STMT(fc, REMOVE);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, file_id(f));
        sqlite3_bind_int(stmt, 2, key);
    }
    else
    {
        stmt = STMT(fc, REMNUL);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, file_id(f));
    }
    sql_step(stmt);
}

void _sqlite_rm_drawer_stmt(FileCabinet *fc, file_id_t key)
{
    if (key)
    {
        sqlite3_stmt *stmt = STMT(fc, RMDRWR);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, key);
        sql_step(stmt);
    }
}

void _sqlite_tag_union_stmt(FileCabinet *fc, File *f, file_id_t t_key, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, TAGUNI);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, t_key);
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_bind_int(stmt, 3, file_id(f));
    sql_step(stmt);
}

GList *_sqlite_tag_union_list_stmt(FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, TAGUNL);
    sem_wait(STMT_SEM(fc,TAGUNL));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    GList *res = NULL;
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        res = g_list_prepend(res, TO_SP(id));
    }
    sem_post(STMT_SEM(fc, TAGUNL));
    return res;
}

void _sqlite_remove_from_tag_union_stmt(FileCabinet *fc, File *f, file_id_t t_key, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, RMTAGU);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, t_key);
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_bind_int(stmt, 3, file_id(f));
    sql_step(stmt);
}

void _sqlite_remove_all_from_tag_union_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, RALLTU);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    sqlite3_bind_int(stmt, 2, file_id(f));
    sql_step(stmt);
}

void _sqlite_remove_tag_from_tag_unions(FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, RTUDWR);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    sqlite3_bind_int(stmt, 2, key);
    sql_step(stmt);
}

void _sqlite_ins_stmt (FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = NULL;
    if (key)
    {
        stmt = STMT(fc, INSERT);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, file_id(f));
        sqlite3_bind_int(stmt, 2, key);
    }
    else
    {
        stmt = STMT(fc, INSUNT);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, file_id(f));
    }
    sql_step(stmt);
}

void file_cabinet_remove (FileCabinet *fc, file_id_t key, File *f)
{
    _sqlite_rm_stmt(fc,f,key);
    _sqlite_remove_all_from_tag_union_stmt(fc, f, key);
    /* NOTE: Although we always want to insert a file into fc->files on
     * insert, we never want to delete the file since it could remain in
     * any of the "drawers"
     */
}

void file_cabinet_remove_v (FileCabinet *fc, tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        file_cabinet_remove(fc, key_ref(key,i), f);
    } KL_END;
}

void file_cabinet_remove_all (FileCabinet *fc, File *f)
{
    tagdb_key_t key = file_extract_key(f);
    file_cabinet_remove(fc, UNTAGGED, f);
    file_cabinet_remove_v(fc, key, f);
    key_destroy(key);
}

void file_cabinet_delete_file(FileCabinet *fc, File *f)
{
    int rem = g_hash_table_remove(fc->files, TO_SP(file_id(f)));
    assert(rem);
    if (!file_destroy(f))
    {
        error("Could not destroy file: %s", file_name(f));
    }
}

void file_cabinet_insert (FileCabinet *fc, file_id_t key, File *f)
{
    /* XXX: Consider not doing this on every insert */
    g_hash_table_insert(fc->files, TO_SP(file_id(f)), f);
    _sqlite_ins_stmt(fc,f,key);

    tagdb_key_t fkey = file_extract_key(f);
    KL(fkey, i)
    {
        key_elem_t k = key_ref(fkey, i);
        if (k != key)
        {
            _sqlite_tag_union_stmt(fc, f, key, k);
            _sqlite_tag_union_stmt(fc, f, k, key);
        }
    } KL_END;
    key_destroy(fkey);
}

void file_cabinet_insert_v (FileCabinet *fc, const tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        file_cabinet_insert(fc, key_ref(key,i), f);
    } KL_END
}

gulong file_cabinet_size (FileCabinet *fc)
{
    return g_hash_table_size(fc->files);
}

/* Lookup a file with the given name and tags */
File *file_cabinet_lookup_file (FileCabinet *fc, tagdb_key_t key, char *name)
{
    return find_file(fc, key, name);
}

File *file_cabinet_get_file_by_id(FileCabinet *fc, file_id_t id)
{
    return g_hash_table_lookup(fc->files, TO_P(id));
}

GList *file_cabinet_get_drawer_tags (FileCabinet *fc, file_id_t slot_id)
{
    return _sqlite_tag_union_list_stmt(fc, slot_id);
}
