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
#include <rsl.h>

#include "aweather-gui.h"
#include "plugin-radar.h"
#include "data.h"

/****************
 * GObject code *
 ****************/
/* Plugin init */
static void aweather_radar_plugin_init(AWeatherPluginInterface *iface);
static void _aweather_radar_expose(AWeatherPlugin *_radar);
G_DEFINE_TYPE_WITH_CODE(AWeatherRadar, aweather_radar, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(AWEATHER_TYPE_PLUGIN,
			aweather_radar_plugin_init));
static void aweather_radar_plugin_init(AWeatherPluginInterface *iface)
{
	g_debug("AWeatherRadar: plugin_init");
	/* Add methods to the interface */
	iface->expose = _aweather_radar_expose;
}
/* Class/Object init */
static void aweather_radar_init(AWeatherRadar *radar)
{
	g_debug("AWeatherRadar: class_init");
	/* Set defaults */
	radar->gui = NULL;
}
static void aweather_radar_dispose(GObject *gobject)
{
	g_debug("AWeatherRadar: dispose");
	AWeatherRadar *self = AWEATHER_RADAR(gobject);
	/* Drop references */
	G_OBJECT_CLASS(aweather_radar_parent_class)->dispose(gobject);
}
static void aweather_radar_finalize(GObject *gobject)
{
	g_debug("AWeatherRadar: finalize");
	AWeatherRadar *self = AWEATHER_RADAR(gobject);
	/* Free data */
	G_OBJECT_CLASS(aweather_radar_parent_class)->finalize(gobject);

}
static void aweather_radar_class_init(AWeatherRadarClass *klass)
{
	g_debug("AWeatherRadar: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = aweather_radar_dispose;
	gobject_class->finalize = aweather_radar_finalize;
}

/**************************
 * Data loading functions *
 **************************/
/* Convert a sweep to an 2d array of data points */
static void bscan_sweep(AWeatherRadar *self, Sweep *sweep, colormap_t *colormap,
		guint8 **data, int *width, int *height)
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

/* Load a sweep as the active texture */
static void load_sweep(AWeatherRadar *self, Sweep *sweep)
{
	aweather_gui_gl_begin(self->gui);
	self->cur_sweep = sweep;
	int height, width;
	guint8 *data;
	bscan_sweep(self, sweep, self->cur_colormap, &data, &width, &height);
	glDeleteTextures(1, &self->cur_sweep_tex);
	glGenTextures(1, &self->cur_sweep_tex);
	glBindTexture(GL_TEXTURE_2D, self->cur_sweep_tex);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, data);
	g_free(data);
	aweather_gui_gl_redraw(self->gui);
	aweather_gui_gl_end(self->gui);
}

static void load_colormap(AWeatherRadar *self, gchar *table)
{
	/* Set colormap so we can draw it on expose */
	for (int i = 0; colormaps[i].name; i++)
		if (g_str_equal(colormaps[i].name, table))
			self->cur_colormap = &colormaps[i];
}

