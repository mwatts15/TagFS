#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <glib.h>
#include <stdint.h>
#include "tag.h"

typedef struct TagListPopulator TagListPopulator;
typedef struct PluginBase PluginBase;
typedef struct PluginManager PluginManager;
typedef struct PluginTag PluginTag;

struct PluginManager {
    GHashTable *plugins;
};

struct PluginBase {
    uint8_t type;
    char *name;
};

struct TagListPopulator {
    PluginBase base;
    GList *(*populate) (TagListPopulator*, GList *files);
    gboolean (*owns) (TagListPopulator*, const char *base);
    GList *(*filter) (TagListPopulator*, GList *own_tags, GList *files);
    PluginTag *(*get_tag) (TagListPopulator*, const char *tag_name);
};

struct PluginTag {
    Tag base;
    char *plugin_name;
};

PluginManager *plugin_manager_new();
void plugin_manager_destroy(PluginManager *pm);
GList *plugin_manager_get_plugins(PluginManager *plugin_manager, const char *plugin_type);
PluginBase *_plugin_manager_get_plugin(PluginManager *pm, const char *plugin_type, const char *pname);
void plugin_manager_register_plugin(PluginManager *plugin_manager, const char *plugin_type, const char *plugin_name);
PluginTag *new_plugin_tag (const char *name, int type, const tagdb_value_t *default_value, const char *plugin_name);
void plugin_tag_destroy (PluginTag *t);

#define plugin_manager_get_plugin(__plugin_manager, __plugin_type, __plugin_name) ((__plugin_type *)_plugin_manager_get_plugin((__plugin_manager),\
        #__plugin_type, (__plugin_name)))
#define PMCALL(__object, __method, ...) ((__object)->__method((__object), __VA_ARGS__))
#define plugin_name(__p) (((PluginBase*)(__p))->name)
#define tag_is_plugin_tag(__t) (abstract_file_get_type((__t))==abstract_file_plugin_tag_type)
#define plugin_tag_plugin_name(__t) (((PluginTag*)(__t))->plugin_name)

#endif
