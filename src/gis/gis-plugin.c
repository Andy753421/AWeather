/*
 * Copyright (C) 2009 Andy Spencer <spenceal@rose-hulman.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <gmodule.h>

#include "gis-plugin.h"

/********************
 * Plugin interface *
 ********************/
static void gis_plugin_base_init(gpointer g_class)
{
	static gboolean is_initialized = FALSE;
	if (!is_initialized) {
		/* add properties and signals to the interface here */
		is_initialized = TRUE;
	}
}

GType gis_plugin_get_type()
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof(GisPluginInterface),
			gis_plugin_base_init,
			NULL,
		};
		type = g_type_register_static(G_TYPE_INTERFACE,
				"GisPlugin", &info, 0);
	}
	return type;
}

void gis_plugin_expose(GisPlugin *self)
{
	g_return_if_fail(GIS_IS_PLUGIN(self));
	GIS_PLUGIN_GET_INTERFACE(self)->expose(self);
}

GtkWidget *gis_plugin_get_config(GisPlugin *self)
{
	g_return_val_if_fail(GIS_IS_PLUGIN(self), NULL);
	return GIS_PLUGIN_GET_INTERFACE(self)->get_config(self);
}


/***************
 * Plugins API *
 ***************/
typedef struct {
	gchar *name;
	GisPlugin *plugin;
} GisPluginStore;

GisPlugins *gis_plugins_new()
{
	return g_ptr_array_new();
}

void gis_plugins_free(GisPlugins *self)
{
	for (int i = 0; i < self->len; i++) {
		GisPluginStore *store = g_ptr_array_index(self, i);
		g_object_unref(store->plugin);
		g_free(store->name);
		g_free(store);
		g_ptr_array_remove_index(self, i);
	}
	g_ptr_array_free(self, TRUE);
}

GList *gis_plugins_available()
{
	GDir *dir = g_dir_open(PLUGINDIR, 0, NULL);
	if (dir == NULL)
		return NULL;
	GList *list = NULL;
	const gchar *name;
	while ((name = g_dir_read_name(dir))) {
		if (g_pattern_match_simple("*.so", name)) {
			gchar **parts = g_strsplit(name, ".", 2);
			list = g_list_prepend(list, g_strdup(parts[0]));
			g_strfreev(parts);
		}
	}
	return list;
}

GisPlugin *gis_plugins_load(GisPlugins *self, const char *name,
		GisWorld *world, GisView *view, GisOpenGL *opengl, GisPrefs *prefs)
{
	gchar *path = g_strdup_printf("%s/%s.%s", PLUGINDIR, name, G_MODULE_SUFFIX);
	GModule *module = g_module_open(path, 0);
	g_free(path);
	if (module == NULL) {
		g_warning("Unable to load module %s: %s", name, g_module_error());
		return NULL;
	}

	gpointer constructor_ptr; // GCC 4.1 fix?
	gchar *constructor_str = g_strconcat("gis_plugin_", name, "_new", NULL);
	if (!g_module_symbol(module, constructor_str, &constructor_ptr)) {
		g_warning("Unable to load symbol %s from %s: %s",
				constructor_str, name, g_module_error());
		g_module_close(module);
		g_free(constructor_str);
		return NULL;
	}
	g_free(constructor_str);
	GisPluginConstructor constructor = constructor_ptr;

	GisPluginStore *store = g_malloc(sizeof(GisPluginStore));
	store->name = g_strdup(name);
	store->plugin = constructor(world, view, opengl, prefs);
	g_ptr_array_add(self, store);
	return store->plugin;
}

gboolean gis_plugins_unload(GisPlugins *self, const char *name)
{
	for (int i = 0; i < self->len; i++) {
		GisPluginStore *store = g_ptr_array_index(self, i);
		if (g_str_equal(store->name, name)) {
			g_object_unref(store->plugin);
			g_free(store->name);
			g_free(store);
			g_ptr_array_remove_index(self, i);
		}
	}
	return FALSE;
}
void gis_plugins_foreach(GisPlugins *self, GCallback _callback, gpointer user_data)
{
	typedef void (*CBFunc)(GisPlugin *, const gchar *, gpointer);
	CBFunc callback = (CBFunc)_callback;
	for (int i = 0; i < self->len; i++) {
		GisPluginStore *store = g_ptr_array_index(self, i);
		callback(store->plugin, store->name, user_data);
	}
}
