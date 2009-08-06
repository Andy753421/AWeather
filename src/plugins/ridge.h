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

#ifndef __RIDGE_H__
#define __RIDGE_H__

#include <glib-object.h>

#include <gis/gis.h>

#define GIS_TYPE_PLUGIN_RIDGE            (gis_plugin_ridge_get_type ())
#define GIS_PLUGIN_RIDGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_PLUGIN_RIDGE, GisPluginRidge))
#define GIS_IS_PLUGIN_RIDGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_PLUGIN_RIDGE))
#define GIS_PLUGIN_RIDGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_PLUGIN_RIDGE, GisPluginRidgeClass))
#define GIS_IS_PLUGIN_RIDGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_PLUGIN_RIDGE))
#define GIS_PLUGIN_RIDGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_PLUGIN_RIDGE, GisPluginRidgeClass))

typedef struct _GisPluginRidge      GisPluginRidge;
typedef struct _GisPluginRidgeClass GisPluginRidgeClass;

struct _GisPluginRidge {
	GObject parent_instance;

	/* instance members */
	GisWorld    *world;
	GisView     *view;
	GisOpenGL   *opengl;
};

struct _GisPluginRidgeClass {
	GObjectClass parent_class;
};

GType gis_plugin_ridge_get_type();

/* Methods */
GisPluginRidge *gis_plugin_ridge_new(GisWorld *world, GisView *view, GisOpenGL *opengl);

#endif
