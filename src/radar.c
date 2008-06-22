#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <math.h>

#include "rsl.h"

#include "radar.h"

static GtkWidget *rotate_button;
static Radar *radar;

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	/* draw in here */
	glPushMatrix();
	
	glRotatef(0, 0, 0, 0);

	glShadeModel(GL_FLAT);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f(0., 0., 0.);
	glVertex3f(0., .5, 0.);

	int angle;
	for (angle = 1; angle <= 360; angle++) {
		double rad = (angle*M_PI)/180.0; 
		double x = sin(rad);
		double y = cos(rad);
		//g_printf("tick.. %d - %5.3f: (%5.2f,%5.2f)\n", angle, rad, x, y);
		glColor3f((float)angle/360.0, 0., 0.);
		glVertex3f(x/2., y/2., 0.);
	}

	glEnd ();
	
	glPopMatrix ();

	return FALSE;
}

gboolean radar_init(GtkDrawingArea *drawing, GtkNotebook *config)
{
	/* Add configuration tab */
	GtkWidget *label = gtk_label_new("Radar");
	rotate_button = gtk_toggle_button_new_with_label("Rotate");
	gtk_notebook_append_page(GTK_NOTEBOOK(config), rotate_button, label);

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event", G_CALLBACK(expose), NULL);

	/* Parse hard coded file.. */
	RSL_read_these_sweeps("0", NULL);
	radar = RSL_wsr88d_to_radar("/scratch/aweather/src/KABR_20080609_0224", "KABR");
	RSL_load_refl_color_table();
	RSL_volume_to_gif(radar->v[DZ_INDEX], "dz_sweep", 400, 400, 200.0);
}
