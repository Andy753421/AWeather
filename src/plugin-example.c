#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>

#include "aweather-gui.h"

static GtkWidget *rotate_button;

static float ang = 30.;

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	//glOrtho(-1,1,-1,1,-10,10);

	glTranslatef(0.5, -0.5, -2);

	float light_ambient[]  = {0.1f, 0.1f, 0.0f};
	float light_diffuse[]  = {0.9f, 0.9f, 0.9f};
	float light_position[] = {-20.0f, 40.0f, -40.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glRotatef(ang, 1, 0, 1);
	glColor4f(0.9, 0.9, 0.7, 1.0);
	gdk_gl_draw_teapot(TRUE, 0.25);
	gdk_gl_draw_cube(TRUE, 0.25);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

        glMatrixMode(GL_PROJECTION); glPopMatrix(); 
	glMatrixMode(GL_MODELVIEW ); glPopMatrix();
	return FALSE;
}

static gboolean rotate(gpointer user_data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rotate_button)))
		return TRUE;

	GtkWidget *da = GTK_WIDGET (user_data);

	ang++;

	gdk_window_invalidate_rect(da->window, &da->allocation, FALSE);
	gdk_window_process_updates(da->window, FALSE);

	return TRUE;
}

gboolean example_init(AWeatherGui *gui)
{
	GtkWidget *drawing = aweather_gui_get_widget(gui, "drawing");
	GtkWidget *config  = aweather_gui_get_widget(gui, "tabs");

	/* Add configuration tab */
	GtkWidget *label = gtk_label_new("example");
	rotate_button = gtk_toggle_button_new_with_label("Rotate");
	gtk_notebook_append_page(GTK_NOTEBOOK(config), rotate_button, label);

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event", G_CALLBACK(expose), NULL);
	g_timeout_add(1000/60, rotate, drawing);
	return TRUE;
}
