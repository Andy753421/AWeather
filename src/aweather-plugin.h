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

#ifndef __AWEATHER_PLUGIN_H__
#define __AWEATHER_PLUGIN_H__

#include <glib-object.h>

#define AWEATHER_TYPE_PLUGIN                (aweather_plugin_get_type())
#define AWEATHER_PLUGIN(obj)                (G_TYPE_CHECK_INSTANCE_CAST   ((obj),  AWEATHER_TYPE_PLUGIN, AWeatherPlugin))
#define AWEATHER_IS_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE   ((obj),  AWEATHER_TYPE_PLUGIN))
#define AWEATHER_PLUGIN_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), AWEATHER_TYPE_PLUGIN, AWeatherPluginInterface))


typedef struct _AWeatherPlugin          AWeatherPlugin;
typedef struct _AWeatherPluginInterface AWeatherPluginInterface;

struct _AWeatherPluginInterface
{
	GTypeInterface parent_iface;

	/* Virtual functions */
	void (*expose)(AWeatherPlugin *self);
};

GType aweather_plugin_get_type();

void aweather_plugin_expose(AWeatherPlugin *self);

#endif
