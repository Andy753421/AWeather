#include <config.h>
#include <gtk/gtk.h>
#include <GL/gl.h>
#include <math.h>

#include "rsl.h"

#include "radar.h"

GtkWidget *drawing;
static Sweep *cur_sweep = NULL;  // make this not global
static int nred, ngreen, nblue;
static char red[256], green[256], blue[256];
static guint sweep_tex = 0;

static guint8 get_alpha(guint8 db)
{
	if (db == BADVAL) return 0;
	if (db == RFVAL ) return 0;
	if (db == APFLAG) return 0;
	if (db == NOECHO) return 0;
	if (db == 0     ) return 0;
	//if      (db > 60) return 0;
	//else if (db < 10) return 0;
	//else if (db < 25) return (db-10)*(255.0/15);
	else              return 255;
}

//#ifdef USE_TWO_BYTE_PRECISION
//#define F_FACTOR 100.0
//#define F_DR_FACTOR 1000.0
//#define F_DZ_RANGE_OFFSET 50
//#else
//#define F_FACTOR 2.0
//#define F_DR_FACTOR 10.0
//#define F_DZ_RANGE_OFFSET 32
//#endif
//#define F_OFFSET 4
//static float dz_f(Range x)
//{
//	if (x >= F_OFFSET)
//		return (((float)x-F_OFFSET)/F_FACTOR - F_DZ_RANGE_OFFSET);
//	if (x == 0) return BADVAL;
//	if (x == 1) return RFVAL;
//	if (x == 2) return APFLAG;
//	if (x == 3) return NOECHO;
//	return BADVAL;
//}

/* Convert a sweep to an 2d array of data points */
static void bscan_sweep(Sweep *sweep, guint8 **data, int *width, int *height)
{
	/* Calculate max number of bins */
	int i, max_bins = 0;
	for (i = 0; i < sweep->h.nrays; i++)
		max_bins = MAX(max_bins, sweep->ray[i]->h.nbins);

	/* Allocate buffer using max number of bins for each ray */
	guint8 *buf = g_malloc0(sweep->h.nrays * max_bins * 4);

	/* Fill the data */
	int ri, bi;
	for (ri = 0; ri < sweep->h.nrays; ri++) {
		Ray *ray  = sweep->ray[ri];
		for (bi = 0; bi < ray->h.nbins; bi++) {
			/* copy RGBA into buffer */
			//guint val   = dz_f(ray->range[bi]);
			guint val   = ray->h.f(ray->range[bi]);
			guint buf_i = (ri*max_bins+bi)*4;
			buf[buf_i+0] =   red[val];
			buf[buf_i+1] = green[val];
			buf[buf_i+2] =  blue[val];
			buf[buf_i+3] = get_alpha(val);
		}
	}

	/* set output */
	*width  = max_bins;
	*height = sweep->h.nrays;
	*data   = buf;
}

/* Load a sweep as the active texture */
static void load_sweep(Sweep *sweep)
{
	cur_sweep = sweep;
	int height, width;
	guint8 *data;
	bscan_sweep(sweep, &data, &width, &height);
	glGenTextures(1, &sweep_tex);
	glBindTexture(GL_TEXTURE_2D, sweep_tex);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	g_free(data);
	gdk_window_invalidate_rect(drawing->window, &drawing->allocation, FALSE);
	gdk_window_process_updates(drawing->window, FALSE);
}

/* Load the default sweep */
static gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	Sweep *first = (Sweep*)user_data;
	if (cur_sweep == NULL)
		load_sweep(first);
	return FALSE;
}

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	g_message("radar:expose");
	Sweep *sweep = cur_sweep;

	/* Draw the rays */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, sweep_tex);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	glBegin(GL_QUAD_STRIP);
	for (int ri = 0; ri <= sweep->h.nrays+1; ri++) {
		/* Do the first sweep twice to complete the last Quad */
		Ray *ray = sweep->ray[ri % sweep->h.nrays];

		/* right and left looking out from radar */
		double left  = ((ray->h.azimuth - ((double)ray->h.beam_width/2.))*M_PI)/180.0; 
		//double right = ((ray->h.azimuth + ((double)ray->h.beam_width/2.))*M_PI)/180.0; 

		double lx = sin(left);
		double ly = cos(left);

		double near_dist = ray->h.range_bin1;
		double far_dist  = ray->h.nbins*ray->h.gate_size + ray->h.range_bin1;

		/* (find middle of bin) / scale for opengl */
		// near left
		glTexCoord2f(0.0, (double)ri/sweep->h.nrays);
		glVertex3f(lx*near_dist, ly*near_dist, 2.0);

		// far  left
		glTexCoord2f(1.0, (double)ri/sweep->h.nrays);
		glVertex3f(lx*far_dist,  ly*far_dist,  2.0);
	}
	//g_print("ri=%d, nr=%d, bw=%f\n", _ri, sweep->h.nrays, sweep->h.beam_width);
	glEnd();
	glPopMatrix();

	/* Texture debug */
	//glBegin(GL_QUADS);
	//glTexCoord2d( 0.,  0.); glVertex3f(-1.,  0., 0.); // bot left
	//glTexCoord2d( 0.,  1.); glVertex3f(-1.,  1., 0.); // top left
	//glTexCoord2d( 1.,  1.); glVertex3f( 0.,  1., 0.); // top right
	//glTexCoord2d( 1.,  0.); glVertex3f( 0.,  0., 0.); // bot right
	//glEnd();

	/* Print the color table */
	//glDisable(GL_TEXTURE_2D);
	//glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();
	//glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	//glBegin(GL_QUADS);
	//int i;
	//for (i = 0; i < nred; i++) {
	//	glColor4ub(red[i], green[i], blue[i], get_alpha(i));
	//	glVertex3f(-1.0, (float)((i  ) - nred/2)/(nred/2), 0.0); // bot left
	//	glVertex3f(-1.0, (float)((i+1) - nred/2)/(nred/2), 0.0); // top left
	//	glVertex3f(-0.9, (float)((i+1) - nred/2)/(nred/2), 0.0); // top right
	//	glVertex3f(-0.9, (float)((i  ) - nred/2)/(nred/2), 0.0); // bot right
	//}
	//glEnd();
        //glMatrixMode(GL_PROJECTION); glPopMatrix(); 
	//glMatrixMode(GL_MODELVIEW ); glPopMatrix();

	return FALSE;
}

gboolean radar_init(GtkDrawingArea *_drawing, GtkNotebook *config)
{
	drawing = GTK_WIDGET(_drawing);

	/* Parse hard coded file.. */
	RSL_read_these_sweeps("all", NULL);
	//RSL_read_these_sweeps("all", NULL);
	Radar *radar = RSL_wsr88d_to_radar("/scratch/aweather/data/level2/KNQA_20090501_1925.raw", "KNQA");
	RSL_load_refl_color_table();
	RSL_get_color_table(RSL_RED_TABLE,   red,   &nred);
	RSL_get_color_table(RSL_GREEN_TABLE, green, &ngreen);
	RSL_get_color_table(RSL_BLUE_TABLE,  blue,  &nblue);
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1)
		g_print("No sweeps found\n");

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

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(expose),    NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(configure), radar->v[0]->sweep[0]);

	return TRUE;
}
