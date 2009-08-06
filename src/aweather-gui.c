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

#include <gis/gis.h>

#include "aweather-gui.h"
#include "aweather-location.h"

/* Needed prototpes */
gboolean on_gui_key_press(GtkWidget *widget, GdkEventKey *event, AWeatherGui *self);
static void on_gis_refresh(GisWorld *world, gpointer _self);
static void on_gis_site_changed(GisView *view, char *site, gpointer _self);
static void prefs_setup(AWeatherGui *self);
static void site_setup(AWeatherGui *self);
static void time_setup(AWeatherGui *self);

/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherGui, aweather_gui, GTK_TYPE_WINDOW);
static void aweather_gui_init(AWeatherGui *self)
{
	g_debug("AWeatherGui: init");

	/* Simple things */
	self->prefs   = gis_prefs_new("aweather");
	self->plugins = gis_plugins_new();
	self->world   = gis_world_new();
	self->view    = gis_view_new();

	/* Setup window */
	self->builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(self->builder, DATADIR "/aweather/main.ui", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gtk_widget_reparent(aweather_gui_get_widget(self, "body"), GTK_WIDGET(self));

	/* GIS things */
	GtkWidget *drawing = aweather_gui_get_widget(self, "drawing");
	self->opengl = gis_opengl_new(self->world, self->view, GTK_DRAWING_AREA(drawing));
	self->opengl->plugins = self->plugins;
	//gtk_widget_show_all(GTK_WIDGET(self));

	/* Plugins */
	GtkTreeIter iter;
	self->gtk_plugins = GTK_LIST_STORE(aweather_gui_get_object(self, "plugins"));
	for (GList *cur = gis_plugins_available(); cur; cur = cur->next) {
		gchar *name = cur->data;
		gboolean enabled = gis_prefs_get_boolean_v(self->prefs, cur->data, "enabled");
		gtk_list_store_append(self->gtk_plugins, &iter);
		gtk_list_store_set(self->gtk_plugins, &iter, 0, name, 1, enabled, -1);
		if (enabled)
			aweather_gui_attach_plugin(self, name);
	}

	/* Misc, helpers */
	prefs_setup(self);
	site_setup(self);
	time_setup(self);

	/* Connect signals */
	gtk_builder_connect_signals(self->builder, self);
	g_signal_connect(self, "key-press-event",
			G_CALLBACK(on_gui_key_press), self);
	g_signal_connect_swapped(self->world, "offline",
			G_CALLBACK(gtk_toggle_action_set_active),
			aweather_gui_get_object(self, "offline"));

	/* deprecated site stuff */
	g_signal_connect(self->view,  "site-changed", G_CALLBACK(on_gis_site_changed), self);
	g_signal_connect(self->world, "refresh",      G_CALLBACK(on_gis_refresh),      self);
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
		gis_plugins_free(self->plugins);
		self->plugins = NULL;
	}
	if (self->prefs) {
		g_object_unref(self->prefs);
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


void on_plugin_toggled(GtkCellRendererToggle *cell, gchar *path_str, AWeatherGui *self)
{
	GtkWidget    *tview = aweather_gui_get_widget(self, "plugins_view");
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tview));
	gboolean state;
	gchar *name;
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(model, &iter, path_str);
	gtk_tree_model_get(model, &iter, 0, &name, 1, &state, -1);
	state = !state;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, state, -1);
	if (state)
		aweather_gui_attach_plugin(self, name);
	else
		aweather_gui_deattach_plugin(self, name);
	gis_prefs_set_boolean_v(self->prefs, name, "enabled", state);
	g_free(name);
}
void on_prefs(GtkAction *action, AWeatherGui *self)
{
	gtk_window_present(GTK_WINDOW(aweather_gui_get_widget(self, "prefs_window")));
}

void on_about(GtkAction *action, AWeatherGui *self)
{
	gtk_window_present(GTK_WINDOW(aweather_gui_get_widget(self, "about_window")));
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
	gis_prefs_set_boolean(self->prefs, "gis/offline", value);
	gis_world_set_offline(self->world, value);
}
void on_initial_site_changed(GtkEntry *entry, AWeatherGui *self)
{
	const gchar *text = gtk_entry_get_text(entry);
	g_debug("AWeatherGui: on_initial_site_changed - site=%s", text);
	gis_prefs_set_string(self->prefs, "aweather/initial_site", text);
}
void on_nexrad_url_changed(GtkEntry *entry, AWeatherGui *self)
{
	const gchar *text = gtk_entry_get_text(entry);
	g_debug("AWeatherGui: on_nexrad_url_changed - url=%s", text);
	gis_prefs_set_string(self->prefs, "aweather/nexrad_url", text);
}
int on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	gint value = gtk_spin_button_get_value_as_int(spinner);
	g_debug("AWeatherGui: on_log_level_changed - %p, level=%d", self, value);
	gis_prefs_set_integer(self->prefs, "aweather/log_level", value);
	return TRUE;
}
// plugins

/*****************
 * Setup helpers *
 *****************/
