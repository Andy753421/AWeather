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

#ifndef __AWEATHER_LEVEL2_H__
#define __AWEATHER_LEVEL2_H__

#include <grits.h>
#include "radar-info.h"

/* Level2 */
#define AWEATHER_TYPE_LEVEL2            (aweather_level2_get_type())
#define AWEATHER_LEVEL2(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_LEVEL2, AWeatherLevel2))
#define AWEATHER_IS_LEVEL2(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_LEVEL2))
#define AWEATHER_LEVEL2_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_LEVEL2, AWeatherLevel2Class))
#define AWEATHER_IS_LEVEL2_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_LEVEL2))
#define AWEATHER_LEVEL2_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_LEVEL2, AWeatherLevel2Class))

typedef struct _AWeatherLevel2      AWeatherLevel2;
typedef struct _AWeatherLevel2Class AWeatherLevel2Class;

struct _AWeatherLevel2 {
	GritsObject       parent;
	Radar            *radar;
	AWeatherColormap *colormap;

	/* Private */
	GritsVolume      *volume;
	Sweep            *sweep;
	AWeatherColormap *sweep_colors;
	gdouble           sweep_coords[2];
	guint             sweep_tex;
};

struct _AWeatherLevel2Class {
	GritsObjectClass parent_class;
};

GType aweather_level2_get_type(void);

AWeatherLevel2 *aweather_level2_new(Radar *radar, AWeatherColormap *colormap);

AWeatherLevel2 *aweather_level2_new_from_file(const gchar *file, const gchar *site,
		AWeatherColormap *colormap);

void aweather_level2_set_sweep(AWeatherLevel2 *level2,
		int type, gfloat elev);

void aweather_level2_set_iso(AWeatherLevel2 *level2, gfloat level);

GtkWidget *aweather_level2_get_config(AWeatherLevel2 *level2);

#endif
