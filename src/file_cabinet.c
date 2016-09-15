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
    /* An index on files. Usually provided to us by tagdb */
    GHashTable *files;
    /* Indicates whether we created the FILES ourselves */
    gboolean own_files;
    /* The sqlite database */
    sqlite3 *sqlitedb;
    /* The sql prepared statements that we use */
    sqlite3_stmt *stmts[NUMBER_OF_STMTS];
    /* Semaphores to protect prepared statements from being reset while they are executing */
    sem_t stmt_semas[NUMBER_OF_STMTS];
};

FileCabinet *file_cabinet_new0 (sqlite3 *db, GHashTable *files)
{
    FileCabinet *res = calloc(1,sizeof(FileCabinet));
    res->sqlitedb = db;
    res->files = files;
    return file_cabinet_init(res);
}

FileCabinet *file_cabinet_new (sqlite3 *db)
{
    GHashTable *files = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    FileCabinet *fc = file_cabinet_new0(db, files);
    fc->own_files = TRUE;
    return fc;
}

FileCabinet *file_cabinet_init (FileCabinet *res)
{
    sqlite3 *db = res->sqlitedb;
    assert(db);
    for (int i = 0; i < NUMBER_OF_STMTS; i++)
    {
        sem_init(&(res->stmt_semas[i]), 0, 1);
    }

    /* insert statement */
    sql_prepare(db, "insert into file_tag(file, tag) values(?,?)", STMT(res, INSERT));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where file=? and tag is ?", STMT(res, REMOVE));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where tag is ?", STMT(res, RMDRWR));
    /* files-with-tag statement */
    sql_prepare(db, "select distinct file from file_tag where tag is ?", STMT(res, GETFIL));
    sql_prepare(db, "select distinct id from file where id not in (select file from file_tag)", STMT(res, GETUNT));
    sql_prepare(db, "select distinct F.id from file_tag Z,file F where Z.tag is ? and Z.file=F.id and F.name=?", STMT(res, LOOKUP));
    sql_prepare(db, "select distinct F.id"
            " from file F"
            " where F.name=?"
            " and F.id not in (select file from file_tag)", STMT(res, LOOKUT));
    sql_prepare(db, "select distinct b.tag"
            " from file_tag a, file_tag b"
            " where a.tag=?"
            " and a.file=b.file"
            " and a.tag!=b.tag", STMT(res, TAGUNL));
    return res;
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

        if (fc->own_files && fc->files)
        {
            HL (fc->files, it, k, v)
            {
                file_destroy_unsafe((File*) v);
            } HL_END;
            g_hash_table_destroy(fc->files);
        }

        free(fc);
    }
}

GList *_sqlite_getfile_stmt(FileCabinet *fc, file_id_t key);
GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id)
{
    GList *res = _sqlite_getfile_stmt(fc, slot_id);
    return res;
}

File *_sqlite_lookup_stmt (FileCabinet *fc, tagdb_key_t key, const char *name)
{
    // TODO:
    // 1. lookup the first key element and the name in the cache
    // 2. if we have it in the cache, get the file from the fc->files
    //    table
    int stmt_code = -1;
    sqlite3_stmt *stmt = NULL;
    sem_t *stmt_sem;

    if (key_is_empty(key))
    {
        stmt_code = LOOKUT;
    }
    else
    {
        stmt_code = LOOKUP;
    }

    stmt = STMT(fc, stmt_code);
    stmt_sem = STMT_SEM(fc, stmt_code);
    sem_wait(stmt_sem);
    sqlite3_reset(stmt);

    if (stmt_code == LOOKUT)
    {
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    }
    else
    {
        file_id_t tag_id = key_ref(key, 0);
        sqlite3_bind_int(stmt, 1, tag_id);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    }

    File *res = NULL;
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0); // XXX: Should this be sqlite_column_int64 instead?
        File *f = g_hash_table_lookup(fc->files, TO_P(id));
        if (f)
        {
            if (res)
            {
                if (file_only_has_tags(f, key))
                {
                    res = f;
                }
            }
            else
            {
                if (file_has_tags(f, key))
                {
                    res = f;
                }
            }
        }
        else
        {
            warn("Unknown file ID %lld returned from the database", id);
        }
    }

    sem_post(stmt_sem);
    return res;
}

