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

#ifndef __GIS_PLUGIN_H__
#define __GIS_PLUGIN_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define GIS_TYPE_PLUGIN                (gis_plugin_get_type())
#define GIS_PLUGIN(obj)                (G_TYPE_CHECK_INSTANCE_CAST   ((obj),  GIS_TYPE_PLUGIN, GisPlugin))
#define GIS_IS_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE   ((obj),  GIS_TYPE_PLUGIN))
#define GIS_PLUGIN_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), GIS_TYPE_PLUGIN, GisPluginInterface))

typedef struct _GisPlugin          GisPlugin;
typedef struct _GisPluginInterface GisPluginInterface;
typedef GPtrArray                  GisPlugins;

struct _GisPluginInterface
{
	GTypeInterface parent_iface;

	/* Virtual functions */
	void       (*expose    )(GisPlugin *self);
	GtkWidget *(*get_config)(GisPlugin *self);
};

GType gis_plugin_get_type();

/* Methods */
void       gis_plugin_expose(GisPlugin *self);
GtkWidget *gis_plugin_get_config(GisPlugin *self);

/* Plugins API */
#include "gis-world.h"
#include "gis-view.h"
#include "gis-opengl.h"
#include "gis-prefs.h"

typedef GisPlugin *(*GisPluginConstructor)(GisWorld *world, GisView *view, GisOpenGL *opengl, GisPrefs *prefs);

GisPlugins *gis_plugins_new();
void        gis_plugins_free();
GList      *gis_plugins_available();
GisPlugin  *gis_plugins_load(GisPlugins *self, const char *name,
		GisWorld *world, GisView *view, GisOpenGL *opengl, GisPrefs *prefs);
gboolean    gis_plugins_unload(GisPlugins *self, const char *name);
void        gis_plugins_foreach(GisPlugins *self, GCallback callback, gpointer user_data);

#endif
