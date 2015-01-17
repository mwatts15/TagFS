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
    SUBTAG ,
    REMSUB ,
    REMSUP ,
    REMSTA ,
    NUMBER_OF_STMTS };
#define STMT(_db,_i) ((_db)->sql_stmts[(_i)])
void _sqlite_newtag_stmt(TagDB *db, Tag *t);
void _sqlite_newfile_stmt(TagDB *db, File *t);
void _sqlite_rename_file_stmt(TagDB *db, File *f, const char *new_name);
void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, const char *new_name);
void _sqlite_delete_file_stmt(TagDB *db, File *t);
void _sqlite_delete_tag_stmt(TagDB *db, Tag *t);
void _sqlite_subtag_ins_stmt(TagDB *db, Tag *super, Tag *sub);
void _sqlite_subtag_rem_sub(TagDB *db, Tag *sub);
void _sqlite_subtag_rem_sup(TagDB *db, Tag *sup);
void _sqlite_subtag_del_stmt(TagDB *db, Tag *super, Tag *sub);

file_id_t tag_name_to_id (TagDB *db, const char *tag_name);
/* retrieves a root tag by its name using the tag_codes table */
Tag *retrieve_root_tag_by_name (TagDB *db, const char *tag_name);
/* Deletes the tag_codes entry for named tag */
void clear_root_tag_by_name (TagDB *db, const char *tag_name);
/* Deletes the tag_codes entry for the tag */
void clear_root_tag (TagDB *db, Tag *tag);
/* Like tagdb_tag_remove_subtag, but only requires the subtag */
void tagdb_tag_remove_subtag1 (TagDB *db, Tag *sub);

/* Inserts the tag into the tag bucket as well as creating a file slot
   in the file bucket */
void insert_tag (TagDB *db, Tag *t);

/* Ensures that the database remains consistent for a tag deletion */
int delete_tag0 (TagDB *db, Tag *t);

GList *tagdb_untagged_items (TagDB *db)
{
    return file_cabinet_get_drawer_l(db->files, UNTAGGED);
}

GList *tagdb_all_files (TagDB *db)
{
    return NULL;
}

GList *tagdb_all_tags (TagDB *db)
{
    return g_hash_table_get_values(db->tags);
}

