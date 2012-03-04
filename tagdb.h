#include <glib.h>
#include "code_table.h"
#ifndef TAGDB_H
#define TAGDB_H
struct tagdb
{
    GHashTable *forward;
    GHashTable *reverse;
    CodeTable *file_codes;
    CodeTable *tag_codes;
    const gchar *db_fname;
};

typedef struct tagdb tagdb;

tagdb *newdb (const char *fname);

GList *tagdb_files (tagdb *db);
int tagdb_remove_file(tagdb *db, const char *fname);
int tagdb_insert_file(tagdb *db, const char *fname);
int tagdb_insert_tag (tagdb *db, const char *tag);
int tagdb_remove_tag (tagdb *db, const char *tag);

// Return all of the fields of item as a hash
GHashTable *tagdb_get_file_tags (tagdb *db, const char *item);
GHashTable *tagdb_get_tag_files (tagdb *db, const char *item);

// Return the field value if the field and item exists
// returns NULL if either the item doesn't exist or
// the field isn't associated with the item
gpointer tagdb_get (tagdb *db, const char *item);

// returns a list of names of items which satisfy predicate
GList *tagdb_filter (tagdb *db, 
        gboolean (*predicate)(gpointer key, gpointer value, gpointer data),
        gpointer data);
GList *tagdb_get_tag_list (tagdb *db);
GList *get_files_by_tags (tagdb *db, ...);
// NULL terminated array of tag strings
GList *get_files_by_tag_list (tagdb *db, GList *tags);
void tagdb_insert_file_with_tags (tagdb *db, const char *filename, GList *tags);

#endif /*TAGDB_H*/
