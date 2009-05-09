#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include "location.h"
#include "opengl.h"
#include "plugin-radar.h"
#include "plugin-ridge.h"
#include "plugin-example.h"

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

/*****************
 * Setup helpers *
 *****************/
static void combo_sensitive(GtkCellLayout *cell_layout, GtkCellRenderer *cell,
		GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	gboolean sensitive = !gtk_tree_model_iter_has_child(tree_model, iter);
	g_object_set(cell, "sensitive", sensitive, NULL);
}

static void site_setup(GtkBuilder *builder)
{
	GtkTreeIter state, city;
	GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	for (int i = 0; cities[i].label; i++) {
		if (cities[i].type == LOCATION_STATE) {
			gtk_tree_store_append(store, &state, NULL);
			gtk_tree_store_set   (store, &state, 0, cities[i].label, 
					                     1, cities[i].code,  -1);
		} else {
			gtk_tree_store_append(store, &city, &state);
			gtk_tree_store_set   (store, &city, 0, cities[i].label, 
				                            1, cities[i].code,  -1);
		}
	}

	GtkWidget       *combo    = GTK_WIDGET(gtk_builder_get_object(builder, "site"));
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, NULL);
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(store));
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer,
			combo_sensitive, NULL, NULL);

	//g_signal_connect(G_OBJECT(loc_sel),  "changed", G_CALLBACK(loc_changed),  NULL);
	//gtk_box_pack_start(GTK_BOX(selectors), loc_sel,  FALSE, FALSE, 0);
}

static void time_setup(GtkBuilder *builder)
{
	GtkWidget         *view = GTK_WIDGET(gtk_builder_get_object(builder, "time"));
	GtkCellRenderer   *rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col  = gtk_tree_view_column_new_with_attributes(
					"Time", rend, 0, "text", NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
}

/********
 * Main *
 ********/
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

	/* Set up darwing area */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(drawing, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");

	/* Load components */
	site_setup(builder);
	time_setup(builder);
	opengl_init(GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));

	/* Load plugins */
	radar_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));
	ridge_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));
	example_init(GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tabs));

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
