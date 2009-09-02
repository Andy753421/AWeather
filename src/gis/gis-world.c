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
#include <math.h>

#include "gis-marshal.h"
#include "gis-world.h"

/* Constants */
double WGS884_SEMI_MAJOR = 6378137.0;      // a
double WGS884_SEMI_MINOR = 6356752.314245; // b
double WGS884_INV_FLAT   = 298.257223563;  // 1/f

enum {
	SIG_REFRESH,
	SIG_OFFLINE,
	NUM_SIGNALS,
};
static guint signals[NUM_SIGNALS];


/* Signal helpers */
static void _gis_world_emit_refresh(GisWorld *world)
{
	g_signal_emit(world, signals[SIG_REFRESH], 0);
}
static void _gis_world_emit_offline(GisWorld *world)
{
	g_signal_emit(world, signals[SIG_OFFLINE], 0,
			world->offline);
}

/***********
 * Methods *
 ***********/
GisWorld *gis_world_new()
{
	g_debug("GisWorld: new");
	return g_object_new(GIS_TYPE_WORLD, NULL);
}

void gis_world_refresh(GisWorld *world)
{
	g_debug("GisWorld: refresh");
	_gis_world_emit_refresh(world);
}

void gis_world_set_offline(GisWorld *world, gboolean offline)
{
	g_assert(GIS_IS_WORLD(world));
	g_debug("GisWorld: set_offline - %d", offline);
	world->offline = offline;
	_gis_world_emit_offline(world);
}

gboolean gis_world_get_offline(GisWorld *world)
{
	g_assert(GIS_IS_WORLD(world));
	g_debug("GisWorld: get_offline - %d", world->offline);
	return world->offline;
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisWorld, gis_world, G_TYPE_OBJECT);
static void gis_world_init(GisWorld *self)
{
	g_debug("GisWorld: init");
	/* Default values */
	self->offline = FALSE;
}
static void gis_world_dispose(GObject *gobject)
{
	g_debug("GisWorld: dispose");
	/* Drop references to other GObjects */
	G_OBJECT_CLASS(gis_world_parent_class)->dispose(gobject);
}
static void gis_world_finalize(GObject *gobject)
{
	g_debug("GisWorld: finalize");
	GisWorld *self = GIS_WORLD(gobject);
	G_OBJECT_CLASS(gis_world_parent_class)->finalize(gobject);
}
static void gis_world_class_init(GisWorldClass *klass)
{
	g_debug("GisWorld: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose      = gis_world_dispose;
	gobject_class->finalize     = gis_world_finalize;
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


/******************
 * Global helpers *
 ******************/
void lle2xyz(gdouble lat, gdouble lon, gdouble elev,
		gdouble *x, gdouble *y, gdouble *z)
{
	gdouble rad  = elev2rad(elev);
	gdouble azim = lon2azim(lon);
	gdouble incl = lat2incl(lat);
	*z = rad * cos(azim) * sin(incl);
	*x = rad * sin(azim) * sin(incl);
	*y = rad * cos(incl);
}

void xyz2lle(gdouble x, gdouble y, gdouble z,
		gdouble *lat, gdouble *lon, gdouble *elev)
{
	gdouble rad = sqrt(x*x + y*y + z*z);
	*lat  = incl2lat(acos(y / rad));
	*lon  = azim2lon(atan2(x,z));
	*elev = rad2elev(rad);
}


gdouble ll2m(gdouble lon_dist, gdouble lat)
{
	gdouble azim = (-lat+90)/180*M_PI;
	gdouble rad  = sin(azim) * EARTH_R;
	gdouble circ = 2 * M_PI * rad;
	return lon_dist/360 * circ;
}

gdouble distd(gdouble *a, gdouble *b)
{
	return sqrt((a[0]-b[0])*(a[0]-b[0]) +
	            (a[1]-b[1])*(a[1]-b[1]) +
	            (a[2]-b[2])*(a[2]-b[2]));
}
