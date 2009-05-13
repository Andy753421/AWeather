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
#include <math.h>

#include "rsl.h"

#include "aweather-gui.h"
#include "plugin-radar.h"
#include "data.h"

GtkWidget *drawing;
GtkWidget *config_body;
static Sweep *cur_sweep = NULL;  // make this not global
//static int nred, ngreen, nblue;
//static char red[256], green[256], blue[256];
static colormap_t *colormap;
static guint sweep_tex = 0;

static AWeatherGui *gui = NULL;
static Radar *radar = NULL;

/**************************
 * Data loading functions *
 **************************/
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
			guint8 val   = (guint8)ray->h.f(ray->range[bi]);
			guint  buf_i = (ri*max_bins+bi)*4;
			buf[buf_i+0] = colormap->data[val][0];
			buf[buf_i+1] = colormap->data[val][1];
			buf[buf_i+2] = colormap->data[val][2];
			buf[buf_i+3] = colormap->data[val][3];
			if (val == BADVAL     || val == RFVAL      || val == APFLAG ||
			    val == NOTFOUND_H || val == NOTFOUND_V || val == NOECHO) {
				buf[buf_i+3] = 0x00; // transparent
			}
		}
	}

	/* set output */
	*width  = max_bins;
	*height = sweep->h.nrays;
	*data   = buf;
}

static void load_color_table(char *table)
{
	for (int i = 0; colormaps[i].name; i++)
		if (g_str_equal(colormaps[i].name, table))
			colormap = &colormaps[i];
}

/* Load a sweep as the active texture */
static void load_sweep(Sweep *sweep)
{
	aweather_gui_gl_begin(gui);
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
	gtk_widget_queue_draw(aweather_gui_get_widget(gui, "drawing"));
	aweather_gui_gl_end(gui);
}

/* Add selectors to the config area for the sweeps */
static void load_radar_gui(Radar *radar)
{
	/* Clear existing items */
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(config_body));
	if (child)
		gtk_widget_destroy(child);

	GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
	GtkWidget *button = NULL;
	int vi = 0, si = 0;
	for (vi = 0; vi < radar->h.nvolumes; vi++) {
		Volume *vol = radar->v[vi];
		if (vol == NULL) continue;
		GtkWidget *vbox = gtk_vbox_new(TRUE, 0);
		for (si = vol->h.nsweeps-1; si >= 0; si--) {
			Sweep *sweep = vol->sweep[si];
			if (sweep == NULL) continue;
			char *label = g_strdup_printf("Tilt: %.2f (%s)", sweep->h.elev, vol->h.type_str);
			button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), label);
			g_object_set(button, "draw-indicator", FALSE, NULL);
			g_signal_connect_swapped(button, "clicked", G_CALLBACK(load_color_table), vol->h.type_str);
			g_signal_connect_swapped(button, "clicked", G_CALLBACK(load_sweep), sweep);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);
			g_free(label);
		}
		gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	}
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(config_body), hbox);
	gtk_widget_show_all(hbox);
}

/* Load a radar from a file */
static void load_radar_rsl(GPid pid, gint status, gpointer _path)
{
	gchar *path = _path;
	if (status != 0) {
		g_warning("wsr88ddec exited with status %d", status);
		return;
	}
	char *site = g_path_get_basename(g_path_get_dirname(path));
	if (radar) RSL_free_radar(radar);
	RSL_read_these_sweeps("all", NULL);
	radar = RSL_wsr88d_to_radar(path, site);
	if (radar == NULL) {
		g_warning("fail to load radar: path=%s, site=%s", path, site);
		return;
	}

	/* Load the first sweep by default */
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1) {
		g_warning("No sweeps found\n");
	} else {
		/* load first available sweep */
		for (int vi = 0; vi < radar->h.nvolumes; vi++) {
			if (radar->v[vi]== NULL) continue;
			for (int si = 0; si < radar->v[vi]->h.nsweeps; si++) {
				if (radar->v[vi]->sweep[si]== NULL) continue;
				load_color_table(radar->v[vi]->h.type_str);
				load_sweep(radar->v[vi]->sweep[si]);
				break;
			}
			break;
		}
	}

	load_radar_gui(radar);
}

/* decompress a radar file, then chain to the actuall loading function */
static void load_radar(char *path, gpointer user_data)
{
	char *raw  = g_strconcat(path, ".raw", NULL);
	if (g_file_test(raw, G_FILE_TEST_EXISTS)) {
		load_radar_rsl(0, 0, raw);
	} else {
		char *argv[] = {"wsr88ddec", path, raw, NULL};
		GPid pid;
		GError *error = NULL;
		g_spawn_async(
			NULL,    // const gchar *working_directory,
			argv,    // gchar **argv,
			NULL,    // gchar **envp,
			G_SPAWN_SEARCH_PATH|
			G_SPAWN_DO_NOT_REAP_CHILD, 
			         // GSpawnFlags flags,
			NULL,    // GSpawnChildSetupFunc child_setup,
			NULL,    // gpointer user_data,
			&pid,    // GPid *child_pid,
			&error); // GError **error
		if (error)
			g_warning("failed to decompress WSR88D data: %s", error->message);
		g_child_watch_add(pid, load_radar_rsl, raw);
	}
}

