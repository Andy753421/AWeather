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
#include <math.h>

#include "misc.h"
#include "aweather-gui.h"
#include "aweather-plugin.h"
#include "gis-opengl.h"
#include "location.h"

/* Needed prototpes */
gboolean on_gui_key_press(GtkWidget *widget, GdkEventKey *event, AWeatherGui *self);
static void site_setup(AWeatherGui *self);
static void time_setup(AWeatherGui *self);

/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherGui, aweather_gui, GTK_TYPE_WINDOW);
static void aweather_gui_init(AWeatherGui *self)
{
	g_debug("AWeatherGui: init");

	/* Setup window */
	self->builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(self->builder, DATADIR "/aweather/main.ui", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gtk_widget_reparent(aweather_gui_get_widget(self, "body"), GTK_WIDGET(self));

	/* GIS things */
	GtkWidget *drawing = aweather_gui_get_widget(self, "drawing");
	self->world   = gis_world_new();
	self->view    = gis_view_new();
	self->opengl  = gis_opengl_new(self->world, self->view, GTK_DRAWING_AREA(drawing));

	/* Plugins */
	self->plugins     = NULL;
	// TODO: set/load these with prefereces
	aweather_gui_load_plugin(self, "example");
	//aweather_gui_load_plugin(self, "radar");
	//aweather_gui_load_plugin(self, "ridge");
	self->gtk_plugins = GTK_LIST_STORE(aweather_gui_get_object(self, "plugins"));
	GDir *dir = g_dir_open(PLUGINDIR, 0, NULL);
	const gchar *name;
	GtkTreeIter iter;
	while ((name = g_dir_read_name(dir))) {
		if (g_pattern_match_simple("*.so", name)) {
			gtk_list_store_append(self->gtk_plugins, &iter);
			gtk_list_store_set(self->gtk_plugins, &iter, 0, name, 1, FALSE, -1);
		}
	}

	/* Misc, helpers */
	site_setup(self);
	time_setup(self);

	/* Connect signals */
	gtk_builder_connect_signals(self->builder, self);
	g_signal_connect(self, "key-press-event",
			G_CALLBACK(on_gui_key_press), self);
	g_signal_connect_swapped(self->world, "offline",
			G_CALLBACK(gtk_toggle_action_set_active),
			aweather_gui_get_object(self, "offline"));

	/* Preferences */
	gchar *filename = g_build_filename(g_get_user_config_dir(),
			"aweather", "config.ini", NULL);
	self->prefs = g_key_file_new();
	g_key_file_load_from_file(self->prefs, filename, G_KEY_FILE_KEEP_COMMENTS, &error);
}
static GObject *aweather_gui_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("aweather_gui: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_gui_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}
static void aweather_gui_dispose(GObject *_self)
{
	g_debug("AWeatherGui: dispose");
	AWeatherGui *self = AWEATHER_GUI(_self);
	if (self->builder) {
		/* Reparent to avoid double unrefs */
		GtkWidget *body   = aweather_gui_get_widget(self, "body");
		GtkWidget *window = aweather_gui_get_widget(self, "main_window");
		gtk_widget_reparent(body, window);
		g_object_unref(self->builder);
		self->builder = NULL;
	}
	if (self->world) {
		g_object_unref(self->world);
		self->world = NULL;
	}
	if (self->view) {
		g_object_unref(self->view);
		self->view = NULL;
	}
	if (self->opengl) {
		g_object_unref(self->opengl);
		self->opengl = NULL;
	}
	if (self->plugins) {
		g_list_foreach(self->plugins, (GFunc)g_object_unref, NULL);
		g_list_free(self->plugins);
		self->plugins = NULL;
	}
	if (self->prefs) {
		gchar *filename = g_build_filename(g_get_user_config_dir(),
				"aweather", "config.ini", NULL);
		gsize length;
		g_mkdir_with_parents(g_path_get_dirname(filename), 0755);
		gchar *data = g_key_file_to_data(self->prefs, &length, NULL);
		g_file_set_contents(filename, data, length, NULL);
		g_key_file_free(self->prefs);
		self->prefs = NULL;
	}
	G_OBJECT_CLASS(aweather_gui_parent_class)->dispose(_self);
}
static void aweather_gui_finalize(GObject *_self)
{
	g_debug("AWeatherGui: finalize");
	G_OBJECT_CLASS(aweather_gui_parent_class)->finalize(_self);
	gtk_main_quit();

}
static void aweather_gui_class_init(AWeatherGuiClass *klass)
{
	g_debug("AWeatherGui: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = aweather_gui_constructor;
	gobject_class->dispose      = aweather_gui_dispose;
	gobject_class->finalize     = aweather_gui_finalize;
}

/*************
 * Callbacks *
 *************/
gboolean on_gui_key_press(GtkWidget *widget, GdkEventKey *event, AWeatherGui *self)
{
	g_debug("AWeatherGui: on_gui_key_press - key=%x, state=%x",
			event->keyval, event->state);
	if (event->keyval == GDK_q)
		gtk_widget_destroy(GTK_WIDGET(self));
	else if (event->keyval == GDK_r && event->state & GDK_CONTROL_MASK)
		gis_world_refresh(self->world);
	else if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) {
		GtkNotebook *tabs = GTK_NOTEBOOK(aweather_gui_get_widget(self, "tabs"));
		gint num_tabs = gtk_notebook_get_n_pages(tabs);
		gint cur_tab  = gtk_notebook_get_current_page(tabs);
		if (event->state & GDK_SHIFT_MASK)
			gtk_notebook_set_current_page(tabs, (cur_tab-1)%num_tabs);
		else 
			gtk_notebook_set_current_page(tabs, (cur_tab+1)%num_tabs);
	};
	return FALSE;
}

void on_quit(GtkMenuItem *menu, AWeatherGui *self)
{
	gtk_widget_destroy(GTK_WIDGET(self));
}

void on_zoomin(GtkAction *action, AWeatherGui *self)
{
	gis_view_zoom(self->view, 3./4);
}

void on_zoomout(GtkAction *action, AWeatherGui *self)
{
	gis_view_zoom(self->view, 4./3);
}

void on_refresh(GtkAction *action, AWeatherGui *self)
{
	gis_world_refresh(self->world);
}

void load_window(gchar *name, AWeatherGui *self)
{
	// TODO: use gtk_widget_hide_on_delete()
	char *objects[2] = {name, NULL};
	gtk_builder_add_objects_from_file(self->builder,
			DATADIR "/aweather/main.ui", objects, NULL);
	gtk_builder_connect_signals(self->builder, self);
	GtkWidget *main_win = aweather_gui_get_widget(self, "main_window");
	GtkWidget *this_win = aweather_gui_get_widget(self, name);
	gtk_window_set_transient_for(GTK_WINDOW(this_win), GTK_WINDOW(main_win));
	gtk_widget_show_all(this_win);
}

void on_about(GtkAction *action, AWeatherGui *self)
{
	load_window("about_window", self);
}

void on_plugin_toggled(GtkCellRendererToggle *cell, gchar *path_str, AWeatherGui *self)
{
	GtkWidget    *tview = aweather_gui_get_widget(self, "plugins_view");
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tview));
	g_message("model=%p", model);
	gboolean state;
	gchar *so;
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(model, &iter, path_str);
	gtk_tree_model_get(model, &iter, 0, &so, 1, &state, -1);
	state = !state;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, state, -1);
	g_strdelimit(so, ".", '\0');
	if (state)
		aweather_gui_load_plugin(self, so);
	else
		aweather_gui_unload_plugin(self, so);
	g_free(so);
	gtk_widget_show_all(aweather_gui_get_widget(self, "tabs"));
}
void on_prefs(GtkAction *action, AWeatherGui *self)
{
	/* get prefs */
	gchar *is = g_key_file_get_string(self->prefs,  "general", "initial_site", NULL);
	gchar *nu = g_key_file_get_string(self->prefs,  "general", "nexrad_url",   NULL);
	gint   ll = g_key_file_get_integer(self->prefs, "general", "log_level",    NULL);
	load_window("prefs_window", self);
	/* set prefs */
	GtkWidget *isw = aweather_gui_get_widget(self, "initial_site");
	GtkWidget *nuw = aweather_gui_get_widget(self, "nexrad_url");
	GtkWidget *llw = aweather_gui_get_widget(self, "log_level");
	if (is) gtk_entry_set_text(GTK_ENTRY(isw), is), g_free(is);
	if (nu) gtk_entry_set_text(GTK_ENTRY(nuw), nu), g_free(nu);
	if (ll) gtk_spin_button_set_value(GTK_SPIN_BUTTON(llw), ll);

	/* Plugins */
	GtkTreeView       *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "plugins_view"));
	GtkCellRenderer   *rend1 = gtk_cell_renderer_text_new();
	GtkCellRenderer   *rend2 = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn *col1  = gtk_tree_view_column_new_with_attributes(
			"Plugin",  rend1, "text",   0, NULL);
	GtkTreeViewColumn *col2  = gtk_tree_view_column_new_with_attributes(
			"Enabled", rend2, "active", 1, NULL);
	gtk_tree_view_append_column(tview, col1);
	gtk_tree_view_append_column(tview, col2);
	g_signal_connect(rend2, "toggled", G_CALLBACK(on_plugin_toggled), self);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tview), GTK_TREE_MODEL(self->gtk_plugins));
}