void set_file_name (TagDB *db, File *f, const char *new_name)
{
    set_name(f, new_name);
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

    if (!tag_parent(t))
    {
        clear_root_tag(db, t);
    }

    char *s = g_strdup(new_name);
    char *tag_path_base_name = tag_path_split_right1(s);
    if (tag_path_base_name)
    {
        Tag *new_parent = tagdb_make_tag(db, s);
        tag_set_name(t, tag_path_base_name);
        tagdb_tag_set_subtag(db, new_parent, t);
    }
    else
    {
        tag_set_name(t, new_name);
        tag_path_base_name = s;
        if (tag_parent(t))
        {
            tagdb_tag_remove_subtag1(db, t);
        }
        g_hash_table_insert(db->tag_codes, (gpointer) tag_name(t), TO_SP(tag_id(t)));
    }
    _sqlite_rename_tag_stmt(db, t, tag_path_base_name);

    g_free(s);
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

/* This guy needs to take a tag path, create each of the tags in the path,
 * establish their subtag relationships, insert the tags into the TagDB,
 * and return the end tag. It does this by getting TagPathInfo from the
 * tag_path (which can just be a tag name) going element by element,
 * creating each tag and saving it in the TagPathInfo, putting subtag
 * relationships between subsequent tags, calling insert_tag on each tag
 * after that, and finally extracting the last Tag in the TagPathInfo
 * deleting the TagPathInfo, and returning with that last Tag.
 */
Tag *tagdb_make_tag(TagDB *db, const char *tag_path)
{
    Tag *res = NULL;
    TagPathInfo *tpi = tag_process_path(tag_path);
    if (tag_path_info_is_empty(tpi))
    {
        goto TAGDB_MAKE_TAG_END;
    }

    TagPathElementInfo *root_elt = tag_path_info_first_element(tpi);
    Tag *root = retrieve_root_tag_by_name(db, tag_path_element_info_name(root_elt));
    Tag *last = NULL;
    gboolean already_resolved = tag_path_info_add_tags(tpi, root, &last);
    if (already_resolved)
    {
        res = last;
        goto TAGDB_MAKE_TAG_END;
    }
    else
    {
        gboolean past_last_resolved_tag = FALSE;
        Tag *previous_tag = NULL;
        TPIL(tpi, it, tei)
        {
            Tag *this_tag = tag_path_element_info_get_tag(tei);
            if (!this_tag)
            {
                past_last_resolved_tag = TRUE;
            }

            if (past_last_resolved_tag)
            {
                    const char *tname = tag_path_element_info_name(tei);
                    this_tag = new_tag(tname, 0, 0);
                    tag_path_element_info_set_tag(tei, this_tag);
                    insert_tag(db, this_tag);
                    if (previous_tag)
                    {
                        tagdb_tag_set_subtag(db, previous_tag, this_tag);
                    }
                    res = this_tag;
                    assert(this_tag);
            }
            else if (this_tag == last)
            {
                past_last_resolved_tag = TRUE;
            }
            previous_tag = this_tag;
        } TPIL_END;
    }

    TAGDB_MAKE_TAG_END:
    tag_path_info_destroy(tpi);
    return res;
}

File *tagdb_make_file(TagDB *db, const char *file_name)
{
    File *f = new_file(file_name);
    insert_file(db, f);
    return f;
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
    Tag *preexisting_tag = retrieve_root_tag_by_name(db, tag_name(t));

    if (!tag_id(t))
        tag_id(t) = ++db->tag_max_id;

    TSUBL(t, it, child){
        Tag *s = retrieve_root_tag_by_name(db, tag_name(child));
        if (s == child)
        {
            clear_root_tag(db, child);
        }
    } TSUBL_END;


    if (!tag_parent(t))
    {
        if (!preexisting_tag)
        {
            g_hash_table_insert(db->tag_codes, (gpointer) tag_name(t), TO_SP(tag_id(t)));
        }
    }
    tag_bucket_insert(db, t);
    _sqlite_newtag_stmt(db, t);
}

void tagdb_tag_set_subtag (TagDB *db, Tag *sup, Tag *sub)
{
    if (tag_get_child(sup, tag_name(sub)))
    {
        return;
    }

    if (!tag_parent(sub))
    {
        /* If the subtag is a root tag, then we have to clear its root status */
        Tag *t = retrieve_root_tag_by_name(db, tag_name(sub));
        if (t == sub)
        {
            clear_root_tag(db, t);
        }
    }
    tag_set_subtag(sup, sub);
    _sqlite_subtag_rem_sub(db,sub);
    _sqlite_subtag_ins_stmt(db,sup,sub);
}

void tagdb_tag_remove_subtag (TagDB *db, Tag *sup, Tag *sub)
{
    tag_remove_subtag(sup, sub);
    _sqlite_subtag_del_stmt(db, sup, sub);
}

void tagdb_tag_remove_subtag1 (TagDB *db, Tag *sub)
{
    Tag *sup = tag_parent(sub);
    tagdb_tag_remove_subtag(db, sup, sub);
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
    }

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

File *retrieve_file (TagDB *db, file_id_t id)
{
    return file_cabinet_get_file_by_id(db->files, id);
}

GList *tag_files(TagDB *db, Tag *t)
{
    return file_cabinet_get_drawer_l(db->files, tag_id(t));

}

File *lookup_file (TagDB *db, tagdb_key_t keys, char *name)
{
    /* XXX: This is likely to be quite slow when looking up many files */
    GList *l = get_files_list(db, keys);
    File *res = NULL;
    LL(l, it)
    {
        if (file_name_str_cmp((AbstractFile*)it->data, name) == 0)
        {
            res = it->data;
            break;
        }
    }LL_END;
    g_list_free(l);
    return res;
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
        return 0;
    }

    GList *children = NULL;
    int res = 1;

    /* We first check if the sub-tags of this tag have a name conflict with
     * any root tags and fail the operation if they do
     */
    TSUBL(t, it, child)
    {
        children = g_list_prepend(children, child);
    }TSUBL_END

    tag_bucket_remove(db, t);
    clear_root_tag(db, t);
    file_cabinet_remove_drawer(db->files, tag_id(t));

    if (!tag_parent(t))
    {
        /* If we're deleting a root-tag, then in order to promote the tags
         * to the root level, we have to remove the sub-tags in advance of
         * the tag subsystem doing it in tag_destroy
         */
        LL(children, it)
        {
            tag_remove_subtag(t, it->data);
        }LL_END;
        _sqlite_subtag_rem_sup(db, t);
    }

    _sqlite_delete_tag_stmt(db, t);

    if (!tag_parent(t))
    {
        LL(children, it)
        {
            insert_tag(db, it->data);
        }LL_END;
    }

    g_list_free(children);

    return res;
}

gboolean delete_tag (TagDB *db, Tag *t)
{
    if (delete_tag0(db, t))
    {
        tag_destroy(t);
        return TRUE;
    }

    return FALSE;
}

