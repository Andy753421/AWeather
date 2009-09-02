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

#include "gis-marshal.h"
#include "gis-view.h"
#include "gis-world.h"

/* Constants */
enum {
	PROP_0,
	PROP_TIME,
	PROP_SITE,
};
enum {
	SIG_TIME_CHANGED,
	SIG_SITE_CHANGED,
	SIG_LOCATION_CHANGED,
	SIG_ROTATION_CHANGED,
	NUM_SIGNALS,
};
static guint signals[NUM_SIGNALS];


/* Signal helpers */
static void _gis_view_emit_location_changed(GisView *view)
{
	g_signal_emit(view, signals[SIG_LOCATION_CHANGED], 0, 
			view->location[0],
			view->location[1],
			view->location[2]);
}
static void _gis_view_emit_rotation_changed(GisView *view)
{
	g_signal_emit(view, signals[SIG_ROTATION_CHANGED], 0, 
			view->rotation[0],
			view->rotation[1],
			view->rotation[2]);
}
static void _gis_view_emit_time_changed(GisView *view)
{
	g_signal_emit(view, signals[SIG_TIME_CHANGED], 0,
			view->time);
}
static void _gis_view_emit_site_changed(GisView *view)
{
	g_signal_emit(view, signals[SIG_SITE_CHANGED], 0,
			view->site);
}


/***********
 * Methods *
 ***********/
GisView *gis_view_new()
{
	g_debug("GisView: new");
	return g_object_new(GIS_TYPE_VIEW, NULL);
}

void gis_view_set_time(GisView *view, const char *time)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: set_time - time=%s", time);
	g_free(view->time);
	view->time = g_strdup(time);
	_gis_view_emit_time_changed(view);
}

gchar *gis_view_get_time(GisView *view)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: get_time");
	return view->time;
}

void gis_view_set_location(GisView *view, gdouble lat, gdouble lon, gdouble elev)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: set_location");
	view->location[0] = lat;
	view->location[1] = lon;
	view->location[2] = elev;
	_gis_view_emit_location_changed(view);
}

void gis_view_get_location(GisView *view, gdouble *lat, gdouble *lon, gdouble *elev)
{
	g_assert(GIS_IS_VIEW(view));
	//g_debug("GisView: get_location");
	*lat  = view->location[0];
	*lon  = view->location[1];
	*elev = view->location[2];
}

void gis_view_pan(GisView *view, gdouble lat, gdouble lon, gdouble elev)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: pan - lat=%8.3f, lon=%8.3f, elev=%8.3f", lat, lon, elev);
	view->location[0] += lat;
	view->location[1] += lon;
	view->location[2] += elev;
	_gis_view_emit_location_changed(view);
}

void gis_view_zoom(GisView *view, gdouble scale)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: zoom");
	view->location[2] *= scale;
	_gis_view_emit_location_changed(view);
}

void gis_view_set_rotation(GisView *view, gdouble x, gdouble y, gdouble z)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: set_rotation");
	view->rotation[0] = x;
	view->rotation[1] = y;
	view->rotation[2] = z;
	_gis_view_emit_rotation_changed(view);
}

void gis_view_get_rotation(GisView *view, gdouble *x, gdouble *y, gdouble *z)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: get_rotation");
	*x = view->rotation[0];
	*y = view->rotation[1];
	*z = view->rotation[2];
}

void gis_view_rotate(GisView *view, gdouble x, gdouble y, gdouble z)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: rotate - x=%.0f, y=%.0f, z=%.0f", x, y, z);
	view->rotation[0] += x;
	view->rotation[1] += y;
	view->rotation[2] += z;
	_gis_view_emit_rotation_changed(view);
}

/* To be deprecated, use {get,set}_location */
void gis_view_set_site(GisView *view, const gchar *site)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: set_site");
	g_free(view->site);
	view->site = g_strdup(site);
	_gis_view_emit_site_changed(view);
}

gchar *gis_view_get_site(GisView *view)
{
	g_assert(GIS_IS_VIEW(view));
	g_debug("GisView: get_site - %s", view->site);
	return view->site;
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisView, gis_view, G_TYPE_OBJECT);
static void gis_view_init(GisView *self)
{
	g_debug("GisView: init");
	/* Default values */
	self->time = g_strdup("");
	self->site = g_strdup("");
	self->location[0] = 40;
	self->location[1] = -100;
	self->location[2] = 1.5*EARTH_R;
	self->rotation[0] = 0;
	self->rotation[1] = 0;
	self->rotation[2] = 0;
}
static void gis_view_dispose(GObject *gobject)
{
	g_debug("GisView: dispose");
	/* Drop references to other GObjects */
	G_OBJECT_CLASS(gis_view_parent_class)->dispose(gobject);
}
static void gis_view_finalize(GObject *gobject)
{
	g_debug("GisView: finalize");
	GisView *self = GIS_VIEW(gobject);
	g_free(self->time);
	g_free(self->site);
	G_OBJECT_CLASS(gis_view_parent_class)->finalize(gobject);
}
static void gis_view_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec)
{
	g_debug("GisView: set_property");
	GisView *self = GIS_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     gis_view_set_time(self, g_value_get_string(value)); break;
	case PROP_SITE:     gis_view_set_site(self, g_value_get_string(value)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static void gis_view_get_property(GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	g_debug("GisView: get_property");
	GisView *self = GIS_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     g_value_set_string(value, gis_view_get_time(self)); break;
	case PROP_SITE:     g_value_set_string(value, gis_view_get_site(self)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static void gis_view_class_init(GisViewClass *klass)
{
	g_debug("GisView: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose      = gis_view_dispose;
	gobject_class->finalize     = gis_view_finalize;
	gobject_class->get_property = gis_view_get_property;
	gobject_class->set_property = gis_view_set_property;
	g_object_class_install_property(gobject_class, PROP_TIME,
		g_param_spec_pointer(
			"time",
			"time of the current frame",
			"(format unknown)", 
			G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_SITE,
		g_param_spec_pointer(
			"site",
			"site seen by the viewport",
			"Site of the viewport. Currently this is the name of the radar site.", 
			G_PARAM_READWRITE));
	signals[SIG_TIME_CHANGED] = g_signal_new(
			"time-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING);
	signals[SIG_SITE_CHANGED] = g_signal_new(
			"site-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING);
	signals[SIG_LOCATION_CHANGED] = g_signal_new(
			"location-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			gis_cclosure_marshal_VOID__DOUBLE_DOUBLE_DOUBLE,
			G_TYPE_NONE,
			3,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE);
	signals[SIG_ROTATION_CHANGED] = g_signal_new(
			"rotation-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			gis_cclosure_marshal_VOID__DOUBLE_DOUBLE_DOUBLE,
			G_TYPE_NONE,
			3,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE);
}