/* Add selectors to the config area for the sweeps */
static void on_sweep_clicked(GtkRadioButton *button, gpointer _self);
static void load_radar_gui(AWeatherRadar *self, Radar *radar)
{
	/* Clear existing items */
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
	if (child)
		gtk_widget_destroy(child);

	gdouble elev;
	guint rows = 1, cols = 1, cur_cols;
	gchar row_label_str[64], col_label_str[64], button_str[64];
	GtkWidget *row_label, *col_label, *button = NULL, *elev_box;
	GtkWidget *table = gtk_table_new(rows, cols, FALSE);

	for (guint vi = 0; vi < radar->h.nvolumes; vi++) {
		Volume *vol = radar->v[vi];
		if (vol == NULL) continue;
		rows++; cols = 1; elev = 0;

		/* Row label */
		g_snprintf(row_label_str, 64, "<b>%s:</b>", vol->h.type_str);
		row_label = gtk_label_new(row_label_str);
		gtk_label_set_use_markup(GTK_LABEL(row_label), TRUE);
		gtk_misc_set_alignment(GTK_MISC(row_label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), row_label,
				0,1, rows-1,rows, GTK_FILL,GTK_FILL, 5,0);

		for (guint si = 0; si < vol->h.nsweeps; si++) {
			Sweep *sweep = vol->sweep[si];
			if (sweep == NULL || sweep->h.elev == 0) continue;
			if (sweep->h.elev != elev) {
				cols++;
				elev = sweep->h.elev;

				/* Column label */
				g_object_get(table, "n-columns", &cur_cols, NULL);
				if (cols >  cur_cols) {
					g_snprintf(col_label_str, 64, "<b>%.2f°</b>", elev);
					col_label = gtk_label_new(col_label_str);
					gtk_label_set_use_markup(GTK_LABEL(col_label), TRUE);
					gtk_widget_set_size_request(col_label, 40, -1);
					gtk_table_attach(GTK_TABLE(table), col_label,
							cols-1,cols, 0,1, GTK_FILL,GTK_FILL, 0,0);
				}

				elev_box = gtk_hbox_new(TRUE, 0);
				gtk_table_attach(GTK_TABLE(table), elev_box,
						cols-1,cols, rows-1,rows, GTK_FILL,GTK_FILL, 0,0);
			}


			/* Button */
			g_snprintf(button_str, 64, "%3.2f", elev);
			button = gtk_radio_button_new_with_label_from_widget(
					GTK_RADIO_BUTTON(button), button_str);
			gtk_widget_set_size_request(button, -1, 26);
			//button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(button));
			//gtk_widget_set_size_request(button, -1, 22);
			g_object_set(button, "draw-indicator", FALSE, NULL);
			gtk_box_pack_end(GTK_BOX(elev_box), button, TRUE, TRUE, 0);

			g_object_set_data(G_OBJECT(button), "type",  vol->h.type_str);
			g_object_set_data(G_OBJECT(button), "sweep", sweep);
			g_signal_connect(button, "clicked", G_CALLBACK(on_sweep_clicked), self);
		}
	}
	gtk_container_add(GTK_CONTAINER(self->config_body), table);
	gtk_widget_show_all(table);
}

/* Load a radar from a decompressed file */
static void load_radar(AWeatherRadar *self, gchar *radar_file)
{
	char *dir  = g_path_get_dirname(radar_file);
	char *site = g_path_get_basename(dir);
	g_free(dir);
	g_debug("AWeatherRadar: load_radar - Loading new radar");
	RSL_read_these_sweeps("all", NULL);
	Radar *radar = self->cur_radar = RSL_wsr88d_to_radar(radar_file, site);
	if (radar == NULL) {
		g_warning("fail to load radar: path=%s, site=%s", radar_file, site);
		g_free(site);
		return;
	}
	g_free(site);

	/* Load the first sweep by default */
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1) {
		g_warning("No sweeps found\n");
	} else {
		/* load first available sweep */
		for (int vi = 0; vi < radar->h.nvolumes; vi++) {
			if (radar->v[vi]== NULL) continue;
			for (int si = 0; si < radar->v[vi]->h.nsweeps; si++) {
				if (radar->v[vi]->sweep[si]== NULL) continue;
				load_colormap(self, radar->v[vi]->h.type_str);
				load_sweep(self, radar->v[vi]->sweep[si]);
				break;
			}
			break;
		}
	}

	load_radar_gui(self, radar);
}