/*************
 * Callbacks *
 *************/
static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	g_message("radar:expose");
	if (cur_sweep == NULL)
		return FALSE;
	Sweep *sweep = cur_sweep;

	/* Draw the rays */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, sweep_tex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
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
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glBegin(GL_QUADS);
	int i;
	for (i = 0; i < 256; i++) {
		glColor4ub(colormap->data[i][0],
		           colormap->data[i][1],
		           colormap->data[i][2],
		           colormap->data[i][3]);
		glVertex3f(-1.0, (float)((i  ) - 256/2)/(256/2), 0.0); // bot left
		glVertex3f(-1.0, (float)((i+1) - 256/2)/(256/2), 0.0); // top left
		glVertex3f(-0.9, (float)((i+1) - 256/2)/(256/2), 0.0); // top right
		glVertex3f(-0.9, (float)((i  ) - 256/2)/(256/2), 0.0); // bot right
	}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
        glMatrixMode(GL_PROJECTION); glPopMatrix(); 
	glMatrixMode(GL_MODELVIEW ); glPopMatrix();
	return FALSE;
}

static void set_time(AWeatherView *view, char *time, gpointer user_data)
{
	g_message("radar:setting time");
	// format: http://mesonet.agron.iastate.edu/data/nexrd2/raw/KABR/KABR_20090510_0323
	char *site = aweather_view_get_location(view);
	char *base = "http://mesonet.agron.iastate.edu/data/";
	char *path = g_strdup_printf("nexrd2/raw/K%s/K%s_%s", site, site, time);
	//g_message("caching %s/%s", base, path);
	cur_sweep = NULL; // Clear radar
	gtk_widget_queue_draw(aweather_gui_get_widget(gui, "drawing"));
	cache_file(base, path, load_radar, NULL);
}

static void set_site(AWeatherView *view, char *site, gpointer user_data)
{
	g_message("Loading wsr88d list for %s", site);
	gchar *data;
	gsize length;
	GError *error = NULL;
	char *list_uri = g_strdup_printf("http://mesonet.agron.iastate.edu/data/nexrd2/raw/K%s/dir.list", site);
	GFile *list    = g_file_new_for_uri(list_uri);
	cur_sweep = NULL; // Clear radar
	gtk_widget_queue_draw(aweather_gui_get_widget(gui, "drawing"));
	g_file_load_contents(list, NULL, &data, &length, NULL, &error);
	if (error) {
		g_warning("Error loading list for %s: %s", site, error->message);
		return;
	}
	gchar **lines = g_strsplit(data, "\n", -1);
	GtkTreeView  *tview  = GTK_TREE_VIEW(aweather_gui_get_widget(gui, "time"));
	GtkListStore *lstore = GTK_LIST_STORE(gtk_tree_view_get_model(tview));
	gtk_list_store_clear(lstore);
	radar = NULL;
	char *time = NULL;
	for (int i = 0; lines[i] && lines[i][0]; i++) {
		// format: `841907 KABR_20090510_0159'
		//g_message("\tadding %p [%s]", lines[i], lines[i]);
		char **parts = g_strsplit(lines[i], " ", 2);
		time = parts[1]+5;
		GtkTreeIter iter;
		gtk_list_store_insert(lstore, &iter, 0);
		gtk_list_store_set(lstore, &iter, 0, time, -1);
	}
	if (time != NULL) 
		aweather_view_set_time(view, time);
}

/* Init */
gboolean radar_init(AWeatherGui *_gui)
{
	gui = _gui;
	drawing = aweather_gui_get_widget(gui, "drawing");
	GtkNotebook    *config  = GTK_NOTEBOOK(aweather_gui_get_widget(gui, "tabs"));
	AWeatherView   *view    = aweather_gui_get_view(gui);

	/* Add configuration tab */
	config_body = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(config_body), gtk_label_new("No radar loaded"));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(config_body), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_notebook_append_page(GTK_NOTEBOOK(config), config_body, gtk_label_new("Radar"));

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",     G_CALLBACK(expose),   NULL);
	g_signal_connect(view,    "location-changed", G_CALLBACK(set_site), NULL);
	g_signal_connect(view,    "time-changed",     G_CALLBACK(set_time), NULL);

	return TRUE;
}
