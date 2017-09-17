#include "params.h"
#include "plugin_manager.h"
#include "log.h"
#include "tag.h"
#include <stdint.h>
#include <string.h>

#define plugin_get_remote_proxy(__p) (((PluginBase*)(__p))->remote_proxy)
#define plugin_get_type_string(__p) (types[((PluginBase*)(__p))->type].type_string)
#define plugin_get_reconnect_policy(__p) (((PluginBase*)(__p))->reconnect_policy)
#define PLUGIN_NTYPES 1

struct TypeDat {
    const char *type_string;
    const char *interface_name;
    size_t size;
    uint8_t id;
    gpointer template;
};

const char *reconnect_policy_strings[] = {
    [PLUGIN_RECONNECT_ON_DEMAND] = "on-demand",
    [PLUGIN_RECONNECT_ON_NOTIFY] = "on-notify",
    [PLUGIN_RECONNECT_MANUAL] = "manual"
};

struct TypeDat types[PLUGIN_NTYPES];

static void plugin_destroy(gpointer gp);
static void _insert_plugin_by_type(PluginManager *pm, PluginBase *p);
static void _insert_plugin_by_name(PluginManager *pm, PluginBase *p);

void plugin_manager_reconnect_plugins (PluginManager *pm, const char *plugin_name, gboolean *success);
void plugin_manager_write_lock_table (PluginManager *pm);
void plugin_manager_read_lock_table (PluginManager *pm);
void plugin_manager_read_unlock_table (PluginManager *pm);
void plugin_manager_write_unlock_table (PluginManager *pm);
GDBusProxy *_new_plugin_proxy (GDBusConnection *conn,
        const char *interface_name,
        const char *plugin_name);
void _handle_gdbus_call_error(PluginBase *plugin, const char *method_name, gboolean *success, GError *err);
#define handle_gdbus_call_error(__p, __mname, __success, __err) \
    _handle_gdbus_call_error(((PluginBase*)__p), (__mname), (__success), (__err))

GList *tag_list_populator_populate(TagListPopulator *tlp, GList *files)
{
    GList *res = NULL;
    gboolean should_retry;
    do
    {
        should_retry = FALSE;
        if (files)
        {
            GVariantBuilder gv_files_builder;
            GError *err = NULL;
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
                    G_DBUS_CALL_FLAGS_NO_AUTO_START,
                    -1,
                    NULL,
                    &err);
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
                handle_gdbus_call_error(tlp, "Populate", &should_retry, err);
                g_clear_error(&err);
            }
        }
        else
        {
            warn("Received a request to populate without any files."
                    " This is, most likely, a programmer error");
        }
    } while (should_retry);
    return res;
}

GList *tag_list_populator_filter (TagListPopulator *tlp, GList *own_tags, GList *files)
{
    GList *res = NULL;
    gboolean should_retry;
    do
    {
        should_retry = FALSE;
        if (files && own_tags)
        {
            GError *err = NULL;
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
                    G_DBUS_CALL_FLAGS_NO_AUTO_START,
                    -1,
                    NULL,
                    &err);
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
            else
            {
                handle_gdbus_call_error(tlp, "Filter", &should_retry, err);
                g_clear_error(&err);
            }
            g_hash_table_destroy(files_table);
        }
        else
        {
            warn("Received a request to filter when tags or files is NULL."
                    " This is, most likely, a programmer error");
        }
    } while (should_retry);
    return res;
}

PluginTag *tag_list_populator_get_tag (TagListPopulator *tlp, const char *tag_name)
{
    PluginTag *res = NULL;
    gboolean should_retry;
    do
    {
        should_retry = FALSE;
        GError *err = NULL;
        GDBusProxy *prox = plugin_get_remote_proxy(tlp);
        GVariant *tuple = g_dbus_proxy_call_sync(prox, "GetTag",
                g_variant_new("(s)", tag_name),
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                -1,
                NULL,
                &err);
        if (tuple)
        {
            GVariant *res_var = g_variant_get_child_value(tuple, 0);
            if (res_var)
            {
                uint64_t res_id = *(uint64_t*)g_variant_get_data(res_var);
                if (res_id)
                {
                    res = plugin_tag_new(tag_name, 0, NULL, plugin_name(tlp));
                    tag_id(res) = res_id;
                }
                g_variant_unref(res_var);
            }
            g_variant_unref(tuple);
        }
        else
        {
            handle_gdbus_call_error(tlp, "GetTag", &should_retry, err);
            g_clear_error(&err);
        }
    } while (should_retry);

    return res;
}

