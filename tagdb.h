#include <glib.h>
#include "code_table.h"
#ifndef TAGDB_H
#define TAGDB_H
struct tagdb
{
    GHashTable *forward;
    GHashTable *reverse;
    CodeTable *tag_codes;
    const gchar *db_fname;
};

typedef struct tagdb tagdb;

tagdb *newdb (const char *fname);

GList *tagdb_files(tagdb *db);
#endif /*TAGDB_H*/
