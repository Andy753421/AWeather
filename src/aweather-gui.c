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
		gis_viewer_refresh(self->viewer);
	else if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) {
		GtkNotebook *tabs = GTK_NOTEBOOK(aweather_gui_get_widget(self, "main_tabs"));
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
	gis_viewer_zoom(self->viewer, 3./4);
}

void on_zoomout(GtkAction *action, AWeatherGui *self)
{
	gis_viewer_zoom(self->viewer, 4./3);
}

void on_refresh(GtkAction *action, AWeatherGui *self)
{
	gis_viewer_refresh(self->viewer);
}

void on_plugin_toggled(GtkCellRendererToggle *cell, gchar *path_str, AWeatherGui *self)
{
	GtkWidget    *tview = aweather_gui_get_widget(self, "prefs_plugins_view");
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

void on_time_changed(GtkTreeView *view, GtkTreePath *path,
		GtkTreeViewColumn *column, AWeatherGui *self)
{
	gchar *time;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 0, &time, -1);
	gis_viewer_set_time(self->viewer, time);
	g_free(time);
}

void on_site_changed(GtkComboBox *combo, AWeatherGui *self)
{
	gchar *site;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(model, &iter, 1, &site, -1);
	city_t *city;
	for (city = cities; city->type; city++)
		if (city->code && g_str_equal(city->code, site)) {
			gis_viewer_set_location(self->viewer,
					city->lat, city->lon, EARTH_R/20);
			gis_viewer_set_rotation(self->viewer, 0, 0, 0);
			break;
		}
	g_free(site);
}

static gboolean gtk_tree_model_find_string(GtkTreeModel *model,
		GtkTreeIter *rval, GtkTreeIter *parent, gint field, const gchar *key)
{
	g_debug("AWeatherGui: gtk_tree_model_find - field=%d key=%s", field, key);
	char *text;
	GtkTreeIter cur;
	gint num_children = gtk_tree_model_iter_n_children(model, parent);
	for (int i = 0; i < num_children; i++) {
		gtk_tree_model_iter_nth_child(model, &cur, parent, i);
		gtk_tree_model_get(model, &cur, field, &text, -1);
		if (text != NULL && g_str_equal(text, key)) {
			*rval = cur;
			g_free(text);
			return TRUE;
		}
		g_free(text);
		if (gtk_tree_model_iter_has_child(model, &cur))
			if (gtk_tree_model_find_string(model, rval, &cur, field, key))
				return TRUE;
	}
	return FALSE;
}

static void update_time_widget(GisViewer *viewer, const char *time, AWeatherGui *self)
{
	g_debug("AWeatherGui: update_time_widget - time=%s", time);
	GtkTreeView  *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "main_time"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(tview));
	GtkTreeIter iter;
	if (gtk_tree_model_find_string(model, &iter, NULL, 0, time)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		g_signal_handlers_block_by_func(tview,
				G_CALLBACK(on_site_changed), self);
		gtk_tree_view_set_cursor(tview, path, NULL, FALSE);
		g_signal_handlers_unblock_by_func(tview,
				G_CALLBACK(on_site_changed), self);
		gtk_tree_path_free(path);
	}
}