static void prefs_setup(AWeatherGui *self)
{
	/* Set values */
	gchar *is = gis_prefs_get_string (self->prefs, "aweather/initial_site");
	gchar *nu = gis_prefs_get_string (self->prefs, "aweather/nexrad_url");
	gint   ll = gis_prefs_get_integer(self->prefs, "aweather/log_level");
	GtkWidget *isw = aweather_gui_get_widget(self, "initial_site");
	GtkWidget *nuw = aweather_gui_get_widget(self, "nexrad_url");
	GtkWidget *llw = aweather_gui_get_widget(self, "log_level");
	if (is) gtk_entry_set_text(GTK_ENTRY(isw), is), g_free(is);
	if (nu) gtk_entry_set_text(GTK_ENTRY(nuw), nu), g_free(nu);
	if (ll) gtk_spin_button_set_value(GTK_SPIN_BUTTON(llw), ll);

	/* Load Plugins */
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


/*************************
 * Deprecated site stuff *
 *************************/
/* TODO: These update times functions are getting ugly... */
static void update_times_gtk(AWeatherGui *self, GList *times)
{
	gchar *last_time = NULL;
	GRegex *regex = g_regex_new("^[A-Z]{4}_([0-9]{8}_[0-9]{4})$", 0, 0, NULL); // KLSX_20090622_2113
	GMatchInfo *info;

	GtkTreeView  *tview  = GTK_TREE_VIEW(aweather_gui_get_widget(self, "time"));
	GtkListStore *lstore = GTK_LIST_STORE(gtk_tree_view_get_model(tview));
	gtk_list_store_clear(lstore);
	GtkTreeIter iter;
	times = g_list_reverse(times);
	for (GList *cur = times; cur; cur = cur->next) {
		if (g_regex_match(regex, cur->data, 0, &info)) {
			gchar *time = g_match_info_fetch(info, 1);
			gtk_list_store_insert(lstore, &iter, 0);
			gtk_list_store_set(lstore, &iter, 0, time, -1);
			last_time = time;
		}
	}

	gis_view_set_time(self->view, last_time);

	g_regex_unref(regex);
	g_list_foreach(times, (GFunc)g_free, NULL);
	g_list_free(times);
}
static void update_times_online_cb(char *path, gboolean updated, gpointer _self)
{
	GList *times = NULL;
	gchar *data;
	gsize length;
	g_file_get_contents(path, &data, &length, NULL);
	gchar **lines = g_strsplit(data, "\n", -1);
	for (int i = 0; lines[i] && lines[i][0]; i++) {
		char **parts = g_strsplit(lines[i], " ", 2);
		times = g_list_prepend(times, g_strdup(parts[1]));
		g_strfreev(parts);
	}
	g_strfreev(lines);
	g_free(data);

	update_times_gtk(_self, times);
}
static void update_times(AWeatherGui *self, GisView *view, char *site)
{
	g_debug("AWeatherGui: update_times - site=%s", site);
	if (gis_world_get_offline(self->world)) {
		GList *times = NULL;
		gchar *path = g_build_filename(g_get_user_cache_dir(), PACKAGE, "nexrd2", "raw", site, NULL);
		GDir *dir = g_dir_open(path, 0, NULL);
		if (dir) {
			const gchar *name;
			while ((name = g_dir_read_name(dir))) {
				times = g_list_prepend(times, g_strdup(name));
			}
			g_dir_close(dir);
		}
		g_free(path);
		update_times_gtk(self, times);
	} else {
		gchar *path = g_strdup_printf("nexrd2/raw/%s/dir.list", site);
		char *base = gis_prefs_get_string(self->prefs, "aweather/nexrad_url");
		cache_file(base, path, GIS_REFRESH, NULL, update_times_online_cb, self);
		/* update_times_gtk from update_times_online_cb */
	}
}
static void on_gis_site_changed(GisView *view, char *site, gpointer _self)
{
	AWeatherGui *self = AWEATHER_GUI(_self);
	g_debug("AWeatherGui: on_site_changed - Loading wsr88d list for %s", site);
	update_times(self, view, site);
}
static void on_gis_refresh(GisWorld *world, gpointer _self)
{
	AWeatherGui *self = AWEATHER_GUI(_self);
	char *site = gis_view_get_site(self->view);
	update_times(self, self->view, site);
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
void aweather_gui_attach_plugin(AWeatherGui *self, const gchar *name)
{
	GisPlugin *plugin = gis_plugins_load(self->plugins, name,
			self->world, self->view, self->opengl, self->prefs);
	GtkWidget *config = aweather_gui_get_widget(self, "tabs");
	GtkWidget *tab    = gtk_label_new(name);
	GtkWidget *body   = gis_plugin_get_config(plugin);
	gtk_notebook_append_page(GTK_NOTEBOOK(config), body, tab);
	gtk_widget_show_all(config);
}
void aweather_gui_deattach_plugin(AWeatherGui *self, const gchar *name)
{
	GtkWidget *config = aweather_gui_get_widget(self, "tabs");
	guint n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(config));
	for (int i = 0; i < n_pages; i++) {
		g_debug("testing tab %d", i);
		GtkWidget *body = gtk_notebook_get_nth_page(GTK_NOTEBOOK(config), i);
		if (!body) continue;
		GtkWidget *tab = gtk_notebook_get_tab_label(GTK_NOTEBOOK(config), body);
		if (!tab) continue;
		const gchar *tab_name = gtk_label_get_text(GTK_LABEL(tab));
		if (tab_name && g_str_equal(name, tab_name))
			gtk_notebook_remove_page(GTK_NOTEBOOK(config), i);
	}
	gis_plugins_unload(self->plugins, name);
}
