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

#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <math.h>

#include <grits.h>

#include "aweather-gui.h"
#include "aweather-location.h"

/* Autohide */
typedef struct {
	GtkWidget      *widget;
	GtkWidget      *window;
	GtkPositionType position;
	gulong          handler;
} GtkAutohideData;
static gboolean _gtk_widget_autohide_cb(GtkWidget *win,
		GdkEventMotion *event, GtkAutohideData *data)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(data->window, &alloc);
	if (data->position == GTK_POS_BOTTOM)
		g_debug("GtkWidget: autohide_cb - y=%lf, h=%d", event->y, alloc.height);
	gint position = -1;
	if (event->x < 20)                position = GTK_POS_LEFT;
	if (event->y < 20)                position = GTK_POS_TOP;
	if (event->x > alloc.width  - 20) position = GTK_POS_RIGHT;
	if (event->y > alloc.height - 20) position = GTK_POS_BOTTOM;
	if (position == data->position)
		gtk_widget_show(data->widget);
	else
		gtk_widget_hide(data->widget);
	return FALSE;
}
static void _gtk_widget_autohide(GtkWidget *widget, GtkWidget *viewer,
		GtkPositionType position)
{
	GtkAutohideData *data = g_new0(GtkAutohideData, 1);
	data->widget   = widget;
	data->position = position;
	data->window   = viewer;
	data->handler  = g_signal_connect(data->window, "motion-notify-event",
			G_CALLBACK(_gtk_widget_autohide_cb), data);
	g_object_set_data(G_OBJECT(widget), "autohidedata", data);
	gtk_widget_hide(widget);
}
static void _gtk_widget_autoshow(GtkWidget *widget)
{
	GtkAutohideData *data = g_object_get_data(G_OBJECT(widget), "autohidedata");
	g_signal_handler_disconnect(data->window, data->handler);
	g_free(data);
	gtk_widget_show(widget);
}

/*************
 * Callbacks *
 *************/
G_MODULE_EXPORT gboolean on_gui_key_press(GtkWidget *widget, GdkEventKey *event, AWeatherGui *self)
{
	g_debug("AWeatherGui: on_gui_key_press - key=%x, state=%x",
			event->keyval, event->state);
	GObject *action = aweather_gui_get_object(self, "fullscreen");
	gboolean full   = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if (event->keyval == GDK_KEY_q)
		gtk_widget_destroy(GTK_WIDGET(self));
	else if (event->keyval == GDK_KEY_F11 ||
	        (event->keyval == GDK_KEY_Escape && full))
		gtk_action_activate(GTK_ACTION(action));
	else if (event->keyval == GDK_KEY_r && event->state & GDK_CONTROL_MASK)
		grits_viewer_refresh(self->viewer);
	else if (event->keyval == GDK_KEY_Tab || event->keyval == GDK_KEY_ISO_Left_Tab) {
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

static void cleancache_r(gchar *path)
{
	GDir *dir = g_dir_open(path, 0, NULL);
	if (!dir)
		return;
	const gchar *child;
	while ((child = g_dir_read_name(dir))) {
		gchar *child_path = g_build_filename(path, child, NULL);
		if (g_file_test(child_path, G_FILE_TEST_IS_DIR)) {
			cleancache_r(child_path);
		} else {
			struct stat st;
			g_stat(child_path, &st);
			if (st.st_atime < time(NULL)-60*60*24)
				g_remove(child_path);
		}
		g_free(child_path);
	}
	g_dir_close(dir);
	g_rmdir(path);
}
G_MODULE_EXPORT void on_cleancache(GtkMenuItem *menu, AWeatherGui *self)
{
	g_debug("AWeatherGui: on_cleancache");
	/* Todo: move this to grits */
	gchar *cache = g_build_filename(g_get_user_cache_dir(), "grits", NULL);
	cleancache_r(cache);
}

G_MODULE_EXPORT void on_help(GtkMenuItem *menu, AWeatherGui *self)
{
	GError *err = NULL;
	const gchar *name = gtk_buildable_get_name(GTK_BUILDABLE(menu));
	gchar *page = g_str_has_suffix(name, "userguide") ? "userguide" :
	              g_str_has_suffix(name, "manpage")   ? "aweather"  : NULL;
	if (page == NULL) {
		g_warning("Unknown help page: %s", page);
		return;
	}
	gchar *path = g_strdup_printf("%s/%s.html", HTMLDIR, page);
	g_strdelimit(path, "/", G_DIR_SEPARATOR);
	gchar *argv[] = {"xdg-open", path, NULL};
	g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
	if (err) {
		g_warning("Unable to open help page: %s - %s",
				argv[1], err->message);
		g_error_free(err);
	}
	g_free(path);
}

void on_radar_changed(GtkMenuItem *menu, AWeatherGui *self)
{
	city_t *city = g_object_get_data(G_OBJECT(menu), "city");
	grits_viewer_set_location(self->viewer,
			city->pos.lat, city->pos.lon, EARTH_R/35);
	/* Focus radar tab */
	GtkWidget *config = aweather_gui_get_widget(self, "main_tabs");
	gint       npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(config));
	for (int i = 0; i < npages; i++) {
		GtkWidget   *child  = gtk_notebook_get_nth_page(GTK_NOTEBOOK(config), i);
		const gchar *plugin = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(config), child);
		if (g_str_equal(plugin, "radar"))
			gtk_notebook_set_current_page(GTK_NOTEBOOK(config), i);
	}
}

