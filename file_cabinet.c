#include <stdlib.h>
#include <glib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <assert.h>
#include "log.h"
#include "util.h"
#include "tagdb_util.h"
#include "file.h"
#include "file_cabinet.h"
#include "set_ops.h"

#define INSERT 0
#define REMOVE 1
#define GETFIL 2
#define TAGUNI 3
#define SUBTAG 4
#define RMTAGU 5
#define RALLTU 6
#define NUMBER_OF_STMTS 7

#define STMT(_db,_i) ((_db)->stmts[(_i)])

struct FileCabinet{
    GHashTable *fc;
    GHashTable *files;
    sqlite3 *sqlitedb;
    sqlite3_stmt *stmts[16];
};

/* Returns the keyed file slot */
FileDrawer *file_cabinet_get_drawer (FileCabinet *fc, file_id_t slot_id);

FileCabinet *file_cabinet_new (sqlite3 *db)
{
    FileCabinet *res = malloc(sizeof(FileCabinet));
    res->sqlitedb = db;
    return file_cabinet_init(res);

}

FileCabinet *file_cabinet_init (FileCabinet *res)
{
    res->fc = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) file_drawer_destroy);
    res->files = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);

    sqlite3 *db = res->sqlitedb;
    assert(db);
    /* a table associating tags to files */
    sqlite3_exec(db, "create table file_tag(file integer, tag integer)", NULL, NULL, NULL);

    /* a table associating tags to tags with shared files
     * the first column is the containing tag, the second is the
     * associated tag and the third is the associated file */
    sqlite3_exec(db, "create table tag_union(tag integer, assoc integer, file integer)", NULL, NULL, NULL);

    /* a table associating tags to sub-tags. TODO*/
    sqlite3_exec(db, "create table subtag(super integer, sub integer)", NULL, NULL, NULL);

    /* see the sqlite documentation (https://www.sqlite.org/c3ref/prepare.html) for more info */
    /* insert statement */
    sqlite3_prepare_v2(db, "insert into file_tag(file, tag) values(?,?)", -1, &STMT(res, INSERT), NULL);
    /* insert into tag union */
    sqlite3_prepare_v2(db, "insert into tag_union(tag, assoc, file) values(?,?,?)", -1, &STMT(res, TAGUNI), NULL);
    /* insert into subtags */
    sqlite3_prepare_v2(db, "insert into subtag(super, sub) values(?,?)", -1, &STMT(res, SUBTAG), NULL);
    /* remove from tag union */
    sqlite3_prepare_v2(db, "delete from tag_union where tag=? and assoc=? and file=?", -1, &STMT(res, RMTAGU), NULL);
    /* remove all from tag union */
    sqlite3_prepare_v2(db, "delete from tag_union where tag=? and file=?", -1, &STMT(res, RALLTU), NULL);
    /* remove statement */
    sqlite3_prepare_v2(db, "delete from file_tag where file=? and tag=?", -1, &STMT(res, REMOVE), NULL);
    /* files-with-tag statement */
    sqlite3_prepare_v2(db, "select file from file_tag where tag=?", -1, &STMT(res, GETFIL), NULL);
    return res;
}

void file_cabinet_destroy (FileCabinet *fc)
{
    if (fc)
    {
        g_hash_table_destroy(fc->fc);
        g_hash_table_destroy(fc->files);
        for (int i = 0; i < NUMBER_OF_STMTS; i++)
        {
            sqlite3_finalize(STMT(fc,i));
        }
        free(fc);
    }
}

GList *file_cabinet_get_drawer_labels (FileCabinet *fc)
{
    return g_hash_table_get_keys(fc->fc);
}

FileDrawer *file_cabinet_get_drawer (FileCabinet *fc, file_id_t slot_id)
{
    return (FileDrawer*) g_hash_table_lookup(fc->fc, TO_SP(slot_id));
}

GList *_sqlite_getfile_stmt(FileCabinet *fc, file_id_t key);
GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id)
{
    GList *res = _sqlite_getfile_stmt(fc, slot_id);
    /*return file_drawer_as_list(file_cabinet_get_drawer(fc, slot_id));*/
    return res;
}

void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id)
{
    g_hash_table_remove(fc->fc, TO_SP(slot_id));
}

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key)
{
    return file_drawer_size(file_cabinet_get_drawer(fc, key));
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
        error("We didn't finish the getfile SQLite statemnt:%s(%d)", msg, status);
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
    FileDrawer *fs = g_hash_table_lookup(fc->fc, TO_SP(key));
    if (fs)
        file_drawer_remove(fs, f);
    else
        error("Attempting to remove a file drawer that doesn't exists");
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
    file_cabinet_remove(fc, UNTAGGED, f);/* XXX: Possibly a better place exists to do this */
    file_cabinet_remove_v(fc, key, f);
    key_destroy(key);
}

void file_cabinet_insert (FileCabinet *fc, file_id_t key, File *f)
{
    FileDrawer *fs = g_hash_table_lookup(fc->fc, TO_SP(key));
    if (fs)
        file_drawer_insert(fs, f);
    g_hash_table_insert(fc->files, TO_SP(file_id(f)), f);
    _sqlite_ins_stmt(fc,f,key);

    tagdb_key_t fkey = file_extract_key(f);
    KL(fkey, i)
    {
        _sqlite_tag_union_stmt(fc, f, key, key_ref(fkey,i));
    } KL_END;
    key_destroy(fkey);
}

void file_cabinet_insert_v (FileCabinet *fc, const tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        if (g_hash_table_lookup(fc->fc, TO_SP(key_ref(key, i))) == NULL)
        {
            return;
        }
    } KL_END

    KL(key, i)
    {
        file_cabinet_insert(fc, key_ref(key,i), f);
    } KL_END
}

void file_cabinet_new_drawer (FileCabinet *fc, file_id_t slot_id)
{
    g_hash_table_insert(fc->fc, TO_SP(slot_id), file_drawer_new(slot_id));
}

gulong file_cabinet_size (FileCabinet *fc)
{
    return g_hash_table_size(fc->fc);
}

/* Lookup a file with the given name and tags */
File *file_cabinet_lookup_file (FileCabinet *fc, tagdb_key_t key, char *name)
{
    if (key == NULL)
    {
        return NULL;
    }
    File *f = NULL;
    int n = 0;

    KL(key, i)
    {
        FileDrawer *fs = file_cabinet_get_drawer(fc, key_ref(key, i));
        f = file_drawer_lookup1(fs, name, 0);
        if (f && file_has_tags(f, key))
        {
            return f;
        }
        n = i;
    } KL_END;

    if (n == 0)
    {
        FileDrawer *fs = file_cabinet_get_drawer(fc, UNTAGGED);
        f = file_drawer_lookup1(fs, name, 0);
    }

    return f;
}

GList *file_cabinet_tag_intersection(FileCabinet *fc, tagdb_key_t key)
{
    int skip = 1;
    GList *res = NULL;
    KL(key, i)
    {
        FileDrawer *d = file_cabinet_get_drawer(fc, key_ref(key, i));
        if (d)
        {
            GList *this_drawer = file_drawer_get_tags(d);
            this_drawer = g_list_sort(this_drawer, (GCompareFunc) long_cmp);

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
        }
        skip = 0;
    } KL_END;
    return res;
}
