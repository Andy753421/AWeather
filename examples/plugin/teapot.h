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

#ifndef __TEAPOT_H__
#define __TEAPOT_H__

#include <glib-object.h>

#define GIS_TYPE_PLUGIN_TEAPOT            (gis_plugin_teapot_get_type ())
#define GIS_PLUGIN_TEAPOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_PLUGIN_TEAPOT, GisPluginTeapot))
#define GIS_IS_PLUGIN_TEAPOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_PLUGIN_TEAPOT))
#define GIS_PLUGIN_TEAPOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_PLUGIN_TEAPOT, GisPluginTeapotClass))
#define GIS_IS_PLUGIN_TEAPOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_PLUGIN_TEAPOT))
#define GIS_PLUGIN_TEAPOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_PLUGIN_TEAPOT, GisPluginTeapotClass))

typedef struct _GisPluginTeapot      GisPluginTeapot;
typedef struct _GisPluginTeapotClass GisPluginTeapotClass;

struct _GisPluginTeapot {
	GObject parent_instance;

	/* instance members */
	GtkToggleButton *button;
	guint            rotate_id;
	float            rotation;
	GisOpenGL       *opengl;
};

struct _GisPluginTeapotClass {
	GObjectClass parent_class;
};

GType gis_plugin_teapot_get_type();

/* Methods */
GisPluginTeapot *gis_plugin_teapot_new(GisWorld *world, GisView *view, GisOpenGL *opengl);

#endif
