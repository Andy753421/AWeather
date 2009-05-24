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
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#include "aweather-gui.h"
#include "aweather-view.h"
#include "location.h"

/*************
 * callbacks *
 *************/
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AWeatherGui *gui)
{
	g_message("key_press: key=%x, state=%x", event->keyval, event->state);
	AWeatherView *view = aweather_gui_get_view(gui);
	double x,y,z;
	aweather_view_get_location(view, &x, &y, &z);
	if (event->keyval == GDK_q)
		gtk_main_quit();
	else if (event->keyval == GDK_r && event->state & GDK_CONTROL_MASK)
		aweather_view_refresh(view);
	else if (event->keyval == GDK_Right) aweather_view_pan(view,  z/10, 0, 0);
	else if (event->keyval == GDK_Left)  aweather_view_pan(view, -z/10, 0, 0);
	else if (event->keyval == GDK_Up)    aweather_view_pan(view, 0,  z/10, 0);
	else if (event->keyval == GDK_Down)  aweather_view_pan(view, 0, -z/10, 0);
	else if (event->keyval == GDK_minus) aweather_view_zoom(view, 10./9);
	else if (event->keyval == GDK_plus)  aweather_view_zoom(view, 9./10);
	else if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) {
		GtkNotebook *tabs = GTK_NOTEBOOK(aweather_gui_get_widget(gui, "tabs"));
		gint num_tabs = gtk_notebook_get_n_pages(tabs);
		gint cur_tab  = gtk_notebook_get_current_page(tabs);
		if (event->state & GDK_SHIFT_MASK)
			gtk_notebook_set_current_page(tabs, (cur_tab-1)%num_tabs);
		else 
			gtk_notebook_set_current_page(tabs, (cur_tab+1)%num_tabs);
	};
	return TRUE;
}

void on_refresh(GtkToolButton *button, AWeatherGui *gui)
{
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_refresh(view);
}

void on_zoomin(GtkToolButton *button, AWeatherGui *gui)
{
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_zoom(view, 3./4);
}

void on_zoomout(GtkToolButton *button, AWeatherGui *gui)
{
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_zoom(view, 4./3);
}

void on_time_changed(GtkTreeView *view, GtkTreePath *path,
		GtkTreeViewColumn *column, AWeatherGui *gui)
{
	gchar *time;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 0, &time, -1);
	AWeatherView *aview = aweather_gui_get_view(gui);
	aweather_view_set_time(aview, time);
	g_free(time);
}

void on_site_changed(GtkComboBox *combo, AWeatherGui *gui)
{
	gchar *site;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(model, &iter, 1, &site, -1);
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_set_site(view, site);
	g_free(site);
}

gboolean on_map(GtkWidget *da, GdkEventConfigure *event, AWeatherGui *gui)
{
	g_message("on_map");
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_set_site(view, "IND");

	/* Misc */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	/* Tessellation, "finding intersecting triangles" */
	/* http://research.microsoft.com/pubs/70307/tr-2006-81.pdf */
	/* http://www.opengl.org/wiki/Alpha_Blending */
	glAlphaFunc(GL_GREATER,0.1);
	glEnable(GL_ALPHA_TEST);

	/* Depth test */
	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	aweather_gui_gl_end(gui);
	return FALSE;
}

gboolean on_configure(GtkWidget *da, GdkEventConfigure *event, AWeatherGui *gui)
{
	//g_message("on_confiure");
	aweather_gui_gl_begin(gui);

	double x, y, z;
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_get_location(view, &x, &y, &z);

	/* Window is at 500 m from camera */
	double width  = da->allocation.width;
	double height = da->allocation.height;

	glViewport(0, 0, width, height);

	/* Perspective */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double rad  = atan((height/2)/500);
	double deg  = (rad*180)/M_PI;

	/* TODO: set clipping plane properly */
	gluPerspective(deg*2, width/height, -z-20, -z+20);

	aweather_gui_gl_end(gui);
	return FALSE;
}

gboolean on_expose_begin(GtkWidget *da, GdkEventExpose *event, AWeatherGui *gui)
{
	g_message("aweather:espose_begin");
	aweather_gui_gl_begin(gui);

	double x, y, z;
	AWeatherView *view = aweather_gui_get_view(gui);
	aweather_view_get_location(view, &x, &y, &z);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(x, y, z);

	//glRotatef(-45, 1, 0, 0);

	return FALSE;
}
gboolean on_expose_end(GtkWidget *da, GdkEventExpose *event, AWeatherGui *gui)
{
	g_message("aweather:espose_end\n");
	aweather_gui_gl_end(gui);
	aweather_gui_gl_flush(gui);
	return FALSE;
}