void _sqlite_rm_drawer_stmt(FileCabinet *fc, file_id_t key);
void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id)
{
    _sqlite_rm_drawer_stmt(fc, slot_id);
}

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, GETFIL);
    sem_t *stmt_sem = STMT_SEM(fc, GETFIL);
    sem_wait(stmt_sem);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    int sum = 0;
    int status;
    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        sum++;
    }
    sem_post(stmt_sem);

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
    sem_t *stmt_sem;
    int status;
    int stmt_code;
    if (key)
    {
        stmt_code = GETFIL;
    }
    else
    {
        stmt_code = GETUNT;
    }

    stmt_sem = STMT_SEM(fc, stmt_code);
    sem_wait(stmt_sem);
    stmt = STMT(fc, stmt_code);
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
    // insert results into cache
    sem_post(stmt_sem);
    return res;
}

void _sqlite_rm_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    // invalidate cache entry for key
    int stmt_code;
    sqlite3_stmt *stmt = NULL;
    sem_t *stmt_sem;

    if (key)
    {
        stmt_code = REMOVE;
    }
    else
    {
        stmt_code = REMNUL;
    }
    stmt = STMT(fc, stmt_code);
    stmt_sem = STMT_SEM(fc, stmt_code);

    sem_wait(stmt_sem);

    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(f));
    if (key)
    {
        sqlite3_bind_int(stmt, 2, key);
    }

    sql_step(stmt);
    sem_post(stmt_sem);
}

void _sqlite_rm_drawer_stmt(FileCabinet *fc, file_id_t key)
{
    // invalidate cache entry for key
    if (key)
    {
        sqlite3_stmt *stmt = STMT(fc, RMDRWR);
        sem_wait(STMT_SEM(fc, RMDRWR));
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, key);
        sql_step(stmt);
        sem_post(STMT_SEM(fc, RMDRWR));
    }
}

tagdb_key_t _sqlite_tag_union_key_stmt(FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, TAGUNL);
    sem_wait(STMT_SEM(fc,TAGUNL));
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    tagdb_key_t res = key_new();
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        key_push_end(res, id);
    }
    sem_post(STMT_SEM(fc, TAGUNL));
    return res;
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

void _sqlite_ins_stmt (FileCabinet *fc, File *f, file_id_t key)
{
    // update cache
    sqlite3_stmt *stmt = NULL;
    if (key)
    {
        stmt = STMT(fc, INSERT);
        sem_wait(STMT_SEM(fc, INSERT));
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, file_id(f));
        sqlite3_bind_int(stmt, 2, key);
        sql_step(stmt);
        sem_post(STMT_SEM(fc, INSERT));
    }
}

void file_cabinet_remove (FileCabinet *fc, file_id_t key, File *f)
{
    _sqlite_rm_stmt(fc,f,key);
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
    file_cabinet_remove_v(fc, key, f);
    key_destroy(key);
}

GList *file_cabinet_get_untagged_files (FileCabinet *fc)
{
    return _sqlite_getfile_stmt(fc, 0);
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

    if (G_UNLIKELY(fc->own_files))
    {
        g_hash_table_insert(fc->files, TO_SP(file_id(f)), f);
    }
    _sqlite_ins_stmt(fc,f,key);
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
File *file_cabinet_lookup_file (FileCabinet *fc, tagdb_key_t key, const char *name)
{
    return _sqlite_lookup_stmt(fc, key, name);
}

GList *file_cabinet_get_drawer_tags (FileCabinet *fc, file_id_t slot_id)
{
    return _sqlite_tag_union_list_stmt(fc, slot_id);
}

tagdb_key_t file_cabinet_get_drawer_tags_a (FileCabinet *fc, file_id_t slot_id)
{
    return _sqlite_tag_union_key_stmt(fc, slot_id);
}