G_MODULE_EXPORT void on_quit(GtkMenuItem *menu, AWeatherGui *self)
{
	gtk_widget_destroy(GTK_WIDGET(self));
}

G_MODULE_EXPORT void on_zoomin(GtkAction *action, AWeatherGui *self)
{
	grits_viewer_zoom(self->viewer, 3./4);
}

G_MODULE_EXPORT void on_zoomout(GtkAction *action, AWeatherGui *self)
{
	grits_viewer_zoom(self->viewer, 4./3);
}

G_MODULE_EXPORT void on_fullscreen(GtkToggleAction *action, AWeatherGui *self)
{
	GtkWidget *menu    = aweather_gui_get_widget(self, "main_menu");
	GtkWidget *toolbar = aweather_gui_get_widget(self, "main_toolbar");
	GtkWidget *sidebar = aweather_gui_get_widget(self, "main_sidebar");
	GtkWidget *tabs    = aweather_gui_get_widget(self, "main_tabs");
	static gboolean menushow = TRUE; // Toolbar can be always disabled
	if (gtk_toggle_action_get_active(action)) {
		menushow = gtk_widget_get_visible(menu);
		gtk_window_fullscreen(GTK_WINDOW(self));
		gtk_widget_hide(menu);
		_gtk_widget_autohide(toolbar, GTK_WIDGET(self->viewer), GTK_POS_TOP);
		_gtk_widget_autohide(sidebar, GTK_WIDGET(self->viewer), GTK_POS_RIGHT);
		_gtk_widget_autohide(tabs,    GTK_WIDGET(self->viewer), GTK_POS_BOTTOM);
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(self));
		gtk_widget_set_visible(menu, menushow);
		_gtk_widget_autoshow(toolbar);
		_gtk_widget_autoshow(sidebar);
		_gtk_widget_autoshow(tabs);
	}
}

G_MODULE_EXPORT void on_refresh(GtkAction *action, AWeatherGui *self)
{
	grits_viewer_refresh(self->viewer);
}

static gboolean on_update_timeout(AWeatherGui *self)
{
	g_debug("AWeatherGui: on_update_timeout");
	grits_viewer_set_time(self->viewer, time(NULL));
	grits_viewer_refresh(self->viewer);
	return FALSE;
}
static void set_update_timeout(AWeatherGui *self)
{
	GObject *action = aweather_gui_get_object(self, "update");
	gint freq = grits_prefs_get_integer(self->prefs, "aweather/update_freq", NULL);
	if (self->update_source) {
		g_debug("AWeatherGui: set_update_timeout - clear");
		g_source_remove(self->update_source);
	}
	self->update_source = 0;
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)) && freq > 0) {
		g_debug("AWeatherGui: set_update_timeout - %d min", freq);
		self->update_source = g_timeout_add_seconds(freq*60,
				(GSourceFunc)on_update_timeout, self);
	}
}
G_MODULE_EXPORT void on_update(GtkToggleAction *action, AWeatherGui *self)
{
	grits_prefs_set_boolean(self->prefs, "aweather/update_enab",
			gtk_toggle_action_get_active(action));
	set_update_timeout(self);
}