static void update_site_widget(GisViewer *viewer, char *site, AWeatherGui *self)
{
	g_debug("AWeatherGui: update_site_widget - site=%s", site);
	GtkComboBox  *combo = GTK_COMBO_BOX(aweather_gui_get_widget(self, "main_site"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_combo_box_get_model(combo));
	GtkTreeIter iter;
	if (gtk_tree_model_find_string(model, &iter, NULL, 1, site)) {
		g_signal_handlers_block_by_func(combo,
				G_CALLBACK(on_site_changed), self);
		gtk_combo_box_set_active_iter(combo, &iter);
		g_signal_handlers_unblock_by_func(combo,
				G_CALLBACK(on_site_changed), self);
	}
}

/* Prefs callbacks */
void on_offline(GtkToggleAction *action, AWeatherGui *self)
{
	gboolean value = gtk_toggle_action_get_active(action);
	g_debug("AWeatherGui: on_offline - offline=%d", value);
	gis_prefs_set_boolean(self->prefs, "gis/offline", value);
	gis_viewer_set_offline(self->viewer, value);
}

void on_initial_site_changed(GtkComboBox *combo, AWeatherGui *self)
{
	gchar *site;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(model, &iter, 1, &site, -1);
	g_debug("AWeatherGui: on_initial_site_changed - site=%s", site);
	gis_prefs_set_string(self->prefs, "aweather/initial_site", site);
	g_free(site);
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


/*****************
 * Setup helpers *
 *****************/
static void prefs_setup(AWeatherGui *self)
{
	/* Set values */
	gchar *nu = gis_prefs_get_string (self->prefs, "aweather/nexrad_url",   NULL);
	gint   ll = gis_prefs_get_integer(self->prefs, "aweather/log_level",    NULL);
	gchar *is = gis_prefs_get_string (self->prefs, "aweather/initial_site", NULL);
	GtkWidget *nuw = aweather_gui_get_widget(self, "prefs_general_url");
	GtkWidget *llw = aweather_gui_get_widget(self, "prefs_general_log");
	GtkWidget *isw = aweather_gui_get_widget(self, "prefs_general_site");
	if (nu) gtk_entry_set_text(GTK_ENTRY(nuw), nu), g_free(nu);
	if (ll) gtk_spin_button_set_value(GTK_SPIN_BUTTON(llw), ll);
	if (is) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(isw));
		GtkTreeIter iter;
		if (gtk_tree_model_find_string(model, &iter, NULL, 1, is))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(isw), &iter);
	}

	/* Load Plugins */
	GtkTreeView       *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "prefs_plugins_view"));
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

	GtkWidget *combo    = aweather_gui_get_widget(self, "main_site");
	GObject   *renderer = aweather_gui_get_object(self, "main_site_rend");
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo),
			GTK_CELL_RENDERER(renderer), combo_sensitive, NULL, NULL);
}

