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
    GETFIL,
    TAGUNI,
    SUBTAG,
    RMTAGU,
    RALLTU,
    LOOKUP,
    TAGUNL,
    NUMBER_OF_STMTS
};

#define STMT(_db,_i) ((_db)->stmts[(_i)])

struct FileCabinet{
    GHashTable *files;
    sqlite3 *sqlitedb;
    sqlite3_stmt *stmts[16];
};

FileCabinet *file_cabinet_new (sqlite3 *db)
{
    FileCabinet *res = malloc(sizeof(FileCabinet));
    res->sqlitedb = db;
    return file_cabinet_init(res);

}

FileCabinet *file_cabinet_init (FileCabinet *res)
{
    res->files = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);

    sqlite3 *db = res->sqlitedb;
    assert(db);
    /* a table associating tags to files */
    sql_exec(db, "create table file_tag(file integer, tag integer, primary key (file,tag))");

    /* a table associating tags to tags with shared files
     * the first column is the containing tag, the second is the
     * associated tag and the third is the associated file */
    sql_exec(db, "create table tag_union(tag integer, assoc integer, file integer, primary key (tag,assoc,file))");

    /* a table associating tags to sub-tags. TODO*/
    sql_exec(db, "create table subtag(super integer, sub integer)");

    /* see the sqlite documentation (https://www.sqlite.org/c3ref/prepare.html) for more info */
    /* insert statement */
    sql_prepare(db, "insert into file_tag(file, tag) values(?,?)", STMT(res, INSERT));
    /* insert into tag union */
    sql_prepare(db, "insert into tag_union(tag, assoc, file) values(?,?,?)", STMT(res, TAGUNI));
    /* insert into subtags */
    sql_prepare(db, "insert into subtag(super, sub) values(?,?)", STMT(res, SUBTAG));
    /* remove from tag union */
    sql_prepare(db, "delete from tag_union where tag=? and assoc=? and file=?", STMT(res, RMTAGU));
    /* remove all from tag union */
    sql_prepare(db, "delete from tag_union where tag=? and file=?", STMT(res, RALLTU));
    /* remove statement */
    sql_prepare(db, "delete from file_tag where file=? and tag=?", STMT(res, REMOVE));
    /* files-with-tag statement */
    sql_prepare(db, "select distinct file from file_tag where tag=?", STMT(res, GETFIL));
    sql_prepare(db, "select distinct F.id from file_tag Z,file F where Z.tag=? and Z.file=F.id and F.name=?", STMT(res, LOOKUP));
    sql_prepare(db, "select distinct assoc from tag_union tag=?", STMT(res, TAGUNL));
    return res;
}

void file_cabinet_destroy (FileCabinet *fc)
{
    if (fc)
    {
        for (int i = 0; i < NUMBER_OF_STMTS; i++)
        {
            sqlite3_finalize(STMT(fc,i));
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

File *_sqlite_lookup_stmt(FileCabinet *fc, tagdb_key_t key, char *name)
{
    sqlite3_stmt *stmt = NULL;
    file_id_t tag_id;
    if (key_is_empty(key))
    {
        tag_id = 0;
    }
    else
    {
        tag_id = key_ref(key, 0);
    }
    stmt = STMT(fc, LOOKUP);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);

    int status;
    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        File *f = g_hash_table_lookup(fc->files, TO_P(id));
        if (f && file_has_tags(f, key))
        {
            return f;
        }
    }

    if (status != SQLITE_DONE)
    {
        const char* msg = sqlite3_errmsg(fc->sqlitedb);
        error("We didn't finish the getfile SQLite statemnt: %s(%d)", msg, status);
    }
    return NULL;
}

void _sqlite_begin_transaction(FileCabinet *fc)
{
    sql_exec(fc->sqlitedb, "begin transaction");
}

void _sqlite_commit_transaction(FileCabinet *fc)
{
    sql_exec(fc->sqlitedb, "commit");
}

void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id)
{}

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
    sqlite3_stmt *stmt = STMT(fc, GETFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    int status;
    GList *res = NULL;

    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        /* get the actual file */
        File *f = g_hash_table_lookup(fc->files, TO_P(id));
        res = g_list_prepend(res, f);
    }

    if (status != SQLITE_DONE)
    {
        const char* msg = sqlite3_errmsg(fc->sqlitedb);
        error("We didn't finish the getfile SQLite statemnt: %s(%d)", msg, status);
    }
    return res;
}

