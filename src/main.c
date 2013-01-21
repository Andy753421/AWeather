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

#define _XOPEN_SOURCE
#include <sys/time.h>
#include <config.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

#include <grits.h>

#include "aweather-gui.h"
#include "aweather-location.h"

static gint log_levels = 0;

static int int2log(int level) {
	level = G_LOG_LEVEL_ERROR << level;
	level = (level<<1) - 1;
	level = level & G_LOG_LEVEL_MASK;
	return level;
}

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

#if ! GTK_CHECK_VERSION(3,0,0)
static void xdg_open(GtkWidget *widget, const gchar *link, gpointer user_data)
{
	gchar *argv[] = {"xdg-open", (gchar*)link, NULL};
	g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}
#endif

static void on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	g_message("main: log_level_changed");
	log_levels = int2log(gtk_spin_button_get_value_as_int(spinner));
}

static void set_location_time(AWeatherGui *gui, char *site, char *time)
{
	/* Set time
	 *   Do this before setting setting location
	 *   so that it doesn't refresh twice */
	if (time) {
		int year, mon, day, hour, min;
		sscanf(time, "%d-%d-%d %d:%d", &year, &mon, &day, &hour, &min);
		time_t sec = mktime(&(struct tm){0, year-1900, mon-1, day, hour, min});
		if (sec > 0)
			grits_viewer_set_time(gui->viewer, sec);
		g_debug("date = [%s] == %lu\n", time, sec);
	}

	/* Set location */
	if (site) {
		for (city_t *city = cities; city->type; city++) {
			if (city->type == LOCATION_CITY && g_str_equal(city->code, site)) {
				grits_viewer_set_location(gui->viewer,
					city->pos.lat, city->pos.lon, EARTH_R/35);
				break;
			}
		}
	}
}

static void set_toggle_action(AWeatherGui *gui, const char *action, gboolean enabled)
{
	GObject *object = aweather_gui_get_object(gui, action);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(object), enabled);
}

static void setup_mac(AWeatherGui *gui)
{
#ifdef MAC_INTEGRATION
	GtkWidget *menu = aweather_gui_get_widget(gui, "main_menu");
	GtkosxApplication *app = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
	gtk_widget_hide(menu);
	gtkosx_application_set_menu_bar(app, GTK_MENU_SHELL(menu));
	gtkosx_application_set_use_quartz_accelerators(app, TRUE);
	gtkosx_application_ready(app);
	//gtkosx_application_sync_menubar(app)
#endif
}

/********
 * Main *
 ********/
int main(int argc, char *argv[])
{
	/* Defaults */
	gint     debug      = 2; // G_LOG_LEVEL_WARNING
	gchar   *site       = NULL;
	gchar   *time       = NULL;
	gboolean autoupdate = FALSE;
	gboolean offline    = FALSE;
	gboolean fullscreen = FALSE;

	/* Arguments */
	gint     opt_debug      = -1;
	gchar   *opt_site       = NULL;
	gchar   *opt_time       = NULL;
	gboolean opt_offline    = FALSE;
	gboolean opt_autoupdate = FALSE;
	gboolean opt_fullscreen = FALSE;
	GOptionEntry entries[] = {
		//long         short flg type                 location         description                 arg desc
		{"debug",      'd',  0,  G_OPTION_ARG_INT,    &opt_debug,      "Change default log level", "[0-5]"},
		{"site",       's',  0,  G_OPTION_ARG_STRING, &opt_site,       "Set initial site",         "SITE"},
		{"time",       't',  0,  G_OPTION_ARG_STRING, &opt_time,       "Set initial date/time",    "DATE"},
		{"offline",    'o',  0,  G_OPTION_ARG_NONE,   &opt_offline,    "Run in offline mode",      NULL},
		{"autoupdate", 'a',  0,  G_OPTION_ARG_NONE,   &opt_autoupdate, "Auto update radar",        NULL},
		{"fullscreen", 'f',  0,  G_OPTION_ARG_NONE,   &opt_fullscreen, "Open in fullscreen mode",  NULL},
		{NULL}
	};

	/* All times in UTC */
	g_setenv("TZ", "UTC", TRUE);

	/* Init */
	GError *error = NULL;
	if (!gtk_init_with_args(&argc, &argv, "aweather", entries, NULL, &error)) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return -1;
	}

	/* Use external handler for link buttons */
#if ! GTK_CHECK_VERSION(3,0,0)
	gtk_link_button_set_uri_hook((GtkLinkButtonUriFunc)xdg_open, NULL, NULL);
	gtk_about_dialog_set_url_hook((GtkAboutDialogActivateLinkFunc)xdg_open, NULL, NULL);
	gtk_about_dialog_set_email_hook((GtkAboutDialogActivateLinkFunc)xdg_open, NULL, NULL);
#endif

	/* Setup debug level for aweather_gui_new */
	g_log_set_handler(NULL, G_LOG_LEVEL_MASK, log_func, NULL);
	log_levels = int2log(opt_debug >= 0 ? opt_debug : debug);

	/* Set up AWeather */
	/* Pre-load some types for gtkbuilder */
	GRITS_TYPE_OPENGL;
	AWEATHER_TYPE_GUI;
	GtkBuilder *builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, PKGDATADIR "/main.ui", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	AWeatherGui *gui = AWEATHER_GUI(gtk_builder_get_object(builder, "main_window"));
	g_signal_connect(gui, "destroy", gtk_main_quit, NULL);
	GObject *action = aweather_gui_get_object(gui, "prefs_general_log");
	g_signal_connect(action, "changed", G_CALLBACK(on_log_level_changed), NULL);

	/* Finish setting up options */
	GError *err = NULL;
	gint     prefs_debug      = grits_prefs_get_integer(gui->prefs, "aweather/log_level",    &err);
	gchar   *prefs_site       = grits_prefs_get_string(gui->prefs,  "aweather/initial_site", NULL);
	gboolean prefs_offline    = grits_prefs_get_boolean(gui->prefs, "grits/offline",         NULL);
	gint     prefs_autoupdate = grits_prefs_get_boolean(gui->prefs, "aweather/update_enab",  NULL);

	debug      = (opt_debug >= 0 ? opt_debug   :
	              err == NULL    ? prefs_debug : debug);
	site       = (opt_site       ?: prefs_site       ?: site);
	time       = (opt_time       ?:                     time);
	offline    = (opt_offline    ?: prefs_offline    ?: offline);
	autoupdate = (opt_autoupdate ?: prefs_autoupdate ?: autoupdate);
	fullscreen = (opt_fullscreen ?:                     fullscreen);

	log_levels = int2log(debug);
	set_location_time(gui, site, time);
	grits_viewer_set_offline(gui->viewer, offline);
	set_toggle_action(gui, "update",     autoupdate);
	set_toggle_action(gui, "fullscreen", fullscreen);
	g_free(prefs_site);

	/* Done with init, show gui */
	gtk_widget_show_all(GTK_WIDGET(gui));
	set_toggle_action(gui, "fullscreen", fullscreen); // Resest widget hiding
	setup_mac(gui);	// done after show_all
	gtk_main();
	//gdk_display_close(gdk_display_get_default());
	return 0;
}
