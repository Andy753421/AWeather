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

#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

#include "aweather-gui.h"
#include "plugin-radar.h"
#include "plugin-ridge.h"
#include "plugin-example.h"

/********
 * Main *
 ********/
int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	gtk_gl_init(&argc, &argv);

	/* Set up AWeather */
	AWeatherGui  *gui  = aweather_gui_new();
	//AWeatherView *view = aweather_gui_get_view(gui);

	/* Load plugins */
	aweather_gui_register_plugin(gui, AWEATHER_PLUGIN(aweather_example_new(gui)));
	aweather_gui_register_plugin(gui, AWEATHER_PLUGIN(aweather_ridge_new(gui)));
	aweather_gui_register_plugin(gui, AWEATHER_PLUGIN(aweather_radar_new(gui)));

	gtk_widget_show_all(aweather_gui_get_widget(gui, "window"));
	gtk_main();

	return 0;
}
