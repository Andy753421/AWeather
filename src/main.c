#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

#include "aweather-gui.h"
#include "location.h"
#include "opengl.h"
#include "plugin-radar.h"
#include "plugin-ridge.h"
#include "plugin-example.h"

/*****************
 * Setup helpers *
 *****************/
static void combo_sensitive(GtkCellLayout *cell_layout, GtkCellRenderer *cell,
		GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	gboolean sensitive = !gtk_tree_model_iter_has_child(tree_model, iter);
	g_object_set(cell, "sensitive", sensitive, NULL);
}

static void site_setup(AWeatherGui *gui)
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

	GtkBuilder      *builder  = aweather_gui_get_builder(gui);
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

static void time_setup(AWeatherGui *gui)
{
	GtkBuilder        *builder = aweather_gui_get_builder(gui);
	GtkWidget         *view    = GTK_WIDGET(gtk_builder_get_object(builder, "time"));
	GtkCellRenderer   *rend    = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col     = gtk_tree_view_column_new_with_attributes(
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

	/* Set up AWeather */
	AWeatherGui *gui = aweather_gui_new();

	/* Load components */
	site_setup(gui);
	time_setup(gui);
	opengl_init(gui);

	/* Load plugins */
	radar_init  (gui);
        ridge_init  (gui);
	example_init(gui);

	gtk_widget_show_all(GTK_WIDGET(aweather_gui_get_window(gui)));
	gtk_main();

	return 0;
}
