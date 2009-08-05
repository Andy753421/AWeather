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

#ifndef __RIDGE_H__
#define __RIDGE_H__

#include <glib-object.h>

#define AWEATHER_TYPE_RIDGE            (aweather_ridge_get_type ())
#define AWEATHER_RIDGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_RIDGE, AWeatherRidge))
#define AWEATHER_IS_RIDGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_RIDGE))
#define AWEATHER_RIDGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_RIDGE, AWeatherRidgeClass))
#define AWEATHER_IS_RIDGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_RIDGE))
#define AWEATHER_RIDGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_RIDGE, AWeatherRidgeClass))

typedef struct _AWeatherRidge      AWeatherRidge;
typedef struct _AWeatherRidgeClass AWeatherRidgeClass;

struct _AWeatherRidge {
	GObject parent_instance;

	/* instance members */
	AWeatherGui *gui;
};

struct _AWeatherRidgeClass {
	GObjectClass parent_class;
};

GType aweather_ridge_get_type();

/* Methods */
AWeatherRidge *aweather_ridge_new(AWeatherGui *gui);

#endif
