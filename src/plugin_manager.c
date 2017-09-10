#include "params.h"
#include "plugin_manager.h"
#include "log.h"
#include "tag.h"
#include <stdint.h>
#include <string.h>

#define PLUGIN_NTYPES 1

struct TypeDat {
    const char *type_string;
    const char *interface_name;
    size_t size;
    uint8_t id;
    gpointer template;
};

static void plugin_destroy(gpointer gp);

GList *tag_list_populator_populate(TagListPopulator *tlp, GList *files)
{
    GList *res = NULL;

    if (files)
    {
        GVariantBuilder gv_files_builder;
        g_variant_builder_init (&gv_files_builder, G_VARIANT_TYPE_ARRAY);
        int i = 0;
        LL(files, it)
        {
            file_id_t fid = file_id(it->data);
            char *realpath = g_strdup_printf("%s/%" TAGFS_FILE_ID_PRINTF_FORMAT, FSDATA->copiesdir, fid);
            g_variant_builder_add(&gv_files_builder, "(ss)",
                    file_name(it->data),
                    realpath);
            g_free(realpath);
            i++;
        } LL_END
        GVariant *gv_files_arr = g_variant_builder_end(&gv_files_builder);
        GDBusProxy *prox = plugin_get_remote_proxy(tlp);
        GVariant *tuple = g_dbus_proxy_call_sync(prox, "Populate",
                g_variant_new_tuple(&gv_files_arr, 1),
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                NULL,
                NULL);
        if (tuple)
        {
            GVariant *res_var = g_variant_get_child_value(tuple, 0);

            GVariantIter iter;
            // XXX: Can we overflow a buffer in this iterator with our gvariant?
            // TODO: Fuzz this via the plugin
            g_variant_iter_init(&iter, res_var);
            GVariant *child;
            while ((child = g_variant_iter_next_value (&iter)))
            {
                GVariant *child0 = g_variant_get_child_value(child, 0);
                GVariant *child1 = g_variant_get_child_value(child, 1);
                gsize namelen;
                const char *name = g_variant_get_string(child0, &namelen);
                uint64_t id = g_variant_get_uint64(child1);
                if (id && namelen) {
                    PluginTag *pt = plugin_tag_new(name, 0, NULL, plugin_name(tlp));
                    tag_id(pt) = id;
                    res = g_list_append(res, pt);
                }
                else
                {
                    warn("Plugin %s returned an invalid tag ID of 0", plugin_name(tlp));
                }
                g_variant_unref(child1);
                g_variant_unref(child0);
                g_variant_unref(child);
            }

            g_variant_unref(res_var);
            g_variant_unref(tuple);
        }
        else
        {
            warn("Plugin %s errored on call to populate", plugin_name(tlp));
            // TODO: Actual error handling
        }
    }
    else
    {
        warn("Received a request to populate without any files."
                " This is, most likely, a programmer error");
    }

    return res;
}

GList *tag_list_populator_filter (TagListPopulator *tlp, GList *own_tags, GList *files)
{
    GList *res = NULL;
    if (files && own_tags)
    {
        GHashTable *files_table = g_hash_table_new(g_direct_hash, g_direct_equal);

        GVariantBuilder gv_tags_builder;
        GVariantBuilder gv_files_builder;

        g_variant_builder_init (&gv_tags_builder, G_VARIANT_TYPE_ARRAY);
        g_variant_builder_init (&gv_files_builder, G_VARIANT_TYPE_ARRAY);

        int i = 0;
        LL(own_tags, it)
        {
            g_variant_builder_add(&gv_tags_builder,
                    "(st)",
                    tag_name(it->data),
                    tag_id(it->data));
            i++;
        } LL_END

        i = 0;
        LL(files, it)
        {
            file_id_t fid = file_id(it->data);
            g_hash_table_insert(files_table, TO_SP(fid), it->data);
            char *realpath = g_strdup_printf("%s/%" TAGFS_FILE_ID_PRINTF_FORMAT, FSDATA->copiesdir, fid);
            g_variant_builder_add(&gv_files_builder,
                    "(ss" TAGFS_FILE_ID_GVARIANT_TYPE_CODE ")",
                    file_name(it->data),
                    realpath,
                    fid);
            g_free(realpath);
            i++;
        } LL_END

        GDBusProxy *prox = plugin_get_remote_proxy(tlp);
        GVariant *tuple = g_dbus_proxy_call_sync(prox, "Filter",
                g_variant_new("(a(s" TAGFS_FILE_ID_GVARIANT_TYPE_CODE ")a(ss" TAGFS_FILE_ID_GVARIANT_TYPE_CODE "))",
                    &gv_tags_builder,
                    &gv_files_builder),
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                NULL,
                NULL);
        if (tuple)
        {
            GVariant *res_var = g_variant_get_child_value(tuple, 0);

            GVariantIter iter;
            // XXX: Can we overflow a buffer in this iterator with our gvariant?
            // TODO: Fuzz this via the plugin
            g_variant_iter_init(&iter, res_var);
            GVariant *child;
            while ((child = g_variant_iter_next_value (&iter)))
            {
                uint64_t id = g_variant_get_uint64(child);
                File *f = g_hash_table_lookup(files_table, TO_SP(id));
                if (f)
                {
                    res = g_list_append(res, f);
                }
                g_variant_unref (child);
            }

            g_variant_unref(res_var);
            g_variant_unref(tuple);
        }
        g_hash_table_destroy(files_table);

        if (own_tags)
        {
            LL(own_tags, it)
            {
                debug("    %s", tag_name(it->data));
            } LL_END
        }
    }
    else
    {
        warn("Received a request to filter when tags or files is NULL."
                " This is, most likely, a programmer error");
    }
    return res;
}

