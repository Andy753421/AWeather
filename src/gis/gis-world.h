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

#ifndef __GIS_WORLD_H__
#define __GIS_WORLD_H__

#include <glib-object.h>

/* Type macros */
#define GIS_TYPE_WORLD            (gis_world_get_type())
#define GIS_WORLD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_WORLD, GisWorld))
#define GIS_IS_WORLD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_WORLD))
#define GIS_WORLD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_WORLD, GisWorldClass))
#define GIS_IS_WORLD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_WORLD))
#define GIS_WORLD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_WORLD, GisWorldClass))

typedef struct _GisWorld      GisWorld;
typedef struct _GisWorldClass GisWorldClass;

struct _GisWorld {
	GObject parent_instance;

	/* instance members */
	gboolean offline;
};

struct _GisWorldClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType gis_world_get_type(void);

/* Methods */
GisWorld *gis_world_new();

void gis_world_refresh(GisWorld *world);

void gis_world_set_offline(GisWorld *world, gboolean offline);
gboolean gis_world_get_offline(GisWorld *world);

#endif