gboolean can_remove_tag (TagDB *db, Tag *t)
{
    int res = TRUE;

    /* We first check if the sub-tags of this tag have a name conflict with
     * any root tags and fail the operation if they do
     */
    Tag *parent = tag_parent(t);
    if (parent)
    {
        TSUBL(t, it, child)
        {
            if (tag_has_child(parent, tag_name(child)))
            {
                res = FALSE;
                break;
            }
        } TSUBL_END;
    }
    else
    {
        TSUBL(t, it, child)
        {
            if (retrieve_root_tag_by_name(db, tag_name(child)))
            {
                res = FALSE;
                break;
            }
        } TSUBL_END;
    }

    return res;
}

Tag *lookup_tag (TagDB *db, const char *tag_name)
{
    Tag *res = NULL;

    TagPathInfo *tpi = tag_process_path(tag_name);

    if (!tag_path_info_is_empty(tpi))
    {
        /* Lookup the base tag */
        TagPathElementInfo *tpei = tag_path_info_first_element(tpi);
        const char *root_tag_name = tag_path_element_info_name(tpei);
        Tag *t = retrieve_root_tag_by_name(db, root_tag_name);
        /* Lookup the (possible) child tag of the root or the root itself */
        t = tag_evaluate_path0(t, tpi);
        res = t;
    }

    tag_path_info_destroy(tpi);
    return res;
}

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id)
{
    file_remove_tag(f, tag_id);
    file_cabinet_remove(db->files, tag_id, f);
    if (file_is_untagged(f))
    {
        file_cabinet_insert(db->files, UNTAGGED, f);
    }
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
    file_cabinet_remove (db->files, UNTAGGED, f);
    file_cabinet_insert (db->files, tag_id, f);
}

void tagdb_save (TagDB *db, const char *db_fname)
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
    g_hash_table_destroy(db->tags);
    g_hash_table_destroy(db->tag_codes);
    g_free(db);
}

void _sqlite_newtag_stmt(TagDB *db, Tag *t)
{
    /* This function is idempotent with respect to the data */

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

void _sqlite_rename_file_stmt(TagDB *db, File *f, const char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENFIL);
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, file_id(f));
    sqlite3_step(stmt);
}

void _sqlite_rename_tag_stmt(TagDB *db, Tag *t, const char *new_name)
{
    sqlite3_stmt *stmt = STMT(db, RENTAG);
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, tag_id(t));
    sqlite3_step(stmt);
}

void _sqlite_subtag_ins_stmt(TagDB *db, Tag *super, Tag *sub)
{
    sqlite3_stmt *stmt = STMT(db, SUBTAG);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(super));
    sqlite3_bind_int(stmt, 2, tag_id(sub));
    sqlite3_step(stmt);
}

void _sqlite_subtag_rem_sub(TagDB *db, Tag *sub)
{
    sqlite3_stmt *stmt = STMT(db, REMSUB);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(sub));
    sqlite3_step(stmt);
}

void _sqlite_subtag_rem_sup(TagDB *db, Tag *sup)
{
    sqlite3_stmt *stmt = STMT(db, REMSUP);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(sup));
    sqlite3_step(stmt);
}

void _sqlite_subtag_del_stmt (TagDB *db, Tag *super, Tag *sub)
{
    sqlite3_stmt *stmt = STMT(db, REMSTA);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, tag_id(super));
    sqlite3_bind_int(stmt, 2, tag_id(sub));
    sqlite3_step(stmt);
}

TagDB *tagdb_new (const char *db_fname)
{
    return tagdb_new0(db_fname, 0);
}