void on_about(GtkMenuItem *item, AWeatherGui *gui)
{
	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, DATADIR "/aweather/about.xml", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gtk_builder_connect_signals(builder, NULL);
	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	gtk_window_set_transient_for(GTK_WINDOW(window),
			GTK_WINDOW(aweather_gui_get_widget(gui, "window")));
	gtk_widget_show_all(window);
	g_object_unref(builder);
}

void on_location_changed(AWeatherView *view,
		gdouble x, gdouble y, gdouble z, AWeatherGui *gui)
{
	/* Reset clipping area and redraw */
	GtkWidget *da = aweather_gui_get_widget(gui, "drawing");
	on_configure(da, NULL, gui);
	aweather_gui_gl_redraw(gui);
}

/* TODO: replace the code in these with `gtk_tree_model_find' utility */
static void update_time_widget(AWeatherView *view, char *time, AWeatherGui *gui)
{
	g_message("updating time widget");
	GtkTreeView  *tview = GTK_TREE_VIEW(aweather_gui_get_widget(gui, "time"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(tview));
	for (int i = 0; i < gtk_tree_model_iter_n_children(model, NULL); i++) {
		char *text;
		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, i);
		gtk_tree_model_get(model, &iter, 0, &text, -1);
		if (g_str_equal(text, time)) {
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			g_signal_handlers_block_by_func(tview,
					G_CALLBACK(on_site_changed), gui);
			gtk_tree_view_set_cursor(tview, path, NULL, FALSE);
			g_signal_handlers_unblock_by_func(tview,
					G_CALLBACK(on_site_changed), gui);
			gtk_tree_path_free(path);
			g_free(text);
			return;
		}
		g_free(text);
	}
}
static void update_site_widget(AWeatherView *view, char *site, AWeatherGui *gui)
{
	g_message("updating site widget to %s", site);
	GtkComboBox  *combo = GTK_COMBO_BOX(aweather_gui_get_widget(gui, "site"));
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_combo_box_get_model(combo));
	for (int i = 0; i < gtk_tree_model_iter_n_children(model, NULL); i++) {
		GtkTreeIter iter1;
		gtk_tree_model_iter_nth_child(model, &iter1, NULL, i);
		for (int i = 0; i < gtk_tree_model_iter_n_children(model, &iter1); i++) {
			GtkTreeIter iter2;
			gtk_tree_model_iter_nth_child(model, &iter2, &iter1, i);
			char *text;
			gtk_tree_model_get(model, &iter2, 1, &text, -1);
			if (g_str_equal(text, site)) {
				g_signal_handlers_block_by_func(combo,
						G_CALLBACK(on_site_changed), gui);
				gtk_combo_box_set_active_iter(combo, &iter2);
				g_signal_handlers_unblock_by_func(combo,
						G_CALLBACK(on_site_changed), gui);
				g_free(text);
				return;
			}
			g_free(text);
		}
	}
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

	GtkWidget       *combo    = aweather_gui_get_widget(gui, "site");
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, NULL);
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(store));
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer,
			combo_sensitive, NULL, NULL);
	g_object_unref(renderer);
	g_object_unref(store);

	AWeatherView *aview = aweather_gui_get_view(gui);
	g_signal_connect(aview, "site-changed", G_CALLBACK(update_site_widget), gui);
}

static void time_setup(AWeatherGui *gui)
{
	GtkTreeView  *tview  = GTK_TREE_VIEW(aweather_gui_get_widget(gui, "time"));
	GtkTreeModel *store  = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
	gtk_tree_view_set_model(tview, store);

	GtkCellRenderer   *rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col  = gtk_tree_view_column_new_with_attributes(
					"Time", rend, "text", 0, NULL);
	gtk_tree_view_append_column(tview, col);

	AWeatherView *aview = aweather_gui_get_view(gui);
	g_signal_connect(aview, "time-changed", G_CALLBACK(update_time_widget), gui);
}

