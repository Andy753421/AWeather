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

#include "marshal.h"
#include "aweather-view.h"

/****************
 * GObject code *
 ****************/
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
	SIG_REFRESH,
	SIG_OFFLINE,
	NUM_SIGNALS,
};
static guint signals[NUM_SIGNALS];

/* Class/Object init */
G_DEFINE_TYPE(AWeatherView, aweather_view, G_TYPE_OBJECT);
static void aweather_view_init(AWeatherView *self)
{
	g_debug("AWeatherView: init");
	/* Default values */
	self->time = g_strdup("");
	self->site = g_strdup("");
	self->location[0] = 0;
	self->location[1] = 0;
	self->location[2] = -300*1000;
	self->offline = FALSE;
}
static void aweather_view_dispose(GObject *gobject)
{
	g_debug("AWeatherView: dispose");
	/* Drop references to other GObjects */
	G_OBJECT_CLASS(aweather_view_parent_class)->dispose(gobject);
}
static void aweather_view_finalize(GObject *gobject)
{
	g_debug("AWeatherView: finalize");
	AWeatherView *self = AWEATHER_VIEW(gobject);
	g_free(self->time);
	g_free(self->site);
	G_OBJECT_CLASS(aweather_view_parent_class)->finalize(gobject);
}
static void aweather_view_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec)
{
	g_debug("AWeatherView: set_property");
	AWeatherView *self = AWEATHER_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     aweather_view_set_time(self, g_value_get_string(value)); break;
	case PROP_SITE:     aweather_view_set_site(self, g_value_get_string(value)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static void aweather_view_get_property(GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	g_debug("AWeatherView: get_property");
	AWeatherView *self = AWEATHER_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     g_value_set_string(value, aweather_view_get_time(self)); break;
	case PROP_SITE:     g_value_set_string(value, aweather_view_get_site(self)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static void aweather_view_class_init(AWeatherViewClass *klass)
{
	g_debug("AWeatherView: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose      = aweather_view_dispose;
	gobject_class->finalize     = aweather_view_finalize;
	gobject_class->get_property = aweather_view_get_property;
	gobject_class->set_property = aweather_view_set_property;
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
			aweather_cclosure_marshal_VOID__DOUBLE_DOUBLE_DOUBLE,
			G_TYPE_NONE,
			3,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE,
			G_TYPE_DOUBLE);
	signals[SIG_REFRESH] = g_signal_new(
			"refresh",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	signals[SIG_OFFLINE] = g_signal_new(
			"offline",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__BOOLEAN,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN);
}

/* Signal helpers */
static void _aweather_view_emit_location_changed(AWeatherView *view)
{
	g_signal_emit(view, signals[SIG_LOCATION_CHANGED], 0, 
			view->location[0],
			view->location[1],
			view->location[2]);
}
static void _aweather_view_emit_time_changed(AWeatherView *view)
{
	g_signal_emit(view, signals[SIG_TIME_CHANGED], 0,
			view->time);
}
static void _aweather_view_emit_site_changed(AWeatherView *view)
{
	g_signal_emit(view, signals[SIG_SITE_CHANGED], 0,
			view->site);
}
static void _aweather_view_emit_refresh(AWeatherView *view)
{
	g_signal_emit(view, signals[SIG_REFRESH], 0);
}
static void _aweather_view_emit_offline(AWeatherView *view)
{
	g_signal_emit(view, signals[SIG_OFFLINE], 0,
			view->offline);
}


/***********
 * Methods *
 ***********/
AWeatherView *aweather_view_new()
{
	g_debug("AWeatherView: new");
	return g_object_new(AWEATHER_TYPE_VIEW, NULL);
}

void aweather_view_set_time(AWeatherView *view, const char *time)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: set_time - time=%s", time);
	g_free(view->time);
	view->time = g_strdup(time);
	_aweather_view_emit_time_changed(view);
}

gchar *aweather_view_get_time(AWeatherView *view)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: get_time");
	return view->time;
}

void aweather_view_set_location(AWeatherView *view, gdouble x, gdouble y, gdouble z)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: set_location");
	view->location[0] = x;
	view->location[1] = y;
	view->location[2] = z;
	_aweather_view_emit_location_changed(view);
}

void aweather_view_get_location(AWeatherView *view, gdouble *x, gdouble *y, gdouble *z)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: get_location");
	*x = view->location[0];
	*y = view->location[1];
	*z = view->location[2];
}

void aweather_view_pan(AWeatherView *view, gdouble x, gdouble y, gdouble z)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: pan - x=%.0f, y=%.0f, z=%.0f", x, y, z);
	view->location[0] += x;
	view->location[1] += y;
	view->location[2] += z;
	_aweather_view_emit_location_changed(view);
}

void aweather_view_zoom(AWeatherView *view, gdouble scale)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: zoom");
	view->location[2] *= scale;
	_aweather_view_emit_location_changed(view);
}

void aweather_view_refresh(AWeatherView *view)
{
	g_debug("AWeatherView: refresh");
	_aweather_view_emit_refresh(view);
}

void aweather_view_set_offline(AWeatherView *view, gboolean offline)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: set_offline - %d", offline);
	view->offline = offline;
	_aweather_view_emit_offline(view);
}

gboolean aweather_view_get_offline(AWeatherView *view)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: get_offline - %d", view->offline);
	return view->offline;
}

/* To be deprecated, use {get,set}_location */
void aweather_view_set_site(AWeatherView *view, const gchar *site)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: set_site");
	g_free(view->site);
	view->site = g_strdup(site);
	_aweather_view_emit_site_changed(view);
}

gchar *aweather_view_get_site(AWeatherView *view)
{
	g_assert(AWEATHER_IS_VIEW(view));
	g_debug("AWeatherView: get_site");
	return view->site;
}
