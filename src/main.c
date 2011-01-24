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

#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <glib/gstdio.h>
#include <time.h>

#include <grits.h>

#include "aweather-gui.h"
#include "aweather-location.h"

static gint log_levels = 0;

static void log_func(const gchar *log_domain, GLogLevelFlags log_level,
              const gchar *message, gpointer udata)
{
	if (log_level & log_levels) {
		if (log_level == G_LOG_LEVEL_DEBUG)
			g_fprintf(stderr, "DEBUG: (%p) %s\n",
					g_thread_self(), message);
		else
			g_log_default_handler(log_domain, log_level, message, udata);
	}
}

static void on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	g_message("main: log_level_changed");
	gint value = gtk_spin_button_get_value_as_int(spinner);
	log_levels = (1 << (value+1))-1;
}

gboolean set_location_time(AWeatherGui *gui, char *site, char *time)
{
	/* Set time
	 *   Do this before setting setting location
	 *   so that it doesn't refresh twice */
	struct tm tm = {};
	strptime(time, "%Y-%m-%d %H:%M", &tm);
	time_t sec = mktime(&tm);
	if (sec > 0)
		grits_viewer_set_time(gui->viewer, sec);
	g_debug("date = [%s] == %lu\n", time, sec);

	/* Set location */
	for (city_t *city = cities; city->type; city++) {
		if (city->type == LOCATION_CITY && g_str_equal(city->code, site)) {
			grits_viewer_set_location(gui->viewer,
				city->pos.lat, city->pos.lon, EARTH_R/25);
			break;
		}
	}
	return FALSE;
}


/********
 * Main *
 ********/
int main(int argc, char *argv[])
{
	/* Defaults */
	gint     debug   = 6;
	gchar   *site    = "";
	gchar   *time    = "";
	gboolean offline = FALSE;

	/* Arguments */
	gint     opt_debug   = 0;
	gchar   *opt_site    = NULL;
	gchar   *opt_time    = NULL;
	gboolean opt_auto    = FALSE;
	gboolean opt_offline = FALSE;
	GOptionEntry entries[] = {
		//long      short flg type                 location      description                 arg desc
		{"debug",   'd',  0,  G_OPTION_ARG_INT,    &opt_debug,   "Change default log level", "[1-7]"},
		{"site",    's',  0,  G_OPTION_ARG_STRING, &opt_site,    "Set initial site",         NULL},
		{"time",    't',  0,  G_OPTION_ARG_STRING, &opt_time,    "Set initial date/time",    NULL},
		{"offline", 'o',  0,  G_OPTION_ARG_NONE,   &opt_offline, "Run in offline mode",      NULL},
		{"auto",    'a',  0,  G_OPTION_ARG_NONE,   &opt_auto,    "Auto update radar (todo)", NULL},
		{NULL}
	};

	/* Init */
	GError *error = NULL;
	g_thread_init(NULL);
	gdk_threads_init();
	if (!gtk_init_with_args(&argc, &argv, "aweather", entries, NULL, &error)) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return -1;
	}
	gtk_gl_init(&argc, &argv);

	/* Do some logging here for aweather_gui_new */
	if (opt_debug) log_levels = (1 << (opt_debug+1))-1;
	else           log_levels = (1 << (6+1))-1;
	g_log_set_handler(NULL, G_LOG_LEVEL_MASK, log_func, NULL);

	/* Set up AWeather */
	gdk_threads_enter();
	/* Pre-load some type for gtkbuilder */
	GRITS_TYPE_OPENGL;
	AWEATHER_TYPE_GUI;
	GtkBuilder *builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, PKGDATADIR "/main.ui", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	AWeatherGui *gui = AWEATHER_GUI(gtk_builder_get_object(builder, "main_window"));
	g_signal_connect(gui, "destroy", gtk_main_quit, NULL);

	gint     prefs_debug   = grits_prefs_get_integer(gui->prefs, "aweather/log_level", NULL);
	gchar   *prefs_site    = grits_prefs_get_string(gui->prefs,  "aweather/initial_site", NULL);
	gboolean prefs_offline = grits_prefs_get_boolean(gui->prefs, "grits/offline", NULL);

	debug   = (opt_debug   ?: prefs_debug   ?: debug);
	site    = (opt_site    ?: prefs_site    ?: site);
	time    = (opt_time    ?:                  time);
	offline = (opt_offline ?: prefs_offline ?: offline);

	set_location_time(gui, site, time);
	grits_viewer_set_offline(gui->viewer, offline);
	log_levels = (1 << (debug+1))-1;

	GObject *action = aweather_gui_get_object(gui, "prefs_general_log");
	g_signal_connect(action, "changed", G_CALLBACK(on_log_level_changed), NULL);

	gtk_widget_show_all(GTK_WIDGET(gui));
	gtk_main();
	gdk_threads_leave();
	gdk_display_close(gdk_display_get_default());
	return 0;
}