void _tagdb_init_tags(TagDB *db);
TagDB *tagdb_new0 (const char *db_fname, int flags)
{
    TagDB *db = calloc(1,sizeof(struct TagDB));
    db->sqlite_db_fname = g_strdup(db_fname);

    if (flags & TAGDB_CLEAR)
    {
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
    sql_exec(db->sqldb, "pragma mmap_size=268435456");
    /* copied from xmms2 settings */
    sql_exec(db->sqldb, "PRAGMA synchronous = OFF");
    sql_exec(db->sqldb, "PRAGMA auto_vacuum = 1");
    sql_exec(db->sqldb, "PRAGMA cache_size = 8000");
    sql_exec(db->sqldb, "PRAGMA temp_store = MEMORY");
    sql_exec(db->sqldb, "PRAGMA foreign_keys = ON");

    /* One minute */
    sqlite3_busy_timeout (db->sqldb, 60000);

    /* insert into subtags */
    sql_prepare(db->sqldb, "insert into subtag(super, sub) values(?,?)", STMT(db, SUBTAG));
    /* remove from subtags by super */
    sql_prepare(db->sqldb, "delete from subtag where super=?", STMT(db, REMSUP));
    /* remove from subtags by sub */
    sql_prepare(db->sqldb, "delete from subtag where sub=?", STMT(db, REMSUB));
    /* remove from subtag */
    sql_prepare(db->sqldb, "delete from subtag where super=? and sub=?", STMT(db, REMSTA));
    /* new tag statement */
    sql_prepare(db->sqldb, "insert into tag(id,name) values(?,?)", STMT(db,NEWTAG));
    /* new file statement */
    sql_prepare(db->sqldb, "insert into file(id,name) values(?,?)", STMT(db,NEWFIL));
    /* delete tag statement */
    sql_prepare(db->sqldb, "delete from tag where id = ?", STMT(db,DELTAG));
    /* delete file statement */
    sql_prepare(db->sqldb, "delete from file where id = ?", STMT(db,DELFIL));
    /* rename file statement */
    sql_prepare(db->sqldb, "update or ignore file set name = ? where id = ?", STMT(db,RENFIL));
    /* rename tag statement */
    sql_prepare(db->sqldb, "update or ignore tag set name = ? where id = ?", STMT(db,RENTAG));

    /* lookup tag by id statement */
    sql_prepare(db->sqldb, "select name from tag where id = ?", STMT(db,STAGID));

    /* lookup tag by name statement */
    sql_prepare(db->sqldb, "select id from tag where name = ?", STMT(db,STAGNM));

    /* lookup file name by id statement */
    sql_prepare(db->sqldb, "select name from file where id = ?", STMT(db,SFILID));

    /* lookup file by name statement */
    sql_prepare(db->sqldb, "select id from file where name = ?", STMT(db,SFILNM));

    db->files = file_cabinet_new(db->sqldb);

    db->tags = tag_bucket_new();
    db->tag_codes = g_hash_table_new(g_str_hash, g_str_equal);
    db->file_max_id = 0;
    db->tag_max_id = 0;
    db->nfiles = 0;
    _tagdb_init_tags(db);
    db->nfiles = file_cabinet_size(db->files);
    db->file_max_id = file_cabinet_max_id(db->files);
    return db;
}

void _tagdb_init_tags(TagDB *db)
{
    /* Reads in the files from the sql database */
    sqlite3_stmt *stmt;

    /* SQL is bizarre, so we have to do this in two steps */
    sql_prepare(db->sqldb, "select * from subtag", stmt);
    sqlite3_reset(stmt);
    gboolean subtag_has_entries = (sql_next_row(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    if (subtag_has_entries)
    {
        sql_prepare(db->sqldb, "select distinct tag.id,tag.name from tag, subtag"
                " where tag.id not in (select sub from subtag)", stmt);
    }
    else
    {
        sql_prepare(db->sqldb, "select distinct tag.id,tag.name from tag", stmt);
    }
    sqlite3_reset(stmt);
    while (sql_next_row(stmt) == SQLITE_ROW)
    {
        file_id_t id = sqlite3_column_int64(stmt, 0);
        if (id > db->tag_max_id)
            db->tag_max_id = id;
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        Tag *t = new_tag((const char*)name, tagdb_int_t, 0);
        tag_id(t) = id;
        tag_bucket_insert(db, t);
        g_hash_table_insert(db->tag_codes, (gpointer)tag_name(t), TO_SP(id));
    }
    sqlite3_finalize(stmt);

    if (subtag_has_entries)
    {
        sql_prepare(db->sqldb, "select distinct super,sub,a.name,b.name from subtag,tag a, tag b"
                " where a.id=sub and b.id=super", stmt);
        sqlite3_reset(stmt);
        while (sql_next_row(stmt) == SQLITE_ROW)
        {
            file_id_t super = sqlite3_column_int64(stmt, 0);
            file_id_t sub = sqlite3_column_int64(stmt, 1);
            if (super > db->tag_max_id)
                db->tag_max_id = super;
            if (sub > db->tag_max_id)
                db->tag_max_id = sub;
            const unsigned char* name = sqlite3_column_text(stmt, 2);
            const unsigned char* parent_name = sqlite3_column_text(stmt, 3);
            Tag *t = new_tag((const char*)name, tagdb_int_t, 0);
            tag_id(t) = sub;
            tag_bucket_insert(db, t);

            Tag *parent = retrieve_tag(db, super);
            if (!parent)
            {
                parent = new_tag((const char*)parent_name, tagdb_int_t, 0);
                tag_id(parent) = super;
                tag_bucket_insert(db, parent);
            }
            tag_set_subtag(parent, t);
        }
        sqlite3_finalize(stmt);
    }
}

TagDB *tagdb_load (const char *db_fname)
{
    return tagdb_new(db_fname);
}