G_MODULE_EXPORT void on_plugin_toggled(GtkCellRendererToggle *cell,
		gchar *path_str, AWeatherGui *self)
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
	g_free(name);
}

G_MODULE_EXPORT void on_time_changed(AWeatherGui *self)
{
	g_debug("AWeatherGui: on_time_changed");
	struct tm tm = {.tm_isdst = -1};

	GtkWidget *cal = aweather_gui_get_widget(self, "main_date_cal");
	gtk_calendar_get_date(GTK_CALENDAR(cal), (guint*)&tm.tm_year,
			(guint*)&tm.tm_mon, (guint*)&tm.tm_mday);
	tm.tm_year -= 1900;

	GtkTreePath *path = NULL;
	GtkWidget   *view = aweather_gui_get_widget(self, "main_time");
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(view), &path, NULL);
	if (path) {
		GtkTreeIter   iter;
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, 1, &tm.tm_hour, 2, &tm.tm_min, -1);
	}

	grits_viewer_set_time(self->viewer, mktime(&tm));
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

static void update_time_widget(GritsViewer *viewer, time_t time, AWeatherGui *self)
{
	g_debug("AWeatherGui: update_time_widget - time=%u", (guint)time);
	// FIXME
	//GtkTreeView  *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "main_time"));
	//GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(tview));
	//GtkTreeIter iter;
	//if (gtk_tree_model_find_string(model, &iter, NULL, 0, time)) {
	//	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
	//	gtk_tree_view_set_cursor(tview, path, NULL, FALSE);
	//	gtk_tree_path_free(path);
	//}
}

G_MODULE_EXPORT gboolean on_configure(AWeatherGui *self, GdkEventConfigure *config)
{
	g_debug("AWeatherGui: on_configure - %dx%d", config->width, config->height);
	grits_prefs_set_integer(self->prefs, "aweather/width",  config->width);
	grits_prefs_set_integer(self->prefs, "aweather/height", config->height);
	return FALSE;
}

/* Prefs callbacks */
G_MODULE_EXPORT void on_offline(GtkToggleAction *action, AWeatherGui *self)
{
	gboolean value = gtk_toggle_action_get_active(action);
	g_debug("AWeatherGui: on_offline - offline=%d", value);
	grits_viewer_set_offline(self->viewer, value);
}

G_MODULE_EXPORT int on_update_freq_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	gint value = gtk_spin_button_get_value_as_int(spinner);
	g_debug("AWeatherGui: on_update_freq_changed - %p, freq=%d", self, value);
	grits_prefs_set_integer(self->prefs, "aweather/update_freq", value);
	set_update_timeout(self);
	return TRUE;
}

G_MODULE_EXPORT void on_initial_site_changed(GtkComboBox *combo, AWeatherGui *self)
{
	gchar *code;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(model, &iter, 0, &code, -1);
	g_debug("AWeatherGui: on_initial_site_changed - code=%s", code);
	grits_prefs_set_string(self->prefs, "aweather/initial_site", code);
	g_free(code);
}

G_MODULE_EXPORT void on_nexrad_url_changed(GtkEntry *entry, AWeatherGui *self)
{
	const gchar *text = gtk_entry_get_text(entry);
	g_debug("AWeatherGui: on_nexrad_url_changed - url=%s", text);
	grits_prefs_set_string(self->prefs, "aweather/nexrad_url", text);
}

