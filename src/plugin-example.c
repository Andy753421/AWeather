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

#include "aweather-gui.h"
#include "plugin-example.h"

/****************
 * GObject code *
 ****************/
/* Plugin init */
static void aweather_example_plugin_init(AWeatherPluginInterface *iface);
static void aweather_example_expose(AWeatherPlugin *_example);
G_DEFINE_TYPE_WITH_CODE(AWeatherExample, aweather_example, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(AWEATHER_TYPE_PLUGIN,
			aweather_example_plugin_init));
static void aweather_example_plugin_init(AWeatherPluginInterface *iface)
{
	g_debug("AWeatherExample: plugin_init");
	/* Add methods to the interface */
	iface->expose = aweather_example_expose;
}
/* Class/Object init */
static void aweather_example_init(AWeatherExample *example)
{
	g_debug("AWeatherExample: init");
	/* Set defaults */
	example->gui      = NULL;
	example->button   = NULL;
	example->rotation = 30.0;
}
static void aweather_example_dispose(GObject *gobject)
{
	g_debug("AWeatherExample: dispose");
	AWeatherExample *self = AWEATHER_EXAMPLE(gobject);
	/* Drop references */
	G_OBJECT_CLASS(aweather_example_parent_class)->dispose(gobject);
}
static void aweather_example_finalize(GObject *gobject)
{
	g_debug("AWeatherExample: finalize");
	AWeatherExample *self = AWEATHER_EXAMPLE(gobject);
	/* Free data */
	G_OBJECT_CLASS(aweather_example_parent_class)->finalize(gobject);

}
static void aweather_example_class_init(AWeatherExampleClass *klass)
{
	g_debug("AWeatherExample: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = aweather_example_dispose;
	gobject_class->finalize = aweather_example_finalize;
}

/***********
 * Helpers *
 ***********/
static gboolean rotate(gpointer _example)
{
	AWeatherExample *example = _example;
	if (gtk_toggle_button_get_active(example->button)) {
		example->rotation += 1.0;
		aweather_gui_gl_redraw(example->gui);
	}
	return TRUE;
}

/***********
 * Methods *
 ***********/
AWeatherExample *aweather_example_new(AWeatherGui *gui)
{
	g_debug("AWeatherExample: new");
	AWeatherExample *example = g_object_new(AWEATHER_TYPE_EXAMPLE, NULL);
	example->gui = gui;

	GtkWidget *drawing = aweather_gui_get_widget(gui, "drawing");
	GtkWidget *config  = aweather_gui_get_widget(gui, "tabs");

	/* Add configuration tab */
	GtkWidget *label = gtk_label_new("Example");
	example->button = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label("Rotate"));
	gtk_notebook_append_page(GTK_NOTEBOOK(config), GTK_WIDGET(example->button), label);

	/* Set up OpenGL Stuff */
	//g_signal_connect(drawing, "expose-event", G_CALLBACK(expose), NULL);
	g_timeout_add(1000/60, rotate, example);

	return example;
}

static void aweather_example_expose(AWeatherPlugin *_example)
{
	AWeatherExample *example = AWEATHER_EXAMPLE(_example);
	g_debug("AWeatherExample: expose");
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
	glRotatef(example->rotation, 1, 0, 1);
	glColor4f(0.9, 0.9, 0.7, 1.0);
	gdk_gl_draw_teapot(TRUE, 0.25);
	gdk_gl_draw_cube(TRUE, 0.25);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

        glMatrixMode(GL_PROJECTION); glPopMatrix(); 
	glMatrixMode(GL_MODELVIEW ); glPopMatrix();
	return;
}