void _handle_gdbus_call_error(PluginBase *plugin, const char *method_name, gboolean *should_retry, GError *err)
{
    const char *plugin_type_name = plugin_get_type_string(plugin);
    const char *pname = plugin_name(plugin);
    *should_retry = FALSE;

    gboolean remote = g_dbus_error_is_remote_error(err);
    if (remote)
    {
        /* Logging this as info since it's not *our* error. Assuming the
         * remote can report its own errors
         */
        char *e = g_dbus_error_get_remote_error(err);
        info("'%s' call to %s plugin '%s' errored: %s", method_name,
                plugin_type_name, pname, e);
        g_free(e);

        if (err->domain == G_DBUS_ERROR && err->code == G_DBUS_ERROR_NAME_HAS_NO_OWNER)
        {
            warn("Couldn't get a result from %s plugin '%s' due to disconnect: %s",
                    plugin_type_name, pname, err->message);
            PluginReconnectPolicy policy = plugin_get_reconnect_policy(plugin);
            debug("Reconnect policy is '%s'", reconnect_policy_strings[policy]);

            if (policy == PLUGIN_RECONNECT_ON_DEMAND)
            {
                info("Attempting to reconnect to %s plugin '%s'",
                        plugin_type_name, pname);
                plugin_manager_reconnect_plugins(plugin_get_manager(plugin), pname, should_retry);
                info("Reconnect %s", (*should_retry)?"succeeded":"failed");
            }
            else if (policy == PLUGIN_RECONNECT_ON_NOTIFY)
            {
                /* TODO: Wait for for the reconnect to be completed (semaphore)
                 * or to timeout
                 */
            }
        }
        else
        {
            warn("Couldn't get a result from TagListPopulator plugin '%s'. Cause: %s %s", pname, g_quark_to_string(err->domain), err->message);
        }
    }
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
        .id = 0,
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

void plugin_manager_reconnect_plugins (PluginManager *pm, const char *plugin_name, gboolean *success)
{
    *success = TRUE;
    plugin_manager_read_lock_table(pm);
    GList *plugins = g_hash_table_lookup(pm->plugins_by_name, plugin_name);
    if (plugins)
    {
        LL (plugins, it)
        {
            PluginBase *pb = it->data;
            GDBusProxy *prox = pb->remote_proxy;
            pb->remote_proxy = _new_plugin_proxy(pm->gdbus_conn,
                    g_dbus_proxy_get_interface_name(prox), plugin_name);
            g_object_unref(prox);
            prox = NULL;

            if (!pb->remote_proxy)
            {
                *success = FALSE;
                break;
            }
        } LL_END
    }
    else
    {
        *success = FALSE;
    }
    plugin_manager_read_unlock_table(pm);

    if (plugins && *success == FALSE)
    {
        plugin_manager_write_lock_table(pm);
        /* Couldn't connect, so we remove from the list */
        GList *l = NULL;

        if (g_hash_table_lookup_extended(pm->plugins_by_name, plugin_name,
                NULL, (gpointer*)&l))
        {
            plugins = l;
            LL (plugins, it)
            {
                const char *plugin_type_string = plugin_get_type_string(it->data);
                char *orig_key = NULL;
                l = NULL;

                if (g_hash_table_lookup_extended(pm->plugins, plugin_type_string,
                            (gpointer*)&orig_key, (gpointer*)&l))
                {
                    g_hash_table_steal(pm->plugins, plugin_type_string);
                    GList *leavings = g_list_remove(l, it->data);
                    if (leavings)
                    {
                        g_hash_table_insert(pm->plugins, orig_key, leavings);
                    }
                    else
                    {
                        g_free(orig_key);
                    }
                }
                plugin_destroy(it->data);
            } LL_END
            g_hash_table_remove(pm->plugins_by_name, plugin_name);
        }
        else
        {
            info("Couldn't find any plugins named '%s'."
                    " Assuming we already removed them and exiting", plugin_name);
        }
        plugin_manager_write_unlock_table(pm);
    }
}

void plugin_manager_write_lock_table (PluginManager *pm)
{
    g_rw_lock_writer_lock(&pm->table_lock);
}

void plugin_manager_read_lock_table (PluginManager *pm)
{
    g_rw_lock_reader_lock(&pm->table_lock);
}

void plugin_manager_read_unlock_table (PluginManager *pm)
{
    g_rw_lock_reader_unlock(&pm->table_lock);
}

void plugin_manager_write_unlock_table (PluginManager *pm)
{
    g_rw_lock_writer_unlock(&pm->table_lock);
}

void plugin_manager_register_plugin(PluginManager *pm, const char *plugin_type, const char *plugin_name)
{
    plugin_manager_register_plugin0(pm, plugin_type, plugin_name, FALSE);
}

GDBusProxy *_new_plugin_proxy (GDBusConnection *conn,
        const char *interface_name,
        const char *plugin_name)
{
    char *object_path = g_strdelimit(g_strconcat("/", plugin_name, NULL), ".", '/');
    GCancellable *cancellable = g_cancellable_new();
    GError *err = NULL;

    GDBusProxy *res = g_dbus_proxy_new_sync(conn,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL, // TODO Get the interface info
            plugin_name,
            object_path,
            interface_name,
            cancellable, // TODO Handle cancels
            &err);
    if (!res)
    {
        warn("Couldn't set up the remote proxy: %s", err->message);
    }
    else
    {
        char *name = g_dbus_proxy_get_name_owner(res);
        if (!name)
        {
            warn("No owner found for the plugin '%s'", plugin_name);
            g_object_unref(res);
            res = NULL;
        }
        g_free(name);
    }
    g_object_unref(cancellable);
    g_free(object_path);
    return res;
}

void plugin_manager_register_plugin0(PluginManager *pm,
        const char *plugin_type,
        const char *plugin_name,
        PluginReconnectPolicy reconnect_policy)
{
    PluginBase *p = NULL;
    GList *existing_plugins = NULL;
    GDBusProxy *prox = NULL;
    struct TypeDat *plugin_type_dat = NULL;
    if (g_hash_table_lookup_extended(pm->plugins_by_name,
                plugin_name, NULL, (gpointer*)&existing_plugins))
    {
        LL (existing_plugins, it)
        {
            PluginBase *pb = it->data;
            if (g_strcmp0(types[pb->type].type_string, plugin_type) == 0)
            {
                warn("Received a request to register a plugin that is already registered ('%s', '%s')",
                        plugin_name, plugin_type);
                return;
            }
        } LL_END;
    }

    for (int i = 0; i < PLUGIN_NTYPES; i++ )
    {
        if (g_strcmp0(types[i].type_string, plugin_type) == 0)
        {
            plugin_type_dat = &(types[i]);
            prox = _new_plugin_proxy(pm->gdbus_conn, types[i].interface_name, plugin_name);
            break;
        }
    }

    if (prox)
    {
        PluginBase *pb = g_malloc0(plugin_type_dat->size);
        memmove(pb, plugin_type_dat->template, plugin_type_dat->size);
        pb->name = g_strdup(plugin_name);
        pb->type = plugin_type_dat->id;
        pb->remote_proxy = prox;
        pb->reconnect_policy = reconnect_policy;
        pb->manager = pm;
        p = pb;
    }

    if (p != NULL)
    {
        _insert_plugin_by_type(pm, p);
        _insert_plugin_by_name(pm, p);
    }
    else
    {
        warn("Failed to register plugin of type: %s", plugin_type);
    }
}

static void _insert_plugin_by_type(PluginManager *pm, PluginBase *p)
{
    const char *plugin_type = types[p->type].type_string;
    GList *plugins = g_hash_table_lookup(pm->plugins, plugin_type);

    /* 'Steal' the value so we don't delete our plugins list with the
     * default hash table value destroy function
     */
    g_hash_table_steal(pm->plugins, plugin_type);

    /* This cast is OK because we aren't freeing the keys of this hash.
     * Also, we know that their storage isn't going to change
     */
    g_hash_table_insert(pm->plugins, (char *) plugin_type, g_list_prepend(plugins, p));
}

static void _insert_plugin_by_name(PluginManager *pm, PluginBase *p)
{
    char *plugin_name = g_strdup(p->name);
    GList *plugins_by_name = g_hash_table_lookup(pm->plugins, plugin_name);

    /* 'Steal' the value so we don't delete our plugins list with the
     * default hash table value destroy function
     */
    g_hash_table_steal(pm->plugins_by_name, plugin_name);

    /* We could recycle the key if it was put in here before, but that's
     * extra unnecessary work.
     */
    g_hash_table_insert(pm->plugins_by_name, plugin_name, g_list_prepend(plugins_by_name, p));

}

static void plugin_destroy(gpointer gp)
{
    if (gp)
    {
        PluginBase *pb = (PluginBase*) gp;
        if (pb->remote_proxy)
        {
            g_object_unref(pb->remote_proxy);
        }
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
    pm->plugins_by_name = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_list_free);
    pm->gdbus_conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    g_rw_lock_init(&pm->table_lock);
    return pm;
}

void plugin_manager_destroy (PluginManager *pm)
{
    if (pm)
    {
        g_hash_table_destroy(pm->plugins);
        g_hash_table_destroy(pm->plugins_by_name);
        g_object_unref(pm->gdbus_conn);
        g_rw_lock_clear(&pm->table_lock);
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
