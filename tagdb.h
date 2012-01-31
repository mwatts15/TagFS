#include <glib.h>
#ifndef TAGDB_H
#define TAGDB_H
struct tagdb
{
    GHashTable *dbstruct;
    GNode *tagstruct;
    const gchar *db_fname;
    const gchar *tag_list_fname;
};

typedef struct tagdb tagdb;

tagdb *newdb (const char *name, const char *tags_fname);
GHashTable *tagdb_toHash (tagdb *db);

GList *tagdb_files (tagdb *db);

// Return all of the fields of item as a hash
GHashTable tagdb_get_tags (tagdb *db, const char *item);

// Return the field value if the field and item exists
// returns NULL if either the item doesn't exist or
// the field isn't associated with the item
gpointer tagdb_get (tagdb *db, const char *item, const char *field);

// returns a list of names of items which satisfy predicate
GList *tagdb_filter (tagdb *db, 
        gboolean (*predicate)(gpointer key, gpointer value, gpointer data),
        gpointer data);
GList *get_tag_list (tagdb *db);
GList *get_files_by_tags (tagdb *db, ...);
#endif /*TAGDB_H*/
