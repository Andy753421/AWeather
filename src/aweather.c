#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include "opengl.h"
#include "plugin-radar.h"
#include "plugin-ridge.h"
#include "plugin-example.h"

gboolean on_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return TRUE;
}

int main(int argc, char *argv[])
{

	gtk_init(&argc, &argv);
	gtk_gl_init(&argc, &argv);

	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, DATADIR "/aweather/aweather.xml", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gtk_builder_connect_signals(builder, NULL);

	GtkWidget  *window  = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	GtkWidget  *drawing = GTK_WIDGET(gtk_builder_get_object(builder, "drawing"));
	GtkWidget  *tabs    = GTK_WIDGET(gtk_builder_get_object(builder, "tabs"));
	if (window  == NULL) g_error("window  is null");
	if (drawing == NULL) g_error("drawing is null");
	if (tabs    == NULL) g_error("tabs    is null");

	/* Set up darwing area */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_assert_not_reached();
	if (!gtk_widget_set_gl_capability(drawing, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_assert_not_reached();

	/* Load plugins */
	opengl_init (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));
	radar_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));
	ridge_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));
	example_init(GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
