#ifndef QUERY_FILE_H
#define QUERY_FILE_H
gboolean value_lt_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_eq_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_gt_sp (gpointer key, gpointer value, gpointer lvalue);

typedef GList* (*special_tag_fn) (TagDB*);
#endif
