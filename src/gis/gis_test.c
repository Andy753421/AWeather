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
#include <gdk/gdkkeysyms.h>

#include "gis.h"

/*************
 * Callbacks *
 *************/
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer _)
{
	g_debug("gis: on_key_press - key=%x, state=%x",
			event->keyval, event->state);
	switch (event->keyval) {
	case GDK_q:
		gtk_widget_destroy(widget);
		return TRUE;
	}
	return FALSE;
}

/***********
 * Methods *
 ***********/
int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	g_thread_init(NULL);

	GisPrefs   *prefs   = gis_prefs_new("aweather");
	GisPlugins *plugins = gis_plugins_new();
	GisWorld   *world   = gis_world_new();
	GisView    *view    = gis_view_new();
	GisOpenGL  *opengl  = gis_opengl_new(world, view, plugins);

	//gis_plugins_load(plugins, "radar",   world, view, opengl, prefs);
	//gis_plugins_load(plugins, "ridge",   world, view, opengl, prefs);
	//gis_plugins_load(plugins, "example", world, view, opengl, prefs);

	GtkWidget  *window  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window,  "destroy",         G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window,  "key-press-event", G_CALLBACK(on_key_press),  NULL);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(opengl));
	gtk_widget_show_all(window);

	gis_view_set_site(view, "KLSX");
	gtk_main();

	g_object_unref(prefs);
	g_object_unref(world);
	g_object_unref(view);
	g_object_unref(opengl);
	gis_plugins_free(plugins);
	return 0;
}
