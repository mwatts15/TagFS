#ifndef TYPES_H
#define TYPES_H
#include <glib.h>
#include <stdint.h>
#include "stream.h"
#include "util.h"

#define TAGDB_VALUE_INT_TYPE uint64_t
#define TAGDB_VALUE_INT_SIZE sizeof(TAGDB_VALUE_INT_TYPE)
#define N_TYPES 5
// enumeration of types in result_t
enum _tagdb_types
{
    tagdb_dict_t,
    tagdb_list_t,
    tagdb_int_t,
    tagdb_str_t,
    tagdb_bin_t,
    tagdb_err_t = -1,
} tagdb_types;

extern const char *type_strings[N_TYPES];

struct binstring_t
{
    size_t size;
    char *data;
};

typedef struct binstring_t binstring_t;

// Values returned by a query
// Also used in the database for tag values
union _value
{
    GHashTable *d;
    GList *l;
    TAGDB_VALUE_INT_TYPE i;
    char *s;
    binstring_t *b;
};

#define MAX_QUERY_ARGS 20
struct query_t
{
    int class_id;
    int command_id;
    int argc;
    gchar* argv[MAX_QUERY_ARGS];
};

struct result_t
{
    int type;
    union _value data;
};

typedef struct query_t query_t;
typedef struct result_t result_t;
typedef result_t tagdb_value_t;

typedef char *(*v_to_bs) (tagdb_value_t*, size_t*);

v_to_bs value_to_bstring_converters[N_TYPES];

tagdb_value_t *tagdb_str_to_value(int type, char *data);
char          *tagdb_value_to_str (tagdb_value_t *value);
binstring_t *tagdb_value_to_binstring (tagdb_value_t *value);
tagdb_value_t *tagdb_value_from_stream (ScannerStream *stream);

result_t *encapsulate (char *type, gpointer data);

tagdb_value_t *copy_value (tagdb_value_t *v);
tagdb_value_t *default_value (int type);

void query_destroy (query_t *q);
void result_destroy (result_t *r);
void binstring_destroy (binstring_t *bs);

int tagdb_value_cmp (tagdb_value_t *lhs, tagdb_value_t *rhs);
gboolean tagdb_value_equals (tagdb_value_t *lvalue, tagdb_value_t *rvalue);
guint tagdb_value_hash (tagdb_value_t *v);

int tagdb_value_get_type(tagdb_value_t *v);
void tagdb_value_set_type(tagdb_value_t *v, int type);

gboolean tagdb_value_is_error (tagdb_value_t *value);
const char *tagdb_value_strerror (tagdb_value_t *value);
GHashTable *tagdb_value_extract_dict (tagdb_value_t *v);
GList *tagdb_value_extract_list (tagdb_value_t *v);
TAGDB_VALUE_INT_TYPE tagdb_value_extract_int (tagdb_value_t *v);

void res_info (result_t *r, printer p);
GHashTable *tagdb_value_dict_new ();
tagdb_value_t *tagdb_value_dict_lookup_data (tagdb_value_t *dict, char *data);
#endif /* TYPES_H */
