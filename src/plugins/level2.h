/*
 * Copyright (C) 2009-2010 Andy Spencer <andy753421@gmail.com>
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

#ifndef __AWEATHER_LEVEL2_H__
#define __AWEATHER_LEVEL2_H__

#include <gis.h>
#include "aweather-colormap.h"

/* Level2 */
#define AWEATHER_TYPE_LEVEL2 (aweather_level2_get_type())

GOBJECT_HEAD(
	AWEATHER, LEVEL2,
	AWeather, Level2,
	aweather, level2);

struct _AWeatherLevel2 {
	GisCallback       parent;
	GisViewer        *viewer;
	Radar            *radar;
	AWeatherColormap *colormap;

	/* Private */
	Sweep            *sweep;
	AWeatherColormap *sweep_colors;
	guint             sweep_tex;
};

struct _AWeatherLevel2Class {
	GisCallbackClass parent_class;
};

AWeatherLevel2 *aweather_level2_new(GisViewer *viewer,
		AWeatherColormap *colormap, Radar *radar);

AWeatherLevel2 *aweather_level2_new_from_file(GisViewer *viewer,
		AWeatherColormap *colormap,
		const gchar *file, const gchar *site);

void aweather_level2_set_sweep(AWeatherLevel2 *level2,
		int type, float elev);

#endif
