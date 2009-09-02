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

#ifndef __GIS_VIEW_H__
#define __GIS_VIEW_H__

#include <glib-object.h>

/* Type macros */
#define GIS_TYPE_VIEW            (gis_view_get_type())
#define GIS_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_VIEW, GisView))
#define GIS_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_VIEW))
#define GIS_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_VIEW, GisViewClass))
#define GIS_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_VIEW))
#define GIS_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_VIEW, GisViewClass))

typedef struct _GisView      GisView;
typedef struct _GisViewClass GisViewClass;

struct _GisView {
	GObject parent_instance;

	/* instance members */
	gchar   *time;
	gchar   *site;
	gdouble  location[3];
	gdouble  rotation[3];
};

struct _GisViewClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType gis_view_get_type(void);

/* Methods */
GisView *gis_view_new();

void gis_view_set_time(GisView *view, const gchar *time);
gchar *gis_view_get_time(GisView *view);

void gis_view_set_location(GisView *view, gdouble  lat, gdouble  lon, gdouble  elev);
void gis_view_get_location(GisView *view, gdouble *lat, gdouble *lon, gdouble *elev);
void gis_view_pan         (GisView *view, gdouble  lat, gdouble  lon, gdouble  elev);
void gis_view_zoom        (GisView *view, gdouble  scale);

void gis_view_set_rotation(GisView *view, gdouble  x, gdouble  y, gdouble  z);
void gis_view_get_rotation(GisView *view, gdouble *x, gdouble *y, gdouble *z);
void gis_view_rotate      (GisView *view, gdouble  x, gdouble  y, gdouble  z);

/* To be deprecated, use {get,set}_location */
void gis_view_set_site(GisView *view, const gchar *site);
gchar *gis_view_get_site(GisView *view);

#endif
