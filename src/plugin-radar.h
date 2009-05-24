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
#include <rsl.h>

/* TODO: convert */
typedef struct {
	char *name;
	guint8 data[256][4];
} colormap_t;
extern colormap_t colormaps[];

#define AWEATHER_TYPE_RADAR            (aweather_radar_get_type ())
#define AWEATHER_RADAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_RADAR, AWeatherRadar))
#define AWEATHER_IS_RADAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_RADAR))
#define AWEATHER_RADAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_RADAR, AWeatherRadarClass))
#define AWEATHER_IS_RADAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_RADAR))
#define AWEATHER_RADAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_RADAR, AWeatherRadarClass))

typedef struct _AWeatherRadar        AWeatherRadar;
typedef struct _AWeatherRadarClass   AWeatherRadarClass;

struct _AWeatherRadar {
	GObject parent_instance;

	/* instance members */
	AWeatherGui *gui;
	GtkWidget   *config_body;

	/* Private data for loading radars */
	Radar       *cur_radar;
	Sweep       *cur_sweep;
	colormap_t  *cur_colormap;
	guint        cur_sweep_tex;
};

struct _AWeatherRadarClass {
	GObjectClass parent_class;
};

GType aweather_radar_get_type();

/* Methods */
AWeatherRadar *aweather_radar_new(AWeatherGui *gui);

#endif
