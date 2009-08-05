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

#ifndef __EXAMPLE_H__
#define __EXAMPLE_H__

#include <glib-object.h>

#define AWEATHER_TYPE_EXAMPLE            (aweather_example_get_type ())
#define AWEATHER_EXAMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_EXAMPLE, AWeatherExample))
#define AWEATHER_IS_EXAMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_EXAMPLE))
#define AWEATHER_EXAMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_EXAMPLE, AWeatherExampleClass))
#define AWEATHER_IS_EXAMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_EXAMPLE))
#define AWEATHER_EXAMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_EXAMPLE, AWeatherExampleClass))

typedef struct _AWeatherExample        AWeatherExample;
typedef struct _AWeatherExampleClass   AWeatherExampleClass;

struct _AWeatherExample {
	GObject parent_instance;

	/* instance members */
	AWeatherGui     *gui;
	GtkToggleButton *button;
	float            rotation;
};

struct _AWeatherExampleClass {
	GObjectClass parent_class;
};

GType aweather_example_get_type();

/* Methods */
AWeatherExample *aweather_example_new(AWeatherGui *gui);

#endif
