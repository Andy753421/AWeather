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
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "misc.h"
#include "aweather-gui.h"
#include "gis-world.h"
#include "gis-view.h"
#include "gis-opengl.h"

/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisOpenGL, gis_opengl, G_TYPE_OBJECT);
static void gis_opengl_init(GisOpenGL *self)
{
	g_debug("GisOpenGL: init");
}
static GObject *gis_opengl_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("gis_opengl: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(gis_opengl_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}
static void gis_opengl_dispose(GObject *gobject)
{
	g_debug("GisOpenGL: dispose");
	GisOpenGL *self = GIS_OPENGL(gobject);
	if (self->world) {
		g_object_unref(self->world);
		self->world = NULL;
	}
	if (self->view) {
		g_object_unref(self->view);
		self->view = NULL;
	}
	if (self->drawing) {
		g_object_unref(self->drawing);
		self->drawing = NULL;
	}
	G_OBJECT_CLASS(gis_opengl_parent_class)->dispose(gobject);
}
static void gis_opengl_finalize(GObject *gobject)
{
	g_debug("GisOpenGL: finalize");
	G_OBJECT_CLASS(gis_opengl_parent_class)->finalize(gobject);
	gtk_main_quit();

}
static void gis_opengl_class_init(GisOpenGLClass *klass)
{
	g_debug("GisOpenGL: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = gis_opengl_constructor;
	gobject_class->dispose      = gis_opengl_dispose;
	gobject_class->finalize     = gis_opengl_finalize;
}

/*************
 * Callbacks *
 *************/
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, GisOpenGL *self)
{
	g_debug("GisOpenGL: on_drawing_button_press - Grabbing focus");
	gtk_widget_grab_focus(GTK_WIDGET(self->drawing));
	return TRUE;
}
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, GisOpenGL *self)
{
	g_debug("GisOpenGL: on_drawing_key_press - key=%x, state=%x, plus=%x",
			event->keyval, event->state, GDK_plus);
	double x,y,z;
	gis_view_get_location(self->view, &x, &y, &z);
	guint kv = event->keyval;
	if      (kv == GDK_Left  || kv == GDK_h) gis_view_pan(self->view, -z/10, 0, 0);
	else if (kv == GDK_Down  || kv == GDK_j) gis_view_pan(self->view, 0, -z/10, 0);
	else if (kv == GDK_Up    || kv == GDK_k) gis_view_pan(self->view, 0,  z/10, 0);
	else if (kv == GDK_Right || kv == GDK_l) gis_view_pan(self->view,  z/10, 0, 0);
	else if (kv == GDK_minus || kv == GDK_o) gis_view_zoom(self->view, 10./9);
	else if (kv == GDK_plus  || kv == GDK_i) gis_view_zoom(self->view, 9./10);
	else if (kv == GDK_H                   ) gis_view_rotate(self->view,  0, -10, 0);
	else if (kv == GDK_J                   ) gis_view_rotate(self->view,  10,  0, 0);
	else if (kv == GDK_K                   ) gis_view_rotate(self->view, -10,  0, 0);
	else if (kv == GDK_L                   ) gis_view_rotate(self->view,  0,  10, 0);
	return TRUE;
}


gboolean on_map(GtkWidget *drawing, GdkEventConfigure *event, GisOpenGL *self)
{
	g_debug("GisOpenGL: on_map");

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

	gis_opengl_end(self);
	return FALSE;
}

gboolean on_configure(GtkWidget *drawing, GdkEventConfigure *event, GisOpenGL *self)
{
	g_debug("GisOpenGL: on_confiure");
	gis_opengl_begin(self);

	double x, y, z;
	gis_view_get_location(self->view, &x, &y, &z);

	/* Window is at 500 m from camera */
	double width  = GTK_WIDGET(self->drawing)->allocation.width;
	double height = GTK_WIDGET(self->drawing)->allocation.height;

	glViewport(0, 0, width, height);

	/* Perspective */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double ang = atan((height/2)/500);

	//gluPerspective(r2d(ang)*2, width/height, -z-20, -z+20);
	gluPerspective(r2d(ang)*2, width/height, 1, 500*1000);

	gis_opengl_end(self);
	return FALSE;
}

gboolean on_expose(GtkWidget *drawing, GdkEventExpose *event, GisOpenGL *self)
{
	g_debug("GisOpenGL: on_expose - begin");
	gis_opengl_begin(self);

	double lx, ly, lz;
	double rx, ry, rz;
	gis_view_get_location(self->view, &lx, &ly, &lz);
	gis_view_get_rotation(self->view, &rx, &ry, &rz);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(lx, ly, lz);
	glRotatef(rx, 1, 0, 0);
	glRotatef(ry, 0, 1, 0);
	glRotatef(rz, 0, 0, 1);

	/* Expose plugins */
	/* TODO: Figure out how to handle plugins:
	 *   - Do they belong to AWeatherGui, GisOpenGL, etc?
	 */
	for (GList *cur = self->plugins; cur; cur = cur->next) {
		AWeatherPlugin *plugin = AWEATHER_PLUGIN(cur->data);
		aweather_plugin_expose(plugin);
	}

	gis_opengl_end(self);
	gis_opengl_flush(self);
	g_debug("GisOpenGL: on_expose - end\n");
	return FALSE;
}

void on_state_changed(GisView *view,
		gdouble x, gdouble y, gdouble z, GisOpenGL *self)
{
	/* Reset clipping area and redraw */
	on_configure(NULL, NULL, self);
	gis_opengl_redraw(self);
}

/***********
 * Methods *
 ***********/
GisOpenGL *gis_opengl_new(GisWorld *world, GisView *view, GtkDrawingArea *drawing)
{
	g_debug("GisOpenGL: new");
	GisOpenGL *self = g_object_new(GIS_TYPE_OPENGL, NULL);
	self->world   = world;
	self->view    = view;
	self->drawing = drawing;
	g_object_ref(world);
	g_object_ref(view);
	g_object_ref(drawing);

	/* OpenGL setup */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(GTK_WIDGET(self->drawing),
				glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");

	g_signal_connect(self->view, "location-changed", G_CALLBACK(on_state_changed), self);
	g_signal_connect(self->view, "rotation-changed", G_CALLBACK(on_state_changed), self);

	g_signal_connect(self->drawing, "map-event",       G_CALLBACK(on_map),       self);
	g_signal_connect(self->drawing, "configure-event", G_CALLBACK(on_configure), self);
	g_signal_connect(self->drawing, "expose-event",    G_CALLBACK(on_expose),    self);

	g_signal_connect(self->drawing, "button-press-event", G_CALLBACK(on_button_press), self);
	g_signal_connect(self->drawing, "enter-notify-event", G_CALLBACK(on_button_press), self);
	g_signal_connect(self->drawing, "key-press-event",    G_CALLBACK(on_key_press),    self);
	return self;
}

void gis_opengl_redraw(GisOpenGL *self)
{
	g_debug("GisOpenGL: gl_redraw");
	gtk_widget_queue_draw(GTK_WIDGET(self->drawing));
}
void gis_opengl_begin(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));

	GdkGLContext   *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(self->drawing));
	GdkGLDrawable  *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self->drawing));

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void gis_opengl_end(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	gdk_gl_drawable_gl_end(gldrawable);
}
void gis_opengl_flush(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();
	gdk_gl_drawable_gl_end(gldrawable);
}
