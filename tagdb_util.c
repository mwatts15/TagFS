#include "log.h"
#include "tagdb.h"
#include "set_ops.h"
#include "tagdb.h"
#include "tagdb_util.h"

GList *get_tags_list (TagDB *db, tagdb_key_t key)
{
    log_msg("\nt db=%p key=%p\n", db, key);;

    GList *tags = NULL;
    int skip = 1;
    if (key_is_empty(key))
    {
        tags = tagdb_tag_names(db);
    }
    else
    {
        KL(key, i)
        {
            FileDrawer *d = file_cabinet_get_drawer(db->files, key_ref(key, i));
            if (d)
            {
                GList *this = file_drawer_get_tags(d);
                this = g_list_sort(this, (GCompareFunc) long_cmp);

                GList *tmp = NULL;
                if (skip)
                {
                    tmp = g_list_copy(this);
                }
                else
                {
                    tmp = g_list_intersection(tags, this, (GCompareFunc) long_cmp);
                }

                g_list_free(this);
                g_list_free(tags);

                tags = tmp;
            }
            skip = 0;
        } KL_END;
    }

    #if 1
    return tags;
    #else
    GList *res = NULL;
    LL(tags, list)
    {
        int skip = 0;
        KL(key, i)
        {
            if (TO_S(list->data) == key[i])
            {
                skip = 1;
            }
        } KL_END;
        if (!skip)
        {
            Tag *t = retrieve_tag(db, TO_S(list->data));
            if (t != NULL)
                res = g_list_prepend(res, t);
        }
    } LL_END;
    g_list_free(tags);
    return res;
    #endif
}

/* Gets all of the files with the given tags
   as well as all of the tags below this one
   in the tree */
GList *get_files_list (TagDB *db, tagdb_key_t key)
{
    GList *res = NULL;
    int skip = 1;

    if (key_is_empty(key))
    {
        res = file_cabinet_get_drawer_l(db->files, UNTAGGED);
    }
    else
    {
        KL(key, i)
        {
            GList *files = NULL;
            files = file_cabinet_get_drawer_l(db->files, key_ref(key, i));
            files = g_list_sort(files, (GCompareFunc) file_id_cmp);

            GList *tmp;
            if (skip)
                tmp = g_list_copy(files);
            else
                tmp = g_list_intersection(res, files, (GCompareFunc) file_id_cmp);

            g_list_free(res);
            g_list_free(files);
            res = tmp;
            skip = 0;
        } KL_END;
    }

    return res;
}