void on_time_changed(GtkTreeView *view, GtkTreePath *path,
		GtkTreeViewColumn *column, AWeatherGui *self)
{
	gchar *time;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 0, &time, -1);
	gis_view_set_time(self->view, time);
	g_free(time);
}

void on_site_changed(GtkComboBox *combo, AWeatherGui *self)
{
	gchar *site;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(model, &iter, 1, &site, -1);
	gis_view_set_site(self->view, site);
	g_free(site);
}

/* TODO: replace the code in these with `gtk_tree_model_find' utility */
static void update_time_widget(GisView *view, const char *time, AWeatherGui *self)
{
	g_debug("AWeatherGui: update_time_widget - time=%s", time);
	GtkTreeView  *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "time"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(tview));
	for (int i = 0; i < gtk_tree_model_iter_n_children(model, NULL); i++) {
		char *text;
		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, i);
		gtk_tree_model_get(model, &iter, 0, &text, -1);
		if (g_str_equal(text, time)) {
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			g_signal_handlers_block_by_func(tview,
					G_CALLBACK(on_site_changed), self);
			gtk_tree_view_set_cursor(tview, path, NULL, FALSE);
			g_signal_handlers_unblock_by_func(tview,
					G_CALLBACK(on_site_changed), self);
			gtk_tree_path_free(path);
			g_free(text);
			return;
		}
		g_free(text);
	}
}
static void update_site_widget(GisView *view, char *site, AWeatherGui *self)
{
	g_debug("AWeatherGui: update_site_widget - site=%s", site);
	GtkComboBox  *combo = GTK_COMBO_BOX(aweather_gui_get_widget(self, "site"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_combo_box_get_model(combo));
	for (int i = 0; i < gtk_tree_model_iter_n_children(model, NULL); i++) {
		GtkTreeIter iter1;
		gtk_tree_model_iter_nth_child(model, &iter1, NULL, i);
		for (int i = 0; i < gtk_tree_model_iter_n_children(model, &iter1); i++) {
			GtkTreeIter iter2;
			gtk_tree_model_iter_nth_child(model, &iter2, &iter1, i);
			char *text;
			gtk_tree_model_get(model, &iter2, 1, &text, -1);
			if (text == NULL)
				continue;
			if (g_str_equal(text, site)) {
				g_signal_handlers_block_by_func(combo,
						G_CALLBACK(on_site_changed), self);
				gtk_combo_box_set_active_iter(combo, &iter2);
				g_signal_handlers_unblock_by_func(combo,
						G_CALLBACK(on_site_changed), self);
				g_free(text);
				return;
			}
			g_free(text);
		}
	}
}
/* Prefs callbacks */
void on_offline(GtkToggleAction *action, AWeatherGui *self)
{
	gboolean value = gtk_toggle_action_get_active(action);
	g_debug("AWeatherGui: on_offline - offline=%d", value);
	g_key_file_set_boolean(self->prefs, "general", "offline", value);
	gis_world_set_offline(self->world, value);
}
void on_initial_site_changed(GtkEntry *entry, AWeatherGui *self)
{
	const gchar *text = gtk_entry_get_text(entry);
	g_debug("AWeatherGui: on_initial_site_changed - site=%s", text);
	g_key_file_set_string(self->prefs, "general", "initial_site", text);
}
void on_nexrad_url_changed(GtkEntry *entry, AWeatherGui *self)
{
	const gchar *text = gtk_entry_get_text(entry);
	g_debug("AWeatherGui: on_nexrad_url_changed - url=%s", text);
	g_key_file_set_string(self->prefs, "general", "nexrad_url", text);
}
int on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	gint value = gtk_spin_button_get_value_as_int(spinner);
	g_debug("AWeatherGui: on_log_level_changed - %p, level=%d", self, value);
	g_key_file_set_integer(self->prefs, "general", "log_level", value);
	g_debug("test");
	return TRUE;
}
// plugins