static void time_setup(AWeatherGui *self)
{
	GtkTreeView       *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "main_time"));
	GtkCellRenderer   *rend  = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col   = gtk_tree_view_column_new_with_attributes(
					"Time", rend, "text", 0, NULL);

	gtk_tree_view_append_column(tview, col);
	g_object_set(rend, "size-points", 8.0, NULL);

	g_signal_connect(self->viewer, "time-changed",
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

	GtkTreeView  *tview  = GTK_TREE_VIEW(aweather_gui_get_widget(self, "main_time"));
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

	gis_viewer_set_time(self->viewer, last_time);

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
static void update_times(AWeatherGui *self, GisViewer *viewer, char *site)
{
	g_debug("AWeatherGui: update_times - site=%s", site);
	if (gis_viewer_get_offline(self->viewer)) {
		GList *times = NULL;
		gchar *path = g_build_filename(g_get_user_cache_dir(),
				"libgis", "nexrd2", "raw", site, NULL);
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
		char *base = gis_prefs_get_string(self->prefs, "aweather/nexrad_url", NULL);
		cache_file(base, path, GIS_REFRESH, NULL, update_times_online_cb, self);
		/* update_times_gtk from update_times_online_cb */
	}
}
/* FIXME: This is redundent with the radar plugin
 *        Make a shared wsr88d file? */
static void on_gis_location_changed(GisViewer *viewer,
		gdouble lat, gdouble lon, gdouble elev,
		gpointer _self)
{
	AWeatherGui *self = AWEATHER_GUI(_self);
	gdouble min_dist = EARTH_R / 5;
	city_t *city, *min_city = NULL;
	for (city = cities; city->type; city++) {
		if (city->type != LOCATION_CITY)
			continue;
		gdouble city_loc[3] = {};
		gdouble eye_loc[3]  = {lat, lon, elev};
		lle2xyz(city->lat, city->lon, city->elev,
				&city_loc[0], &city_loc[1], &city_loc[2]);
		lle2xyz(lat, lon, elev,
				&eye_loc[0], &eye_loc[1], &eye_loc[2]);
		gdouble dist = distd(city_loc, eye_loc);
		if (dist < min_dist) {
			min_dist = dist;
			min_city = city;
		}
	}
	static city_t *last_city = NULL;
	if (min_city && min_city != last_city) {
		update_site_widget(self->viewer, min_city->code, self);
		update_times(self, viewer, min_city->code);
	}
	last_city = min_city;
}
static void on_gis_refresh(GisViewer *viewer, gpointer _self)
{
	AWeatherGui *self = AWEATHER_GUI(_self);
	gdouble lat, lon, elev;
	gis_viewer_get_location(self->viewer, &lat, &lon, &elev);
	/* Hack to update times */
	on_gis_location_changed(self->viewer, lat, lon, elev, self);
}


/***********
 * Methods *
 ***********/
AWeatherGui *aweather_gui_new()
{
	g_debug("AWeatherGui: new");
	return g_object_new(AWEATHER_TYPE_GUI, NULL);
}
GisViewer *aweather_gui_get_viewer(AWeatherGui *self)
{
	g_assert(AWEATHER_IS_GUI(self));
	return self->viewer;
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
			self->viewer, self->prefs);
	GtkWidget *body = gis_plugin_get_config(plugin);
	if (body) {
		GtkWidget *config = aweather_gui_get_widget(self, "main_tabs");
		GtkWidget *tab    = gtk_label_new(name);
		gtk_notebook_append_page(GTK_NOTEBOOK(config), body, tab);
		gtk_widget_show_all(config);
	}
	gtk_widget_queue_draw(GTK_WIDGET(self->viewer));
}
void aweather_gui_deattach_plugin(AWeatherGui *self, const gchar *name)
{
	GtkWidget *config = aweather_gui_get_widget(self, "main_tabs");
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
	gtk_widget_queue_draw(GTK_WIDGET(self->viewer));
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherGui, aweather_gui, GTK_TYPE_WINDOW);
static void aweather_gui_init(AWeatherGui *self)
{
	g_debug("AWeatherGui: init");

	/* Simple things */
	gchar *config   = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.ini", NULL);
	gchar *defaults = g_build_filename(PKGDATADIR, "defaults.ini", NULL);
	self->prefs   = gis_prefs_new(config, defaults);
	self->plugins = gis_plugins_new(PLUGINSDIR);
	self->viewer  = gis_opengl_new(self->plugins);
	g_free(config);
	g_free(defaults);

	/* Setup window */
	self->builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(self->builder, PKGDATADIR "/main.ui", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gtk_widget_reparent(aweather_gui_get_widget(self, "main_body"), GTK_WIDGET(self));
	GtkWidget *paned = aweather_gui_get_widget(self, "main_paned");
	gtk_widget_destroy(gtk_paned_get_child1(GTK_PANED(paned)));
	gtk_paned_pack1(GTK_PANED(paned), GTK_WIDGET(self->viewer), TRUE, FALSE);

	/* Plugins */
	GtkTreeIter iter;
	self->gtk_plugins = GTK_LIST_STORE(aweather_gui_get_object(self, "plugins"));
	for (GList *cur = gis_plugins_available(self->plugins); cur; cur = cur->next) {
		gchar *name = cur->data;
		GError *error = NULL;
		gboolean enabled = gis_prefs_get_boolean_v(self->prefs, cur->data, "enabled", &error);
		if (error && error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
			enabled = TRUE;
		gtk_list_store_append(self->gtk_plugins, &iter);
		gtk_list_store_set(self->gtk_plugins, &iter, 0, name, 1, enabled, -1);
		if (enabled)
			aweather_gui_attach_plugin(self, name);
	}

	/* Misc, helpers */
	site_setup(self);
	time_setup(self);
	prefs_setup(self);

	/* Connect signals */
	gtk_builder_connect_signals(self->builder, self);
	g_signal_connect(self, "key-press-event",
			G_CALLBACK(on_gui_key_press), self);
	g_signal_connect_swapped(self->viewer, "offline",
			G_CALLBACK(gtk_toggle_action_set_active),
			aweather_gui_get_object(self, "offline"));

	/* deprecated site stuff */
	g_signal_connect(self->viewer, "location-changed",
			G_CALLBACK(on_gis_location_changed), self);
	g_signal_connect(self->viewer, "refresh",
			G_CALLBACK(on_gis_refresh), self);
}
static GObject *aweather_gui_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("aweather_gui: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_gui_parent_class);
	return parent_class->constructor(gtype, n_properties, properties);
}
static void aweather_gui_dispose(GObject *_self)
{
	g_debug("AWeatherGui: dispose");
	AWeatherGui *self = AWEATHER_GUI(_self);
	if (self->builder) {
		/* Reparent to avoid double unrefs */
		GtkWidget *body   = aweather_gui_get_widget(self, "main_body");
		GtkWidget *window = aweather_gui_get_widget(self, "main_window");
		gtk_widget_reparent(body, window);
		g_object_unref(self->builder);
		self->builder = NULL;
	}
	if (self->viewer) {
		g_object_unref(self->viewer);
		self->viewer = NULL;
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
