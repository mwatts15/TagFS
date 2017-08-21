#include "plugin_manager.h"
#include "log.h"
#include "tag.h"
#include <stdint.h>
#include <string.h>

#define PLUGIN_NTYPES 1

struct TypeDat {
    const char *type_string;
    size_t size;
    uint8_t id;
    gpointer template;
};

GList *tag_list_populator_populate(TagListPopulator *tlp, GList *files)
{
    GList *res = NULL;
    if (files)
    {
        res = g_list_append(res, "plugin_tag_1");
        res = g_list_append(res, "plugin_tag_2");
    }

    GList *rres = NULL;
    LL (res, it)
    {
        const char *t = (const char*) it->data;
        rres = g_list_append(rres, PMCALL(tlp, get_tag, t));
    }
    g_list_free(res);
    return rres;
}

GList *tag_list_populator_filter (TagListPopulator *tlp, GList *own_tags, GList *files)
{
    if (own_tags)
    {
        debug("Got own tags");
        LL(own_tags, it)
        {
            debug("    %s", tag_name((Tag*)it->data));
        } LL_END
    }
    return g_list_copy(files);
}

gboolean tag_list_populator_owns (TagListPopulator *tlp, const char *base)
{
    if (g_strcmp0(base, "plugin_tag_1") == 0 || g_strcmp0(base, "plugin_tag_2") == 0)
    {
        return TRUE;
    }
    return FALSE;
}

PluginTag *tag_list_populator_get_tag (TagListPopulator *tlp, const char *tag_name)
{
    PluginTag *res;
    if (g_strcmp0(tag_name, "plugin_tag_1") == 0)
    {
        res = new_plugin_tag(tag_name, 0, NULL, plugin_name(tlp));
        tag_id(res) = 1;
    }
    else if (g_strcmp0(tag_name, "plugin_tag_2") == 0)
    {
        res = new_plugin_tag(tag_name, 0, NULL, plugin_name(tlp));
        tag_id(res) = 2;
    }
    else
    {
        res = NULL;
    }
    return res;
}

TagListPopulator tag_list_populator_template = {
    .populate = tag_list_populator_populate,
    .owns = tag_list_populator_owns,
    .filter = tag_list_populator_filter,
    .get_tag = tag_list_populator_get_tag
};

struct TypeDat types[PLUGIN_NTYPES] = {
    {
        .type_string = "TagListPopulator",
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
            pb->name = g_strdup(plugin_name);
            pb->type = types[i].id;
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
        warn("Failed to register plugin of unknown type: %s", plugin_type);
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
    return pm;
}

void plugin_manager_destroy(PluginManager *pm)
{
    if (pm)
    {
        g_hash_table_destroy(pm->plugins);
        g_free(pm);
    }
}

PluginTag *new_plugin_tag (const char *name, int type, const tagdb_value_t *default_value, const char *plugin_name)
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