/*****************
 * Setup helpers *
 *****************/
static void combo_sensitive(GtkCellLayout *cell_layout, GtkCellRenderer *cell,
		GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	gboolean sensitive = !gtk_tree_model_iter_has_child(tree_model, iter);
	g_object_set(cell, "sensitive", sensitive, NULL);
}

static void site_setup(AWeatherGui *self)
{
	GtkTreeIter state, city;
	GtkTreeStore *store = GTK_TREE_STORE(aweather_gui_get_object(self, "sites"));
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

	GtkWidget *combo    = aweather_gui_get_widget(self, "site");
	GObject   *renderer = aweather_gui_get_object(self, "site_rend");
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo),
			GTK_CELL_RENDERER(renderer), combo_sensitive, NULL, NULL);

	g_signal_connect(self->view, "site-changed",
			G_CALLBACK(update_site_widget), self);
}

static void time_setup(AWeatherGui *self)
{
	GtkTreeView       *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "time"));
	GtkCellRenderer   *rend  = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col   = gtk_tree_view_column_new_with_attributes(
					"Time", rend, "text", 0, NULL);

	gtk_tree_view_append_column(tview, col);
	g_object_set(rend, "size-points", 8.0, NULL);

	g_signal_connect(self->view, "time-changed",
			G_CALLBACK(update_time_widget), self);
}