static void update_times(AWeatherRadar *self, char *site, char **last_time)
{
	char *list_uri = g_strdup_printf(
			"http://mesonet.agron.iastate.edu/data/nexrd2/raw/K%s/dir.list",
			site);
	GFile *list    = g_file_new_for_uri(list_uri);
	g_free(list_uri);

	gchar *data;
	gsize length;
	GError *error = NULL;
	g_file_load_contents(list, NULL, &data, &length, NULL, &error);
	g_object_unref(list);
	if (error) {
		g_warning("Error loading list for %s: %s", site, error->message);
		g_error_free(error);
		return;
	}
	gchar **lines = g_strsplit(data, "\n", -1);
	GtkTreeView  *tview  = GTK_TREE_VIEW(aweather_gui_get_widget(self->gui, "time"));
	GtkListStore *lstore = GTK_LIST_STORE(gtk_tree_view_get_model(tview));
	gtk_list_store_clear(lstore);
	GtkTreeIter iter;
	for (int i = 0; lines[i] && lines[i][0]; i++) {
		// format: `841907 KABR_20090510_0159'
		//g_message("\tadding %p [%s]", lines[i], lines[i]);
		char **parts = g_strsplit(lines[i], " ", 2);
		char *time = parts[1]+5;
		gtk_list_store_insert(lstore, &iter, 0);
		gtk_list_store_set(lstore, &iter, 0, time, -1);
		g_strfreev(parts);
	}

	if (last_time)
		gtk_tree_model_get(GTK_TREE_MODEL(lstore), &iter, 0, last_time, -1);

	g_free(data);
	g_strfreev(lines);
}

/*****************
 * ASync helpers *
 *****************/
typedef struct {
	AWeatherRadar *self;
	gchar *radar_file;
} decompressed_t;

static void decompressed_cb(GPid pid, gint status, gpointer _udata)
{
	decompressed_t *udata = _udata;
	if (status != 0) {
		g_warning("wsr88ddec exited with status %d", status);
		return;
	}
	load_radar(udata->self, udata->radar_file);
	g_spawn_close_pid(pid);
	g_free(udata->radar_file);
	g_free(udata);
}

static void cached_cb(char *path, gboolean updated, gpointer _self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	char *decompressed = g_strconcat(path, ".raw", NULL);
	if (!updated) {
		load_radar(self, decompressed);
		return;
	}

	decompressed_t *udata = g_malloc(sizeof(decompressed_t));
	udata->self       = self;
	udata->radar_file = decompressed;
	g_debug("AWeatherRadar: cached_cb - File updated, decompressing..");
	char *argv[] = {"wsr88ddec", path, decompressed, NULL};
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
	if (error) {
		g_warning("failed to decompress WSR88D data: %s",
				error->message);
		g_error_free(error);
	}
	g_child_watch_add(pid, decompressed_cb, udata);
}

/*************
 * Callbacks *
 *************/
static void on_sweep_clicked(GtkRadioButton *button, gpointer _self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	load_colormap(self, g_object_get_data(G_OBJECT(button), "type" ));
	load_sweep   (self, g_object_get_data(G_OBJECT(button), "sweep"));
}

static void on_time_changed(AWeatherView *view, char *time, gpointer _self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	g_debug("AWeatherRadar: on_time_changed - setting time");
	// format: http://mesonet.agron.iastate.edu/data/nexrd2/raw/KABR/KABR_20090510_0323
	char *site = aweather_view_get_site(view);
	char *base = "http://mesonet.agron.iastate.edu/data/";
	char *path = g_strdup_printf("nexrd2/raw/K%s/K%s_%s", site, site, time);

	/* Clear out children */
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
	if (child)
		gtk_widget_destroy(child);
	gtk_container_add(GTK_CONTAINER(self->config_body),
		gtk_label_new("Loading radar..."));
	gtk_widget_show_all(self->config_body);
	if (self->cur_radar)
		RSL_free_radar(self->cur_radar);
	self->cur_radar = NULL;
	self->cur_sweep = NULL;
	aweather_gui_gl_redraw(self->gui);

	/* Start loading the new radar */
	cache_file(base, path, AWEATHER_AUTOMATIC, cached_cb, self);
	g_free(path);
}

static void on_site_changed(AWeatherView *view, char *site, gpointer _self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	g_debug("AWeatherRadar: on_site_changed - Loading wsr88d list for %s", site);
	char *time = NULL;
	update_times(self, site, &time);
	aweather_view_set_time(view, time);

	g_free(time);
}

