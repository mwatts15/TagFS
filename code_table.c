#include "code_table.h"

CodeTable *code_table_new()
{
    CodeTable *res = (CodeTable*) malloc(sizeof(CodeTable));
    res->lhs = g_hash_table_new(g_direct_hash, g_direct_equal);
    res->rhs = g_hash_table_new(g_str_hash, g_str_equal);
    res->size = 0;
    res->last_id = 0; // the keeps the last id assigned
    return res;
}

int code_table_get_code (CodeTable *ct, const char *value)
{
    return GPOINTER_TO_INT(g_hash_table_lookup(ct->rhs, value));
}

char *code_table_get_value (CodeTable *ct, int code)
{
    // fail if code == 0
    // => null pointer
    if (code == 0)
    {
        NULL;
    }
    return g_hash_table_lookup(ct->lhs, GINT_TO_POINTER(code));
}

// creates or changes an entry
// returns the new id
int code_table_ins_entry (CodeTable *ct, int code, const char *value)
{
    if (code == 0)
        return 0;
    ct->last_id = code;
    ct->size++;
    g_hash_table_insert(ct->rhs, value, GINT_TO_POINTER(code));
    g_hash_table_insert(ct->lhs, GINT_TO_POINTER(code), value);
    return code;
}

// makes a new entry from ct->last_id
int code_table_new_entry (CodeTable *ct, const char *value)
{
    return code_table_ins_entry(ct, ct->last_id + 1, value);
}

void _code_table_delete (CodeTable *ct, int code, const char *value)
{
    gboolean a = g_hash_table_remove(ct->lhs, GINT_TO_POINTER(code));
    gboolean b = g_hash_table_remove(ct->rhs, value);
    if (!a && !b || b && a)// both succeeded or failed
    {
        ct->size--;
    }
    else
    {
        perror("CODE_TABLE_DELETE has failed to remove");
    }
}

void code_table_del_by_code (CodeTable *ct, int code)
{
    // fail if code == 0
    // => null pointer
    if (code == 0)
    {
        return;
    }
    char *value = g_hash_table_lookup(ct->lhs, GINT_TO_POINTER(code));
    _code_table_delete(ct, code, value);
}

void code_table_del_by_value (CodeTable *ct, const char *value)
{
    int code = GPOINTER_TO_INT(g_hash_table_lookup(ct->rhs, value));
    _code_table_delete(ct, code, value);
}