/***********
 * Methods *
 ***********/
AWeatherGui *aweather_gui_new()
{
	g_debug("AWeatherGui: new");
	return g_object_new(AWEATHER_TYPE_GUI, NULL);
}
GisWorld *aweather_gui_get_world(AWeatherGui *self)
{
	g_assert(AWEATHER_IS_GUI(self));
	return self->world;
}
GisView *aweather_gui_get_view(AWeatherGui *self)
{
	g_assert(AWEATHER_IS_GUI(self));
	return self->view;
}
GisOpenGL *aweather_gui_get_opengl(AWeatherGui *self)
{
	g_assert(AWEATHER_IS_GUI(self));
	return self->opengl;
}
GtkBuilder *aweather_gui_get_builder(AWeatherGui *self)
{
	g_debug("AWeatherGui: get_builder");
	g_assert(AWEATHER_IS_GUI(self));
	return self->builder;
}
GtkWidget *aweather_gui_get_widget(AWeatherGui *self, const gchar *name)
{
	g_debug("AWeatherGui: get_widget - name=%s", name);
	g_assert(AWEATHER_IS_GUI(self));
	GObject *widget = gtk_builder_get_object(self->builder, name);
	if (!GTK_IS_WIDGET(widget))
		g_error("Failed to get widget `%s'", name);
	return GTK_WIDGET(widget);
}
GObject *aweather_gui_get_object(AWeatherGui *self, const gchar *name)
{
	g_debug("AWeatherGui: get_widget - name=%s", name);
	g_assert(AWEATHER_IS_GUI(self));
	return gtk_builder_get_object(self->builder, name);
}
gboolean aweather_gui_load_plugin(AWeatherGui *self, const char *name)
{
	gchar *path = g_strdup_printf("%s/%s.%s", PLUGINDIR, name, G_MODULE_SUFFIX);
	GModule *module = g_module_open(path, 0);
	g_free(path);
	if (module == NULL) {
		g_warning("Unable to load module %s: %s", name, g_module_error());
		return FALSE;
	}

	AWeatherPlugin *(*constructor)();
	gchar *constructor_str = g_strconcat("aweather_", name, "_new", NULL);
	if (!g_module_symbol(module, constructor_str, (gpointer*)&constructor)) {
		g_warning("Unable to load symbol %s from %s: %s",
				constructor_str, name, g_module_error());
		g_module_close(module);
		g_free(constructor_str);
		return FALSE;
	}
	g_free(constructor_str);

	self->plugins = g_list_append(self->plugins, constructor(self));
	self->opengl->plugins = self->plugins;
	return TRUE;
}
gboolean aweather_gui_unload_plugin(AWeatherGui *self, const char *name)
{
	return FALSE;
}