void _sqlite_rm_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, REMOVE);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(f));
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_step(stmt);
}

void _sqlite_tag_union_stmt(FileCabinet *fc, File *f, file_id_t t_key, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, TAGUNI);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, t_key);
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_bind_int(stmt, 3, file_id(f));
    sqlite3_step(stmt);
}

GList *_sqlite_tag_union_list_stmt(FileCabinet *fc, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, TAGUNL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    int status;
    GList *res = NULL;
    while ((status = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        res = g_list_prepend(res, TO_SP(id));
    }
    return res;
}

void _sqlite_remove_from_tag_union_stmt(FileCabinet *fc, File *f, file_id_t t_key, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, RMTAGU);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, t_key);
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_bind_int(stmt, 3, file_id(f));
    sqlite3_step(stmt);
}

void _sqlite_remove_all_from_tag_union_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, RALLTU);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, key);
    sqlite3_bind_int(stmt, 2, file_id(f));
    sqlite3_step(stmt);

}

void _sqlite_ins_stmt(FileCabinet *fc, File *f, file_id_t key)
{
    sqlite3_stmt *stmt = STMT(fc, INSERT);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, file_id(f));
    sqlite3_bind_int(stmt, 2, key);
    sqlite3_step(stmt);
}

void file_cabinet_remove (FileCabinet *fc, file_id_t key, File *f)
{
    _sqlite_begin_transaction(fc);
    _sqlite_rm_stmt(fc,f,key);
    _sqlite_remove_all_from_tag_union_stmt(fc, f, key);
    _sqlite_commit_transaction(fc);
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
    g_hash_table_insert(fc->files, TO_SP(file_id(f)), f);
    _sqlite_begin_transaction(fc);
    _sqlite_ins_stmt(fc,f,key);

    tagdb_key_t fkey = file_extract_key(f);
    KL(fkey, i)
    {
        _sqlite_tag_union_stmt(fc, f, key, key_ref(fkey,i));
    } KL_END;
    key_destroy(fkey);
    _sqlite_commit_transaction(fc);
}

void file_cabinet_insert_v (FileCabinet *fc, const tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        file_cabinet_insert(fc, key_ref(key,i), f);
    } KL_END
}

void file_cabinet_new_drawer (FileCabinet *fc, file_id_t slot_id)
{}

gulong file_cabinet_size (FileCabinet *fc)
{
    return 0;
}

/* Lookup a file with the given name and tags */
File *file_cabinet_lookup_file (FileCabinet *fc, tagdb_key_t key, char *name)
{
    return _sqlite_lookup_stmt(fc, key, name);
}

File *file_cabinet_get_file_by_id(FileCabinet *fc, file_id_t id)
{
    return g_hash_table_lookup(fc->files, TO_P(id));
}

GList *file_cabinet_tag_intersection(FileCabinet *fc, tagdb_key_t key)
{
    int skip = 1;
    GList *res = NULL;
    KL(key, i)
    {
        GList *this_drawer = _sqlite_tag_union_list_stmt(fc,key_ref(key, i));
        this_drawer = g_list_sort(this_drawer, (GCompareFunc) long_cmp);
        LL(this_drawer, it)
        {
            printf("%llu\n", (file_id_t)it->data);
        }
        GList *tmp = NULL;
        if (skip)
        {
            tmp = g_list_copy(this_drawer);
        }
        else
        {
            tmp = g_list_intersection(res, this_drawer, (GCompareFunc) long_cmp);
        }

        g_list_free(this_drawer);
        g_list_free(res);

        res = tmp;
        skip = 0;
    } KL_END;

    return res;
}