G_MODULE_EXPORT int on_log_level_changed(GtkSpinButton *spinner, AWeatherGui *self)
{
	gint value = gtk_spin_button_get_value_as_int(spinner);
	g_debug("AWeatherGui: on_log_level_changed - %p, level=%d", self, value);
	grits_prefs_set_integer(self->prefs, "aweather/log_level", value);
	return TRUE;
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
static void site_setup(AWeatherGui *self)
{
	GtkTreeIter state, city;
	GtkTreeStore *store = GTK_TREE_STORE(aweather_gui_get_object(self, "sites"));
	gtk_tree_store_append(store, &state, NULL);
	gtk_tree_store_set   (store, &state, 0, "", 1, "None", -1);
	for (int i = 0; cities[i].type; i++) {
		if (cities[i].type == LOCATION_STATE) {
			gtk_tree_store_append(store, &state, NULL);
			gtk_tree_store_set   (store, &state, 0, cities[i].code,
					                     1, cities[i].name, -1);
		} else if (cities[i].type == LOCATION_CITY) {
			gtk_tree_store_append(store, &city, &state);
			gtk_tree_store_set   (store, &city, 0, cities[i].code,
				                            1, cities[i].name, -1);
		}
	}

	GtkWidget *combo    = aweather_gui_get_widget(self, "prefs_general_site");
	GObject   *renderer = aweather_gui_get_object(self, "prefs_general_site_rend");
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo),
			GTK_CELL_RENDERER(renderer), combo_sensitive, NULL, NULL);
}

static void menu_setup(AWeatherGui *self)
{
	GtkWidget *menu = aweather_gui_get_widget(self, "main_menu_radar");
	GtkWidget *states=NULL, *state=NULL, *sites=NULL, *site=NULL;

	states = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), states);

	for (int i = 0; cities[i].type; i++) {
		if (cities[i].type == LOCATION_STATE) {
			state = gtk_menu_item_new_with_label(cities[i].name);
			sites = gtk_menu_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(states), state);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(state), sites);
		} else if (cities[i].type == LOCATION_CITY) {
			site  = gtk_menu_item_new_with_label(cities[i].name);
			gtk_menu_shell_append(GTK_MENU_SHELL(sites), site);
			g_object_set_data(G_OBJECT(site), "city", &cities[i]);
			g_signal_connect(site, "activate", G_CALLBACK(on_radar_changed), self);
		}
	}
}

static void prefs_setup(AWeatherGui *self)
{
	/* Set values */
	gint   uf = grits_prefs_get_integer(self->prefs, "aweather/update_freq",  NULL);
	gchar *nu = grits_prefs_get_string (self->prefs, "aweather/nexrad_url",   NULL);
	gint   ll = grits_prefs_get_integer(self->prefs, "aweather/log_level",    NULL);
	gchar *is = grits_prefs_get_string (self->prefs, "aweather/initial_site", NULL);
	GtkWidget *ufw = aweather_gui_get_widget(self, "prefs_general_freq");
	GtkWidget *nuw = aweather_gui_get_widget(self, "prefs_general_url");
	GtkWidget *llw = aweather_gui_get_widget(self, "prefs_general_log");
	GtkWidget *isw = aweather_gui_get_widget(self, "prefs_general_site");
	if (uf) gtk_spin_button_set_value(GTK_SPIN_BUTTON(ufw), uf);
	if (nu) gtk_entry_set_text(GTK_ENTRY(nuw), nu), g_free(nu);
	if (ll) gtk_spin_button_set_value(GTK_SPIN_BUTTON(llw), ll);
	if (is) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(isw));
		GtkTreeIter iter;
		if (gtk_tree_model_find_string(model, &iter, NULL, 0, is))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(isw), &iter);
		g_free(is);
	}

	/* Load Plugins */
	GtkTreeView       *tview = GTK_TREE_VIEW(aweather_gui_get_widget(self, "prefs_plugins_view"));
	GtkCellRenderer   *rend1 = gtk_cell_renderer_text_new();
	GtkCellRenderer   *rend2 = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn *col1  = gtk_tree_view_column_new_with_attributes(
			"Plugin",  rend1, "text",   0, NULL);
	GtkTreeViewColumn *col2  = gtk_tree_view_column_new_with_attributes(
			"Enabled", rend2, "active", 1, NULL);
	g_object_set(rend2, "xalign", 0.0, NULL);
	gtk_tree_view_append_column(tview, col1);
	gtk_tree_view_append_column(tview, col2);
	g_signal_connect(rend2, "toggled", G_CALLBACK(on_plugin_toggled), self);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tview), GTK_TREE_MODEL(self->gtk_plugins));
}

