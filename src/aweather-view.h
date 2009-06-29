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

#ifndef __AWEATHER_VIEW_H__
#define __AWEATHER_VIEW_H__

#include <glib-object.h>

/* Type macros */
#define AWEATHER_TYPE_VIEW            (aweather_view_get_type())
#define AWEATHER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_VIEW, AWeatherView))
#define AWEATHER_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_VIEW))
#define AWEATHER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_VIEW, AWeatherViewClass))
#define AWEATHER_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_VIEW))
#define AWEATHER_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_VIEW, AWeatherViewClass))

typedef struct _AWeatherView      AWeatherView;
typedef struct _AWeatherViewClass AWeatherViewClass;

struct _AWeatherView {
	GObject parent_instance;

	/* instance members */
	gchar   *time;
	gchar   *site;
	gdouble  location[3];
	gboolean offline;
};

struct _AWeatherViewClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType aweather_view_get_type(void);

/* Methods */
AWeatherView *aweather_view_new();

void aweather_view_set_time(AWeatherView *view, const gchar *time);
gchar *aweather_view_get_time(AWeatherView *view);

void aweather_view_set_location(AWeatherView *view, gdouble  x, gdouble  y, gdouble  z);
void aweather_view_get_location(AWeatherView *view, gdouble *x, gdouble *y, gdouble *z);
void aweather_view_pan         (AWeatherView *view, gdouble  x, gdouble  y, gdouble  z);
void aweather_view_zoom        (AWeatherView *view, gdouble  scale);

void aweather_view_refresh(AWeatherView *view);

void aweather_view_set_offline(AWeatherView *view, gboolean offline);
gboolean aweather_view_get_offline(AWeatherView *view);

/* To be deprecated, use {get,set}_location */
void aweather_view_set_site(AWeatherView *view, const gchar *site);
gchar *aweather_view_get_site(AWeatherView *view);

#endif
