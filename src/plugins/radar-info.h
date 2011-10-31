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

#ifndef __AWEATHER_COLORMAP_H__
#define __AWEATHER_COLORMAP_H__

#include <glib.h>
#include <rsl.h>

typedef struct {
	gint     type;     // From RSL e.g. DZ_INDEX
	gchar   *file;     // Basename of the colors file
	gchar    name[64]; // Name of the colormap          (line 1)
	gfloat   scale;    // Map values to color table idx (line 2)
	gfloat   shift;    //   index = value*scale + shift (line 3)
	gint     len;      // Length of data                    
	guint8 (*data)[4]; // The actual colormap           (line 4..)
} AWeatherColormap;

extern AWeatherColormap colormaps[];

static inline guint8 *colormap_get(AWeatherColormap *colormap, float value)
{
	int idx = value * colormap->scale + colormap->shift;
	return colormap->data[CLAMP(idx, 0, colormap->len)];
}

#endif