static void time_setup(AWeatherGui *self)
{
	/* Add times */
	GtkTreeStore *store = GTK_TREE_STORE(aweather_gui_get_object(self, "times"));
	for (int hour = 0; hour < 24; hour++) {
		GtkTreeIter hour_iter;
		gchar *str = g_strdup_printf("%02d:00Z", hour);
		gtk_tree_store_append(store, &hour_iter, NULL);
		gtk_tree_store_set(store, &hour_iter, 0, str, 1, hour, 2, 0, -1);
		g_free(str);
		for (int min = 5; min < 60; min += 5) {
			GtkTreeIter min_iter;
			gchar *str = g_strdup_printf("%02d:%02dZ", hour, min);
			gtk_tree_store_append(store, &min_iter, &hour_iter);
			gtk_tree_store_set(store, &min_iter, 0, str, 1, hour, 2, min, -1);
			g_free(str);
		}
	}

	/* Connect signals */
	GtkWidget *cal  = aweather_gui_get_widget(self, "main_date_cal");
	GtkWidget *view = aweather_gui_get_widget(self, "main_time");
	g_signal_connect_swapped(cal,  "day-selected-double-click",
			G_CALLBACK(on_time_changed), self);
	g_signal_connect_swapped(view, "row-activated",
			G_CALLBACK(on_time_changed), self);
	g_signal_connect(self->viewer, "time-changed",
			G_CALLBACK(update_time_widget), self);
}

static void icons_setup(AWeatherGui *self)
{
	gchar *icons[] = {
		ICONDIR "/hicolor/16x16/apps/aweather.png",
		ICONDIR "/hicolor/22x22/apps/aweather.png",
		ICONDIR "/hicolor/24x24/apps/aweather.png",
		ICONDIR "/hicolor/32x32/apps/aweather.png",
		ICONDIR "/hicolor/48x48/apps/aweather.png",
		ICONDIR "/hicolor/scalable/apps/aweather.svg",
	};
	GList *list = NULL;
	for (int i = 0; i < G_N_ELEMENTS(icons); i++) {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(icons[i], NULL);
		if (!pixbuf)
			g_warning("AWeatherGui: icons_setup - %s failed", icons[i]);
		list = g_list_prepend(list, pixbuf);
	}
	gtk_window_set_default_icon_list(list);
	g_list_free(list);
}


/***********
 * Methods *
 ***********/
