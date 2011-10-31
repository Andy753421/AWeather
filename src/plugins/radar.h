/*
 * Copyright (C) 2009-2011 Andy Spencer <andy753421@gmail.com>
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
#include <rsl.h>

#include <grits.h>
#include "radar-info.h"
#include "level2.h"

#define GRITS_TYPE_PLUGIN_RADAR            (grits_plugin_radar_get_type ())
#define GRITS_PLUGIN_RADAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GRITS_TYPE_PLUGIN_RADAR, GritsPluginRadar))
#define GRITS_IS_PLUGIN_RADAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GRITS_TYPE_PLUGIN_RADAR))
#define GRITS_PLUGIN_RADAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GRITS_TYPE_PLUGIN_RADAR, GritsPluginRadarClass))
#define GRITS_IS_PLUGIN_RADAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GRITS_TYPE_PLUGIN_RADAR))
#define GRITS_PLUGIN_RADAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GRITS_TYPE_PLUGIN_RADAR, GritsPluginRadarClass))

typedef struct _GritsPluginRadar      GritsPluginRadar;
typedef struct _GritsPluginRadarClass GritsPluginRadarClass;

typedef struct _RadarConus RadarConus;
typedef struct _RadarSite  RadarSite;

struct _GritsPluginRadar {
	GObject parent_instance;

	/* instance members */
	GritsViewer *viewer;
	GritsPrefs  *prefs;
	GtkWidget   *config;
	guint        tab_id;
	AWeatherColormap *colormap;
	GritsCallback    *hud;

	GHashTable  *sites;
	GritsHttp   *sites_http;

	RadarConus  *conus;
	GritsHttp   *conus_http;
};

struct _GritsPluginRadarClass {
	GObjectClass parent_class;
};

GType grits_plugin_radar_get_type();

/* Methods */
GritsPluginRadar *grits_plugin_radar_new(GritsViewer *viewer, GritsPrefs *prefs);

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
