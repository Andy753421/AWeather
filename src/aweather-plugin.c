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

#include "aweather-plugin.h"

static void aweather_plugin_base_init(gpointer g_class)
{
	static gboolean is_initialized = FALSE;
	if (!is_initialized) {
		/* add properties and signals to the interface here */
		is_initialized = TRUE;
	}
}

GType aweather_plugin_get_type()
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof(AWeatherPluginInterface),
			aweather_plugin_base_init,
			NULL,
		};
		type = g_type_register_static(G_TYPE_INTERFACE,
				"AWeatherPlugin", &info, 0);
	}
	return type;
}

void aweather_plugin_expose(AWeatherPlugin *self)
{
	g_return_if_fail(AWEATHER_IS_PLUGIN(self));
	AWEATHER_PLUGIN_GET_INTERFACE(self)->expose(self);
}
