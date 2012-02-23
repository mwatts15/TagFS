#ifndef CODE_TABLE_H
#define CODE_TABLE_H
#include <glib.h>
#include <stdlib.h>
// associates numerical ids and strings
// numerical ids are stored as pointers using GINT_TO_POINTER
// in a forward and a reverse Hash Table
// The mappings are 1-to-1 (or should be) and you access by
// the rhs or lhs
struct CodeTable
{
    GHashTable *lhs;
    GHashTable *rhs;
    gulong size; // keeps the size (number of valid entries)
    gulong last_id; // for choosing new entries. only works when your sequence
                // of entries is well ordered and positive
};

typedef struct CodeTable CodeTable;

CodeTable *code_table_new();
int code_table_get_code (CodeTable *ct, char *value);
char *code_table_get_value (CodeTable *ct, int code);
void code_table_ins_entry (CodeTable *ct, int code, char *value);
void code_table_new_entry (CodeTable *ct, char *value);
void code_table_del_by_code (CodeTable *ct, int code);
void code_table_del_by_value (CodeTable *ct, char *value);

#endif /* CODE_TABLE_H */
