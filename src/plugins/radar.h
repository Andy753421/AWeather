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

#ifndef __RADAR_H__
#define __RADAR_H__

#include <glib-object.h>
#include <libsoup/soup.h>
#include <rsl.h>

#include <gis/gis.h>

#include "marching.h"

/* TODO: convert */
typedef struct {
	char *name;
	guint8 data[256][4];
} colormap_t;
extern colormap_t colormaps[];

#define GIS_TYPE_PLUGIN_RADAR            (gis_plugin_radar_get_type ())
#define GIS_PLUGIN_RADAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_PLUGIN_RADAR, GisPluginRadar))
#define GIS_IS_PLUGIN_RADAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_PLUGIN_RADAR))
#define GIS_PLUGIN_RADAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_PLUGIN_RADAR, GisPluginRadarClass))
#define GIS_IS_PLUGIN_RADAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_PLUGIN_RADAR))
#define GIS_PLUGIN_RADAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_PLUGIN_RADAR, GisPluginRadarClass))

typedef struct _GisPluginRadar        GisPluginRadar;
typedef struct _GisPluginRadarClass   GisPluginRadarClass;

struct _GisPluginRadar {
	GObject parent_instance;

	/* instance members */
	GisViewer   *viewer;
	GisPrefs    *prefs;
	GtkWidget   *config_body;
	GtkWidget   *progress_bar;
	GtkWidget   *progress_label;
	SoupSession *soup;
	guint        time_changed_id;

	/* Private data for loading radars */
	Radar       *cur_radar;
	Sweep       *cur_sweep;
	colormap_t  *cur_colormap;
	guint        cur_sweep_tex;
	TRIANGLE    *cur_triangles;
	guint        cur_num_triangles;
};

struct _GisPluginRadarClass {
	GObjectClass parent_class;
};

GType gis_plugin_radar_get_type();

/* Methods */
GisPluginRadar *gis_plugin_radar_new(GisViewer *viewer, GisPrefs *prefs);

/* Misc. RSL helpers */
#define RSL_FOREACH_VOL(radar, volume, count, index) \
	guint count = 0; \
	for (guint index = 0; index < radar->h.nvolumes; index++) { \
		Volume *volume = radar->v[index]; \
		if (volume == NULL) continue; \
		count++;

#define RSL_FOREACH_SWEEP(volume, sweep, count, index) \
	guint count = 0; \
	for (guint index = 0; index < volume->h.nsweeps; index++) { \
		Sweep *sweep = volume->sweep[index]; \
		if (sweep == NULL || sweep->h.elev == 0) continue; \
		count++;

#define RSL_FOREACH_RAY(sweep, ray, count, index) \
	guint count = 0; \
	for (guint index = 0; index < sweep->h.nrays; index++) { \
		Ray *ray = sweep->ray[index]; \
		if (ray == NULL) continue; \
		count++;

#define RSL_FOREACH_BIN(ray, bin, count, index) \
	guint count = 0; \
	for (guint index = 0; index < ray->h.nbins; index++) { \
		Range bin = ray->range[index]; \
		count++;

#define RSL_FOREACH_END }

#endif
