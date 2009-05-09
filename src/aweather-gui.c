#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include "aweather-gui.h"
#include "aweather-view.h"

/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherGui, aweather_gui, G_TYPE_OBJECT);

/* Constructor / destructors */
static void aweather_gui_init(AWeatherGui *gui)
{
	g_message("aweather_gui_init");
	/* Default values */
	gui->view    = NULL;
	gui->builder = NULL;
	gui->window  = NULL;
	gui->tabs    = NULL;
	gui->drawing = NULL;
}

static GObject *aweather_gui_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_message("aweather_gui_constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_gui_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}

static void aweather_gui_dispose (GObject *gobject)
{
	g_message("aweather_gui_dispose");
	AWeatherGui *gui = AWEATHER_GUI(gobject);
	g_object_unref(gui->view   );
	g_object_unref(gui->builder);
	g_object_unref(gui->window );
	g_object_unref(gui->tabs   );
	g_object_unref(gui->drawing);
	G_OBJECT_CLASS(aweather_gui_parent_class)->dispose(gobject);
}

static void aweather_gui_finalize(GObject *gobject)
{
	g_message("aweather_gui_finalize");
	G_OBJECT_CLASS(aweather_gui_parent_class)->finalize(gobject);
}

static void aweather_gui_class_init(AWeatherGuiClass *klass)
{
	g_message("aweather_gui_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = aweather_gui_constructor;
	gobject_class->dispose      = aweather_gui_dispose;
	gobject_class->finalize     = aweather_gui_finalize;
}

/* Methods */
AWeatherGui *aweather_gui_new()
{
	g_message("aweather_gui_new");
	GError *error = NULL;

	AWeatherGui *gui = g_object_new(AWEATHER_TYPE_GUI, NULL);

	gui->builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(gui->builder, DATADIR "/aweather/aweather.xml", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gui->view    = aweather_view_new();
	gui->window  = GTK_WINDOW      (gtk_builder_get_object(gui->builder, "window"));
	gui->tabs    = GTK_NOTEBOOK    (gtk_builder_get_object(gui->builder, "tabs")); 
	gui->drawing = GTK_DRAWING_AREA(gtk_builder_get_object(gui->builder, "drawing"));
	gtk_builder_connect_signals(gui->builder, NULL);
	return gui;
}
AWeatherView *aweather_gui_get_view(AWeatherGui *gui)
{
	return gui->view;
}
GtkBuilder *aweather_gui_get_builder(AWeatherGui *gui)
{
	return gui->builder;
}
GtkWindow *aweather_gui_get_window(AWeatherGui *gui)
{
	return gui->window;
}
GtkNotebook *aweather_gui_get_tabs(AWeatherGui *gui)
{
	return gui->tabs;
}
GtkDrawingArea *aweather_gui_get_drawing(AWeatherGui *gui)
{
	return gui->drawing;
}

//void aweather_gui_register_plugin(AWeatherGui *gui, AWeatherPlugin *plugin);

/************************
 * GtkBuilder callbacks *
 ************************/
gboolean on_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return TRUE;
}

void on_site_changed() {
	g_message("site changed");
}
