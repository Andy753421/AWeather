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

#include <gis/gis.h>

#include "aweather-gui.h"

static gint log_levels = 0;

static void log_func(const gchar *log_domain, GLogLevelFlags log_level,
              const gchar *message, gpointer udata)
{
	if (log_level & log_levels)
		g_log_default_handler(log_domain, log_level, message, udata);
}

static void on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	gint value = gtk_spin_button_get_value_as_int(spinner);
	log_levels = (1 << (value+1))-1;
}

static gulong on_map_id = 0;
static gboolean on_map(AWeatherGui *gui, GdkEvent *event, gchar *site)
{
	GisView *view = aweather_gui_get_view(gui);
	gis_view_set_site(view, site);
	g_signal_handler_disconnect(gui, on_map_id);
	return FALSE;
}

/********
 * Main *
 ********/
int main(int argc, char *argv[])
{
	/* Arguments */
	gint     opt_debug   = 6;
	gchar   *opt_site    = "KIND";
	gboolean opt_auto    = FALSE;
	gboolean opt_offline = FALSE;
	GOptionEntry entries[] = {
		//long      short flg type                 location      description                 arg desc
		{"debug",   'd',  0,  G_OPTION_ARG_INT,    &opt_debug,   "Change default log level", "[1-7]"},
		{"site",    's',  0,  G_OPTION_ARG_STRING, &opt_site,    "Set initial site",         NULL},
		{"offline", 'o',  0,  G_OPTION_ARG_NONE,   &opt_offline, "Run in offline mode",      NULL},
		{"auto",    'a',  0,  G_OPTION_ARG_NONE,   &opt_auto,    "Auto update radar (todo)", NULL},
		{NULL}
	};

	/* Init */
	GError *error = NULL;
	g_thread_init(NULL);
	if (!gtk_init_with_args(&argc, &argv, "aweather", entries, NULL, &error)) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return -1;
	}
	gtk_gl_init(&argc, &argv);

	/* Logging */
	log_levels = (1 << (opt_debug+1))-1;
	g_log_set_handler(NULL, G_LOG_LEVEL_MASK, log_func, NULL);

	/* Set up AWeather */
	AWeatherGui *gui    = aweather_gui_new();
	GisWorld    *world  = aweather_gui_get_world(gui);
	GisOpenGL   *opengl = aweather_gui_get_opengl(gui);

	gis_world_set_offline(world, opt_offline);
	on_map_id = g_signal_connect(gui, "map-event", G_CALLBACK(on_map), opt_site);

	GObject *action = aweather_gui_get_object(gui, "log_level");
	g_signal_connect(action, "changed", G_CALLBACK(on_log_level_changed), NULL);

	gtk_widget_show_all(GTK_WIDGET(gui));
	gtk_main();

	return 0;
}
