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
#include <GL/gl.h>

#include <gis/gis.h>

#include "example.h"

/****************
 * GObject code *
 ****************/
/* Plugin init */
static void gis_plugin_example_plugin_init(GisPluginInterface *iface);
static void gis_plugin_example_expose(GisPlugin *_self);
static GtkWidget *gis_plugin_example_get_config(GisPlugin *_self);
G_DEFINE_TYPE_WITH_CODE(GisPluginExample, gis_plugin_example, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(GIS_TYPE_PLUGIN,
			gis_plugin_example_plugin_init));
static void gis_plugin_example_plugin_init(GisPluginInterface *iface)
{
	g_debug("GisPluginExample: plugin_init");
	/* Add methods to the interface */
	iface->expose     = gis_plugin_example_expose;
	iface->get_config = gis_plugin_example_get_config;
}
/* Class/Object init */
static void gis_plugin_example_init(GisPluginExample *self)
{
	g_debug("GisPluginExample: init");
	/* Set defaults */
	self->opengl   = NULL;
	self->button   = NULL;
	self->rotation = 30.0;
}
static void gis_plugin_example_dispose(GObject *gobject)
{
	g_debug("GisPluginExample: dispose");
	GisPluginExample *self = GIS_PLUGIN_EXAMPLE(gobject);
	/* Drop references */
	G_OBJECT_CLASS(gis_plugin_example_parent_class)->dispose(gobject);
}
static void gis_plugin_example_finalize(GObject *gobject)
{
	g_debug("GisPluginExample: finalize");
	GisPluginExample *self = GIS_PLUGIN_EXAMPLE(gobject);
	/* Free data */
	G_OBJECT_CLASS(gis_plugin_example_parent_class)->finalize(gobject);

}
static void gis_plugin_example_class_init(GisPluginExampleClass *klass)
{
	g_debug("GisPluginExample: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = gis_plugin_example_dispose;
	gobject_class->finalize = gis_plugin_example_finalize;
}

/***********
 * Helpers *
 ***********/
static gboolean rotate(gpointer _self)
{
	GisPluginExample *self = _self;
	if (gtk_toggle_button_get_active(self->button)) {
		self->rotation += 1.0;
		gis_opengl_redraw(self->opengl);
	}
	return TRUE;
}

/***********
 * Methods *
 ***********/
GisPluginExample *gis_plugin_example_new(GisWorld *world, GisView *view, GisOpenGL *opengl)
{
	g_debug("GisPluginExample: new");
	GisPluginExample *self = g_object_new(GIS_TYPE_PLUGIN_EXAMPLE, NULL);
	self->opengl = opengl;
	self->button = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label("Rotate"));

	/* Set up OpenGL Stuff */
	g_timeout_add(1000/60, rotate, self);

	return self;
}

static GtkWidget *gis_plugin_example_get_config(GisPlugin *_self)
{
	GisPluginExample *self = GIS_PLUGIN_EXAMPLE(_self);
	return GTK_WIDGET(self->button);
}

static void gis_plugin_example_expose(GisPlugin *_self)
{
	GisPluginExample *self = GIS_PLUGIN_EXAMPLE(_self);
	g_debug("GisPluginExample: expose");
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glOrtho(-1,1,-1,1,-10,10);
	glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();

	float light_ambient[]  = {0.1f, 0.1f, 0.0f};
	float light_diffuse[]  = {0.9f, 0.9f, 0.9f};
	float light_position[] = {-30.0f, 50.0f, 40.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glTranslatef(0.5, -0.5, -2);
	glRotatef(self->rotation, 1, 0, 1);
	glColor4f(0.9, 0.9, 0.7, 1.0);
	gdk_gl_draw_teapot(TRUE, 0.25);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

        glMatrixMode(GL_PROJECTION); glPopMatrix(); 
	glMatrixMode(GL_MODELVIEW ); glPopMatrix();
	return;
}