AWeatherGui *aweather_gui_new()
{
	g_debug("AWeatherGui: new");
	return g_object_new(AWEATHER_TYPE_GUI, NULL);
}
GritsViewer *aweather_gui_get_viewer(AWeatherGui *self)
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
	GritsPlugin *plugin = grits_plugins_enable(self->plugins, name,
			self->viewer, self->prefs);
	GtkWidget *body = grits_plugin_get_config(plugin);
	if (body) {
		GtkWidget *config = aweather_gui_get_widget(self, "main_tabs");
		GtkWidget *tab    = gtk_label_new(name);
		gtk_notebook_append_page(GTK_NOTEBOOK(config), body, tab);
		gtk_widget_show_all(config);
	}
	grits_viewer_queue_draw(self->viewer);
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
	grits_plugins_disable(self->plugins, name);
	grits_viewer_queue_draw(self->viewer);
}
void aweather_gui_load_plugins(AWeatherGui *self)
{
	g_debug("AWeatherGui: load_plugins");
	GtkTreeIter iter;
	for (GList *cur = grits_plugins_available(self->plugins); cur; cur = cur->next) {
		gchar *name = cur->data;
		GError *error = NULL;
		gboolean enabled = grits_prefs_get_boolean_v(self->prefs, "plugins", cur->data, &error);
		if (error && error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
			enabled = TRUE;
		gtk_list_store_append(self->gtk_plugins, &iter);
		gtk_list_store_set(self->gtk_plugins, &iter, 0, name, 1, enabled, -1);
		if (enabled)
			aweather_gui_attach_plugin(self, name);
	}
}



/****************
 * GObject code *
 ****************/
static void aweather_gui_parser_finished(GtkBuildable *_self, GtkBuilder *builder)
{
	g_debug("AWeatherGui: parser finished");
	AWeatherGui *self = AWEATHER_GUI(_self);
	self->builder = builder;

	/* Simple things */
	gchar *config   = g_build_filename(g_get_user_config_dir(), PACKAGE, "config.ini", NULL);
	gchar *defaults = g_build_filename(PKGDATADIR, "defaults.ini", NULL);
	self->prefs   = grits_prefs_new(config, defaults);
	self->plugins = grits_plugins_new(PLUGINSDIR, self->prefs);
	self->viewer  = GRITS_VIEWER(aweather_gui_get_widget(self, "main_viewer"));
	self->gtk_plugins = GTK_LIST_STORE(aweather_gui_get_object(self, "plugins"));
	grits_viewer_setup(self->viewer, self->plugins, self->prefs);
	g_free(config);
	g_free(defaults);

	/* Misc, helpers */
	site_setup(self);
	menu_setup(self);
	time_setup(self);
	prefs_setup(self);
	icons_setup(self);

	/* Default size */
	gint width  = grits_prefs_get_integer(self->prefs, "aweather/width",  NULL);
	gint height = grits_prefs_get_integer(self->prefs, "aweather/height", NULL);
	if (width && height)
		gtk_window_set_default_size(GTK_WINDOW(self), width, height);

	/* Connect signals */
	gtk_builder_connect_signals(self->builder, self);
	g_signal_connect(self, "key-press-event",
			G_CALLBACK(on_gui_key_press), self);
	g_signal_connect_swapped(self->viewer, "offline",
			G_CALLBACK(gtk_toggle_action_set_active),
			aweather_gui_get_object(self, "offline"));
	g_signal_connect_swapped(self->viewer, "refresh",
			G_CALLBACK(set_update_timeout), self);
	g_signal_connect_swapped(self->viewer, "realize",
			G_CALLBACK(aweather_gui_load_plugins), self);
}
static void aweather_gui_buildable_init(GtkBuildableIface *iface)
{
	iface->parser_finished = aweather_gui_parser_finished;
}
G_DEFINE_TYPE_WITH_CODE(AWeatherGui, aweather_gui, GTK_TYPE_WINDOW,
		G_IMPLEMENT_INTERFACE(GTK_TYPE_BUILDABLE,
			aweather_gui_buildable_init));
static void aweather_gui_init(AWeatherGui *self)
{
	g_debug("AWeatherGui: init");
	/* Do all the real work in parser_finished */
}
static GObject *aweather_gui_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("AWeatherGui: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_gui_parent_class);
	return parent_class->constructor(gtype, n_properties, properties);
}
static void aweather_gui_dispose(GObject *_self)
{
	g_debug("AWeatherGui: dispose");
	AWeatherGui *self = AWEATHER_GUI(_self);
	if (self->plugins) {
		GritsPlugins *plugins = self->plugins;
		self->plugins = NULL;
		grits_plugins_free(plugins);
	}
	if (self->builder) {
		GtkBuilder *builder = self->builder;
		self->builder = NULL;
		g_object_unref(builder);
	}
	if (self->prefs) {
		GritsPrefs *prefs = self->prefs;
		self->prefs = NULL;
		g_object_unref(prefs);
	}
	G_OBJECT_CLASS(aweather_gui_parent_class)->dispose(_self);
}
static void aweather_gui_finalize(GObject *_self)
{
	g_debug("AWeatherGui: finalize");
	G_OBJECT_CLASS(aweather_gui_parent_class)->finalize(_self);

}
static void aweather_gui_class_init(AWeatherGuiClass *klass)
{
	g_debug("AWeatherGui: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = aweather_gui_constructor;
	gobject_class->dispose      = aweather_gui_dispose;
	gobject_class->finalize     = aweather_gui_finalize;
}