static void on_refresh(AWeatherView *view, gpointer _self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	char *site = aweather_view_get_site(view);
	char *time = NULL;
	update_times(self, site, &time);
	aweather_view_set_time(view, time);
	g_free(time);
}

/***********
 * Methods *
 ***********/
AWeatherRadar *aweather_radar_new(AWeatherGui *gui)
{
	g_debug("AWeatherRadar: new");
	AWeatherRadar *self = g_object_new(AWEATHER_TYPE_RADAR, NULL);
	self->gui = gui;

	GtkWidget    *config  = aweather_gui_get_widget(gui, "tabs");
	AWeatherView *view    = aweather_gui_get_view(gui);

	/* Add configuration tab */
	self->config_body = gtk_alignment_new(0, 0, 1, 1);
	gtk_container_set_border_width(GTK_CONTAINER(self->config_body), 5);
	gtk_container_add(GTK_CONTAINER(self->config_body), gtk_label_new("No radar loaded"));
	gtk_notebook_prepend_page(GTK_NOTEBOOK(config), self->config_body, gtk_label_new("Radar"));

	/* Set up OpenGL Stuff */
	g_signal_connect(view,    "site-changed", G_CALLBACK(on_site_changed), self);
	g_signal_connect(view,    "time-changed", G_CALLBACK(on_time_changed), self);
	g_signal_connect(view,    "refresh",      G_CALLBACK(on_refresh),      self);

	return self;
}

static void _aweather_radar_expose(AWeatherPlugin *_self)
{
	AWeatherRadar *self = AWEATHER_RADAR(_self);
	g_debug("AWeatherRadar: expose");
	if (self->cur_sweep == NULL)
		return;
	Sweep *sweep = self->cur_sweep;

	/* Draw the rays */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, self->cur_sweep_tex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1,1,1,1);
	glBegin(GL_QUAD_STRIP);
	for (int ri = 0; ri <= sweep->h.nrays; ri++) {
		Ray  *ray = NULL;
		double angle = 0;
		if (ri < sweep->h.nrays) {
			ray = sweep->ray[ri];
			angle = ((ray->h.azimuth - ((double)ray->h.beam_width/2.))*M_PI)/180.0; 
		} else {
			/* Do the right side of the last sweep */
			ray = sweep->ray[ri-1];
			angle = ((ray->h.azimuth + ((double)ray->h.beam_width/2.))*M_PI)/180.0; 
		}

		double lx = sin(angle);
		double ly = cos(angle);

		double near_dist = ray->h.range_bin1;
		double far_dist  = ray->h.nbins*ray->h.gate_size + ray->h.range_bin1;

		/* (find middle of bin) / scale for opengl */
		// near left
		glTexCoord2f(0.0, (double)ri/sweep->h.nrays-0.01);
		glVertex3f(lx*near_dist, ly*near_dist, 2.0);

		// far  left
		glTexCoord2f(1.0, (double)ri/sweep->h.nrays-0.01);
		glVertex3f(lx*far_dist,  ly*far_dist,  2.0);
	}
	//g_print("ri=%d, nr=%d, bw=%f\n", _ri, sweep->h.nrays, sweep->h.beam_width);
	glEnd();
	glPopMatrix();

	/* Texture debug */
	//glBegin(GL_QUADS);
	//glTexCoord2d( 0.,  0.); glVertex3f(-500.,   0., 0.); // bot left
	//glTexCoord2d( 0.,  1.); glVertex3f(-500., 500., 0.); // top left
	//glTexCoord2d( 1.,  1.); glVertex3f( 0.,   500., 3.); // top right
	//glTexCoord2d( 1.,  0.); glVertex3f( 0.,     0., 3.); // bot right
	//glEnd();

	/* Print the color table */
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glBegin(GL_QUADS);
	int i;
	for (i = 0; i < 256; i++) {
		glColor4ub(self->cur_colormap->data[i][0],
		           self->cur_colormap->data[i][1],
		           self->cur_colormap->data[i][2],
		           self->cur_colormap->data[i][3]);
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
}
