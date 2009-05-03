#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <math.h>

#include "rsl.h"

#include "radar.h"

GtkWidget *drawing;
static Sweep *cur_sweep; // make this not global
static int nred, ngreen, nblue;
static char red[256], green[256], blue[256];

/* Convert a sweep to an 2d array of data points */
static void bscan_sweep(Sweep *sweep, guint8 **data, int *width, int *height)
{
	/* Calculate max number of bins */
	int i, max_bins = 0;
	for (i = 0; i < sweep->h.nrays; i++)
		max_bins = MAX(max_bins, sweep->ray[i]->h.nbins);

	/* Allocate buffer using max number of bins for each ray */
	guint8 *buf = g_malloc0(sweep->h.nrays * max_bins * 3);

	/* Fill the data */
	int ri, bi;
	for (ri = 0; ri < sweep->h.nrays; ri++) {
		Ray   *ray  = sweep->ray[ri];
		for (bi = 0; bi < ray->h.nbins; bi++) {
			Range bin = ray->range[bi];
			/* copy RGB into buffer */
			buf[(ri*max_bins+bi)*3+0] =   red[(gint8)ray->h.f(bin)];
			buf[(ri*max_bins+bi)*3+1] = green[(gint8)ray->h.f(bin)];
			buf[(ri*max_bins+bi)*3+2] =  blue[(gint8)ray->h.f(bin)];
		}
	}

	/* set output */
	*width  = max_bins;
	*height = sweep->h.nrays;
	*data   = buf;

	/* debug */
	//static int fi = 0;
	//char fname[128];
	//sprintf(fname, "image-%d.ppm", fi);
	//FILE *fp = fopen(fname, "w+");
	//fprintf(fp, "P6\n");
	//fprintf(fp, "# Foo\n");
	//fprintf(fp, "%d %d\n", *width, *height);
	//fprintf(fp, "255\n");
	//fwrite(buf, 3, *width * *height, fp);
	//fclose(fp);
	//fi++;
}

/* Load a sweep as the active texture */
static void load_sweep(Sweep *sweep)
{
	cur_sweep = sweep;
	int height, width;
	guint8 *data;
	bscan_sweep(sweep, &data, &width, &height);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	g_free(data);
	glEnable(GL_TEXTURE_2D);
	gdk_window_invalidate_rect(drawing->window, &drawing->allocation, FALSE);
}

/* Load the default sweep */
static gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	/* Load the texture */
	load_sweep(cur_sweep);

	gdk_gl_drawable_gl_end(gldrawable);

	return FALSE;
}

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	Sweep *sweep = cur_sweep;

	//GdkGLContext *glcontext = gtk_widget_get_gl_context(da);
	//GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	/* draw in here */
	glPushMatrix();
	glRotatef(0, 0, 0, 0);

	/* Draw the rays */
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUAD_STRIP);
	int _ri; // not really used, creates strange fragments..
	for (_ri = 0; _ri < sweep->h.nrays; _ri++) {
		/* Do the first sweep twice to complete the last Quad */
		int ri = _ri % sweep->h.nrays;
		Ray *ray = sweep->ray[ri];

		/* right and left looking out from radar */
		double left  = ((ray->h.azimuth - ((double)ray->h.beam_width/2.))*M_PI)/180.0; 
		//double right = ((ray->h.azimuth + ((double)ray->h.beam_width/2.))*M_PI)/180.0; 

		double lx = sin(left);
		double ly = cos(left);

		/* TODO: change this to meters instead of 0..1 */
		double max_dist  = ray->h.nbins*ray->h.gate_size + ray->h.range_bin1;
		double near_dist = (double)(ray->h.range_bin1) / max_dist;
		double far_dist  = (double)(ray->h.nbins*ray->h.gate_size + ray->h.range_bin1) / max_dist;

		/* (find middle of bin) / scale for opengl */
		glTexCoord2d(0.0, ((double)ri)/sweep->h.nrays); glVertex3f(lx*near_dist, ly*near_dist, 0.); // near left
		glTexCoord2d(0.7, ((double)ri)/sweep->h.nrays); glVertex3f(lx*far_dist,  ly*far_dist,  0.); // far  left
	}
	//g_print("ri=%d, nr=%d, bw=%f\n", _ri, sweep->h.nrays, sweep->h.beam_width);
	glEnd();
	/* Texture debug */
	//glBegin(GL_QUADS);
	//glTexCoord2d( 0.,  0.); glVertex3f(-1.,  0., 0.); // bot left
	//glTexCoord2d( 0.,  1.); glVertex3f(-1.,  1., 0.); // top left
	//glTexCoord2d( 1.,  1.); glVertex3f( 0.,  1., 0.); // top right
	//glTexCoord2d( 1.,  0.); glVertex3f( 0.,  0., 0.); // bot right
	//glEnd();
	glDisable(GL_TEXTURE_2D);

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
	
	glPopMatrix();

	return FALSE;
}

gboolean radar_init(GtkDrawingArea *_drawing, GtkNotebook *config)
{
	drawing = GTK_WIDGET(_drawing);
	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(expose),    NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(configure), NULL);

	/* Parse hard coded file.. */
	RSL_read_these_sweeps("all", NULL);
	//RSL_read_these_sweeps("all", NULL);
	Radar *radar = RSL_wsr88d_to_radar("/scratch/aweather/data/KNQA_20090501_1925.raw", "KNQA");
	RSL_load_refl_color_table();
	RSL_get_color_table(RSL_RED_TABLE,   red,   &nred);
	RSL_get_color_table(RSL_GREEN_TABLE, green, &ngreen);
	RSL_get_color_table(RSL_BLUE_TABLE,  blue,  &nblue);
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1)
		g_print("No sweeps found\n");
	cur_sweep = radar->v[0]->sweep[6];

	/* Add configuration tab */
	GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
	GtkWidget *button = NULL;
	int vi = 0, si = 0;
	for (vi = 0; vi < radar->h.nvolumes; vi++) {
		Volume *vol = radar->v[vi];
		if (vol == NULL) continue;
		GtkWidget *vbox = gtk_vbox_new(TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
		for (si = 0; si < vol->h.nsweeps; si++) {
			Sweep *sweep = vol->sweep[si];
			if (sweep == NULL) continue;
			char *label = g_strdup_printf("Tilt: %.2f (%s)", sweep->h.elev, vol->h.type_str);
			button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), label);
			g_signal_connect_swapped(button, "clicked", G_CALLBACK(load_sweep), sweep);
			gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);
			g_free(label);
		}
	}
	GtkWidget *label = gtk_label_new("Radar");
	gtk_notebook_append_page(GTK_NOTEBOOK(config), hbox, label);
	return TRUE;
}
