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
static Sweep *sweep;
static int nred, ngreen, nblue;
static unsigned char red[256], green[256], blue[256];

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	/* draw in here */
	glPushMatrix();
	
	glRotatef(0, 0, 0, 0);

	glShadeModel(GL_FLAT);

	glBegin(GL_QUADS);

	int rayi;
	for (rayi = 0; rayi < sweep->h.nrays; rayi++) {
		Ray *ray = sweep->ray[rayi];

		/* right and left with respect to north */
		double right = ((ray->h.azimuth + ray->h.beam_width)*M_PI)/180.0; 
		double left  = ((ray->h.azimuth - ray->h.beam_width)*M_PI)/180.0; 

		double rx = sin(right), ry = cos(right);
		double lx = sin(left),  ly = cos(left);

		int nbins = ray->h.nbins;
		//int nbins = 20;
		int max_dist = nbins * ray->h.gate_size + ray->h.range_bin1;
		int bini, dist = ray->h.range_bin1;
		for (bini = 0; bini < nbins; bini++, dist += ray->h.gate_size) {
			if (ray->range[bini] == BADVAL) continue;

			/* (find middle of bin) / scale for opengl */
			double nd = ((double)dist - ((double)ray->h.gate_size)/2.) / (double)max_dist;
			double fd = ((double)dist + ((double)ray->h.gate_size)/2.) / (double)max_dist;
		
			glColor3ub(
				  red[(unsigned char)ray->h.f(ray->range[bini])],
				green[(unsigned char)ray->h.f(ray->range[bini])],
				 blue[(unsigned char)ray->h.f(ray->range[bini])]
			);

			glVertex3f(lx*nd, ly*nd, 0.); // near left
			glVertex3f(lx*fd, ly*fd, 0.); // far  left
			glVertex3f(rx*fd, ry*fd, 0.); // far  right
			glVertex3f(rx*nd, ry*nd, 0.); // near right

		}

		g_print("\n");
	}

	glEnd();

	/* Print the color table */
	glBegin(GL_QUADS);
	int i;
	for (i = 0; i < nred; i++) {
		glColor3ub(red[i], green[i], blue[i]);
		glVertex3f(-1., (float)((i  ) - nred/2)/(nred/2), 0.); // bot left
		glVertex3f(-1., (float)((i+1) - nred/2)/(nred/2), 0.); // top left
		glVertex3f(-.9, (float)((i+1) - nred/2)/(nred/2), 0.); // top right
		glVertex3f(-.9, (float)((i  ) - nred/2)/(nred/2), 0.); // bot right
	}
	glEnd();
	
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
	RSL_get_color_table(RSL_RED_TABLE,   red,   &nred);
	RSL_get_color_table(RSL_GREEN_TABLE, green, &ngreen);
	RSL_get_color_table(RSL_BLUE_TABLE,  blue,  &nblue);
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1)
		g_print("No sweeps found\n");
	sweep = radar->v[0]->sweep[0];
	RSL_volume_to_gif(radar->v[DZ_INDEX], "dz_sweep", 1024, 1024, 200.0);
}