static void opengl_setup(AWeatherGui *gui)
{
	GtkDrawingArea *drawing = GTK_DRAWING_AREA(aweather_gui_get_widget(gui, "drawing"));

	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(GTK_WIDGET(drawing),
				glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");

	/* Set up OpenGL Stuff, glade doesn't like doing these */
	g_signal_connect      (drawing, "map-event",       G_CALLBACK(on_map),          gui);
	g_signal_connect      (drawing, "configure-event", G_CALLBACK(on_configure),    gui);
	g_signal_connect      (drawing, "expose-event",    G_CALLBACK(on_expose_begin), gui);
	g_signal_connect_after(drawing, "expose-event",    G_CALLBACK(on_expose_end),   gui);
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherGui, aweather_gui, G_TYPE_OBJECT);

/* Constructor / destructors */
static void aweather_gui_init(AWeatherGui *gui)
{
	//g_message("aweather_gui_init");
}

static GObject *aweather_gui_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	//g_message("aweather_gui_constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_gui_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}

static void aweather_gui_dispose (GObject *gobject)
{
	//g_message("aweather_gui_dispose");
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
	//g_message("aweather_gui_finalize");
	G_OBJECT_CLASS(aweather_gui_parent_class)->finalize(gobject);
}

static void aweather_gui_class_init(AWeatherGuiClass *klass)
{
	//g_message("aweather_gui_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = aweather_gui_constructor;
	gobject_class->dispose      = aweather_gui_dispose;
	gobject_class->finalize     = aweather_gui_finalize;
}

/* Methods */
AWeatherGui *aweather_gui_new()
{
	//g_message("aweather_gui_new");
	GError *error = NULL;

	AWeatherGui *gui = g_object_new(AWEATHER_TYPE_GUI, NULL);

	gui->builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(gui->builder, DATADIR "/aweather/main.xml", &error))
		g_error("Failed to create gtk builder: %s", error->message);
	gui->view    = aweather_view_new();
	gui->window  = GTK_WINDOW      (gtk_builder_get_object(gui->builder, "main"));
	gui->tabs    = GTK_NOTEBOOK    (gtk_builder_get_object(gui->builder, "tabs")); 
	gui->drawing = GTK_DRAWING_AREA(gtk_builder_get_object(gui->builder, "drawing"));
	gtk_builder_connect_signals(gui->builder, gui);


	/* Load components */
	aweather_view_set_location(gui->view, 0, 0, -500*1000);
	g_signal_connect(gui->view, "location-changed", G_CALLBACK(on_location_changed), gui);
	site_setup(gui);
	time_setup(gui);
	opengl_setup(gui);
	return gui;
}
AWeatherView *aweather_gui_get_view(AWeatherGui *gui)
{
	g_assert(AWEATHER_IS_GUI(gui));
	return gui->view;
}
GtkBuilder *aweather_gui_get_builder(AWeatherGui *gui)
{
	g_assert(AWEATHER_IS_GUI(gui));
	return gui->builder;
}
GtkWidget *aweather_gui_get_widget(AWeatherGui *gui, const gchar *name)
{
	g_assert(AWEATHER_IS_GUI(gui));
	GObject *widget = gtk_builder_get_object(gui->builder, name);
	if (!GTK_IS_WIDGET(widget))
		g_error("Failed to get widget `%s'", name);
	return GTK_WIDGET(widget);
}
void aweather_gui_gl_redraw(AWeatherGui *gui)
{
	g_message("redraw");
	GtkWidget *drawing = aweather_gui_get_widget(gui, "drawing");
	gtk_widget_queue_draw(drawing);
}
void aweather_gui_gl_begin(AWeatherGui *gui)
{
	g_assert(AWEATHER_IS_GUI(gui));

	GtkDrawingArea *drawing    = GTK_DRAWING_AREA(aweather_gui_get_widget(gui, "drawing"));
	GdkGLContext   *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(drawing));
	GdkGLDrawable  *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(drawing));

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void aweather_gui_gl_end(AWeatherGui *gui)
{
	g_assert(AWEATHER_IS_GUI(gui));
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	gdk_gl_drawable_gl_end(gldrawable);
}
void aweather_gui_gl_flush(AWeatherGui *gui)
{
	g_assert(AWEATHER_IS_GUI(gui));
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();
	gdk_gl_drawable_gl_end(gldrawable);
}
//void aweather_gui_register_plugin(AWeatherGui *gui, AWeatherPlugin *plugin);