PluginTag *tag_list_populator_get_tag (TagListPopulator *tlp, const char *tag_name)
{
    uint64_t res_id = 0;
    GDBusProxy *prox = plugin_get_remote_proxy(tlp);
    GVariant *tuple = g_dbus_proxy_call_sync(prox, "GetTag",
            g_variant_new("(s)", tag_name),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            NULL);
    GVariant *res_var = g_variant_get_child_value(tuple, 0);
    res_id = g_variant_get_uint64(res_var);
    g_variant_unref(res_var);
    g_variant_unref(tuple);

    PluginTag *res = NULL;;
    if (res_id)
    {
        res = plugin_tag_new(tag_name, 0, NULL, plugin_name(tlp));
        tag_id(res) = res_id;
    }

    return res;
}

TagListPopulator tag_list_populator_template = {
    .populate = tag_list_populator_populate,
    .filter = tag_list_populator_filter,
    .get_tag = tag_list_populator_get_tag
};

struct TypeDat types[PLUGIN_NTYPES] = {
    {
        .type_string = "TagListPopulator",
        .interface_name = PM_DBUS_IFACE_PRE "TagListPopulator1",
        .size = sizeof(TagListPopulator),
        .id = 1,
        .template = &tag_list_populator_template
    }
};

GList *plugin_manager_get_plugins(PluginManager *pm, const char *plugin_type)
{
    return g_hash_table_lookup(pm->plugins, plugin_type);
}

PluginBase *_plugin_manager_get_plugin(PluginManager *pm, const char *plugin_type, const char *plugin_name)
{
    GList *plugins = g_hash_table_lookup(pm->plugins, plugin_type);
    LL (plugins, it)
    {
        if (g_strcmp0(plugin_name(it->data), plugin_name) == 0)
        {
            return (PluginBase*) it->data;
        }
    }
    warn("Plugin '%s' of type %s is no longer with us", plugin_type, plugin_name);
    return NULL;
}

void plugin_manager_register_plugin(PluginManager *pm, const char *plugin_type, const char *plugin_name)
{
    gpointer p = NULL;

    for (int i = 0; i < PLUGIN_NTYPES; i ++ )
    {
        if (g_strcmp0(types[i].type_string, plugin_type) == 0)
        {
            PluginBase *pb = g_malloc0(types[i].size);
            memmove(pb, types[i].template, types[i].size);
            char *object_path = g_strdelimit(g_strdup_printf("/%s", plugin_name), ".", '/');
            GCancellable *cancellable = g_cancellable_new();
            debug("Object path = %s", object_path);
            pb->name = g_strdup(plugin_name);
            pb->type = types[i].id;
            pb->remote_proxy = g_dbus_proxy_new_sync(pm->gdbus_conn,
                G_DBUS_PROXY_FLAGS_NONE,
                NULL, // TODO Get the interface info
                plugin_name,
                object_path,
                types[i].interface_name,
                cancellable, // TODO Handle cancels
                NULL); // TODO Handle fails
            if (!pb->remote_proxy)
            {
                debug("Couldn't set up the remote proxy");
                plugin_destroy(pb);
                pb = NULL;
            }
            g_object_unref(cancellable);
            g_free(object_path);
            p = pb;
        }
    }

    if (p != NULL)
    {
        GList *plugins = g_hash_table_lookup(pm->plugins, plugin_type);
        g_hash_table_steal(pm->plugins, plugin_type);
        g_hash_table_insert(pm->plugins, (char *) plugin_type, g_list_prepend(plugins, p));
    }
    else
    {
        warn("Failed to register plugin of type: %s", plugin_type);
    }
}

static void plugin_destroy(gpointer gp)
{
    if (gp)
    {
        PluginBase *pb = (PluginBase*) gp;
        g_free(pb->name);
        g_free(pb);
    }
}

static void plugins_list_destroy (gpointer gp)
{
    GList *x = (GList*) gp;
    g_list_free_full(x, plugin_destroy);
}

PluginManager *plugin_manager_new()
{
    PluginManager * pm = g_malloc0(sizeof(PluginManager));
    pm->plugins = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, plugins_list_destroy);
    pm->gdbus_conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    return pm;
}

void plugin_manager_destroy(PluginManager *pm)
{
    if (pm)
    {
        g_hash_table_destroy(pm->plugins);
        g_object_unref(pm->gdbus_conn);
        g_free(pm);
    }
}

PluginTag *plugin_tag_new (const char *name, int type, const tagdb_value_t *default_value, const char *plugin_name)
{
    PluginTag *pt = g_malloc0(sizeof(PluginTag));
    tag_init(&pt->base, name, type, default_value);
    abstract_file_set_type((AbstractFile*)&pt->base, abstract_file_plugin_tag_type);
    pt->plugin_name = g_strdup(plugin_name);
    return pt;
}

void plugin_tag_destroy (PluginTag *t)
{
    if (t)
    {
        tag_destroy1(&t->base);
        g_free(t->plugin_name);
        g_free(t);
    }
}
