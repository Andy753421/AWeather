/*
 * Copyright (C) 2009-2010 Andy Spencer <andy753421@gmail.com>
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
#include <gio/gio.h>
#include <GL/gl.h>
#include <math.h>
#include <rsl.h>

#include <gis.h>

#include "radar.h"
#include "marching.h"
#include "../aweather-location.h"


/**************************
 * Data loading functions *
 **************************/
/* Convert a sweep to an 2d array of data points */
static void _bscan_sweep(GisPluginRadar *self, Sweep *sweep, colormap_t *colormap,
		guint8 **data, int *width, int *height)
{
	/* Calculate max number of bins */
	int max_bins = 0;
	for (int i = 0; i < sweep->h.nrays; i++)
		max_bins = MAX(max_bins, sweep->ray[i]->h.nbins);

	/* Allocate buffer using max number of bins for each ray */
	guint8 *buf = g_malloc0(sweep->h.nrays * max_bins * 4);

	/* Fill the data */
	for (int ri = 0; ri < sweep->h.nrays; ri++) {
		Ray *ray  = sweep->ray[ri];
		for (int bi = 0; bi < ray->h.nbins; bi++) {
			/* copy RGBA into buffer */
			//guint val   = dz_f(ray->range[bi]);
			guint8 val   = (guint8)ray->h.f(ray->range[bi]);
			guint  buf_i = (ri*max_bins+bi)*4;
			buf[buf_i+0] = colormap->data[val][0];
			buf[buf_i+1] = colormap->data[val][1];
			buf[buf_i+2] = colormap->data[val][2];
			buf[buf_i+3] = colormap->data[val][3]*0.75; // TESTING
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
static void _load_sweep(GisPluginRadar *self, Sweep *sweep)
{
	GisViewer *viewer = self->viewer;
	gis_viewer_begin(viewer);
	self->cur_sweep = sweep;
	int height, width;
	guint8 *data;
	_bscan_sweep(self, sweep, self->cur_colormap, &data, &width, &height);
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
	gtk_widget_queue_draw(GTK_WIDGET(viewer));
	gis_viewer_end(viewer);
}

static void _load_colormap(GisPluginRadar *self, gchar *table)
{
	/* Set colormap so we can draw it on expose */
	for (int i = 0; colormaps[i].name; i++)
		if (g_str_equal(colormaps[i].name, table))
			self->cur_colormap = &colormaps[i];
}

/* Add selectors to the config area for the sweeps */
static void _on_sweep_clicked(GtkRadioButton *button, gpointer _self);
static void _load_radar_gui(GisPluginRadar *self, Radar *radar)
{
	/* Clear existing items */
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
	if (child)
		gtk_widget_destroy(child);

	gdouble elev;
	guint rows = 1, cols = 1, cur_cols;
	gchar row_label_str[64], col_label_str[64], button_str[64];
	GtkWidget *row_label, *col_label, *button = NULL, *elev_box = NULL;
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
					gtk_widget_set_size_request(col_label, 50, -1);
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
			g_signal_connect(button, "clicked", G_CALLBACK(_on_sweep_clicked), self);
		}
	}
	gtk_container_add(GTK_CONTAINER(self->config_body), table);
	gtk_widget_show_all(table);
}

static void _gis_plugin_radar_grid_set(GRIDCELL *grid, int gi, Ray *ray, int bi)
{
	Range range = ray->range[bi];

	double angle = deg2rad(ray->h.azimuth);
	double tilt  = deg2rad(ray->h.elev);

	double lx    = sin(angle);
	double ly    = cos(angle);
	double lz    = sin(tilt);

	double dist   = bi*ray->h.gate_size + ray->h.range_bin1;

	grid->p[gi].x = lx*dist;
	grid->p[gi].y = ly*dist;
	grid->p[gi].z = lz*dist;

	guint8 val = (guint8)ray->h.f(ray->range[bi]);
	if (val == BADVAL     || val == RFVAL      || val == APFLAG ||
	    val == NOTFOUND_H || val == NOTFOUND_V || val == NOECHO ||
	    val > 80)
		val = 0;
	grid->val[gi] = (float)val;
	//g_debug("(%.2f,%.2f,%.2f) - (%.0f,%.0f,%.0f) = %d",
	//	angle, tilt, dist,
	//	grid->p[gi].x,
	//	grid->p[gi].y,
	//	grid->p[gi].z,
	//	val);
}

/* Load a radar from a decompressed file */
static void _load_radar(GisPluginRadar *self, gchar *radar_file)
{
	char *dir  = g_path_get_dirname(radar_file);
	char *site = g_path_get_basename(dir);
	g_free(dir);
	g_debug("GisPluginRadar: load_radar - Loading new radar");
	RSL_read_these_sweeps("all", NULL);
	Radar *radar = self->cur_radar = RSL_wsr88d_to_radar(radar_file, site);
	if (radar == NULL) {
		g_warning("fail to load radar: path=%s, site=%s", radar_file, site);
		g_free(site);
		return;
	}
	g_free(site);

#ifdef MARCHING
	/* Load the surface */
	if (self->cur_triangles) {
		g_free(self->cur_triangles);
		self->cur_triangles = NULL;
	}
	self->cur_num_triangles = 0;
	int x = 1;
	for (guint vi = 0; vi < radar->h.nvolumes; vi++) {
		if (radar->v[vi] == NULL) continue;

		for (guint si = 0; si+1 < radar->v[vi]->h.nsweeps; si++) {
			Sweep *sweep0 = radar->v[vi]->sweep[si+0];
			Sweep *sweep1 = radar->v[vi]->sweep[si+1];

			//g_debug("GisPluginRadar: load_radar: sweep[%3d-%3d] -- nrays = %d, %d",
			//	si, si+1,sweep0->h.nrays, sweep1->h.nrays);

			/* Skip super->regular resolution switch for now */
			if (sweep0 == NULL || sweep0->h.elev == 0 ||
			    sweep1 == NULL || sweep1->h.elev == 0 ||
			    sweep0->h.nrays != sweep1->h.nrays)
				continue;

			/* We repack the arrays so that raysX[0] is always north, etc */
			Ray **rays0 = g_malloc0(sizeof(Ray*)*sweep0->h.nrays);
			Ray **rays1 = g_malloc0(sizeof(Ray*)*sweep1->h.nrays);

			for (guint ri = 0; ri < sweep0->h.nrays; ri++)
				rays0[(guint)(sweep0->ray[ri]->h.azimuth * sweep0->h.nrays / 360)] =
					sweep0->ray[ri];
			for (guint ri = 0; ri < sweep1->h.nrays; ri++)
				rays1[(guint)(sweep1->ray[ri]->h.azimuth * sweep1->h.nrays / 360)] =
					sweep1->ray[ri];

			for (guint ri = 0; ri+x < sweep0->h.nrays; ri+=x) {
				//g_debug("GisPluginRadar: load_radar - ray[%3d-%3d] -- nbins = %d, %d, %d, %d",
				//	ri, ri+x,
				//	rays0[ri  ]->h.nbins,
				//	rays0[ri+1]->h.nbins,
				//	rays1[ri  ]->h.nbins,
				//	rays1[ri+1]->h.nbins);

				for (guint bi = 0; bi+x < rays1[ri]->h.nbins; bi+=x) {
					GRIDCELL grid = {};
					_gis_plugin_radar_grid_set(&grid, 7, rays0[(ri  )%sweep0->h.nrays], bi+x);
					_gis_plugin_radar_grid_set(&grid, 6, rays0[(ri+x)%sweep0->h.nrays], bi+x);
					_gis_plugin_radar_grid_set(&grid, 5, rays0[(ri+x)%sweep0->h.nrays], bi  );
					_gis_plugin_radar_grid_set(&grid, 4, rays0[(ri  )%sweep0->h.nrays], bi  );
					_gis_plugin_radar_grid_set(&grid, 3, rays1[(ri  )%sweep0->h.nrays], bi+x);
					_gis_plugin_radar_grid_set(&grid, 2, rays1[(ri+x)%sweep0->h.nrays], bi+x);
					_gis_plugin_radar_grid_set(&grid, 1, rays1[(ri+x)%sweep0->h.nrays], bi  );
					_gis_plugin_radar_grid_set(&grid, 0, rays1[(ri  )%sweep0->h.nrays], bi  );
					
					TRIANGLE tris[10];
					int n = march_one_cube(grid, 40, tris);

					self->cur_triangles = g_realloc(self->cur_triangles,
						(self->cur_num_triangles+n)*sizeof(TRIANGLE));
					for (int i = 0; i < n; i++) {
						//g_debug("triangle: ");
						//g_debug("\t(%f,%f,%f)", tris[i].p[0].x, tris[i].p[0].y, tris[i].p[0].z);
						//g_debug("\t(%f,%f,%f)", tris[i].p[1].x, tris[i].p[1].y, tris[i].p[1].z);
						//g_debug("\t(%f,%f,%f)", tris[i].p[2].x, tris[i].p[2].y, tris[i].p[2].z);
						self->cur_triangles[self->cur_num_triangles+i] = tris[i];
					}
					self->cur_num_triangles += n;
					//g_debug(" ");
				}
			}
		}
		break; // Exit after first volume (reflectivity)
	}
#endif

	/* Load the first sweep by default */
	if (radar->h.nvolumes < 1 || radar->v[0]->h.nsweeps < 1) {
		g_warning("No sweeps found\n");
	} else {
		/* load first available sweep */
		for (int vi = 0; vi < radar->h.nvolumes; vi++) {
			if (radar->v[vi]== NULL) continue;
			for (int si = 0; si < radar->v[vi]->h.nsweeps; si++) {
				if (radar->v[vi]->sweep[si]== NULL) continue;
				_load_colormap(self, radar->v[vi]->h.type_str);
				_load_sweep(self, radar->v[vi]->sweep[si]);
				break;
			}
			break;
		}
	}

	_load_radar_gui(self, radar);
}

typedef struct {
	GisPluginRadar *self;
	gchar *radar_file;
} decompressed_t;

static void _decompressed_cb(GPid pid, gint status, gpointer _udata)
{
	g_debug("GisPluginRadar: decompressed_cb");
	decompressed_t *udata = _udata;
	if (status != 0) {
		g_warning("wsr88ddec exited with status %d", status);
		return;
	}
	_load_radar(udata->self, udata->radar_file);
	g_spawn_close_pid(pid);
	g_free(udata->radar_file);
	g_free(udata);
}

static void _cache_chunk_cb(char *path, goffset cur, goffset total, gpointer _self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	double percent = (double)cur/total;

	//g_debug("GisPluginRadar: cache_chunk_cb - %lld/%lld = %.2f%%",
	//		cur, total, percent*100);

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), MIN(percent, 1.0));

	gchar *msg = g_strdup_printf("Loading radar... %5.1f%% (%.2f/%.2f MB)",
			percent*100, (double)cur/1000000, (double)total/1000000);
	gtk_label_set_text(GTK_LABEL(self->progress_label), msg);
	g_free(msg);
}

static void _cache_done_cb(char *path, gboolean updated, gpointer _self)
{
	g_debug("GisPluginRadar: cache_done_cb - updated = %d", updated);
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	char *decompressed = g_strconcat(path, ".raw", NULL);
	if (!updated && g_file_test(decompressed, G_FILE_TEST_EXISTS)) {
		_load_radar(self, decompressed);
		return;
	}

	decompressed_t *udata = g_malloc(sizeof(decompressed_t));
	udata->self       = self;
	udata->radar_file = decompressed;
	g_debug("GisPluginRadar: cache_done_cb - File updated, decompressing..");
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
		gchar *message = g_strdup_printf("Unable to decompress WSR88D data: %s",
				error->message);
		g_warning("%s", message);
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
		if (child)
			gtk_widget_destroy(child);
		gtk_container_add(GTK_CONTAINER(self->config_body), gtk_label_new(message));
		gtk_widget_show_all(self->config_body);
		g_error_free(error);
		g_free(message);
	}
	g_child_watch_add(pid, _decompressed_cb, udata);
}

static void _set_radar(GisPluginRadar *self, const char *site, const char *time)
{
	if (!site || !time)
		return;
	g_debug("GisPluginRadar: set_radar - %s - %s", site, time);

	/* Set up progress bar */
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
	if (child) gtk_widget_destroy(child);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	self->progress_bar   = gtk_progress_bar_new();
	self->progress_label = gtk_label_new("Loading radar...");
	gtk_box_pack_start(GTK_BOX(vbox), self->progress_bar,   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), self->progress_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self->config_body), vbox);
	gtk_widget_show_all(self->config_body);

	/* Clear radar */
	if (self->cur_radar)
		RSL_free_radar(self->cur_radar);
	self->cur_radar = NULL;
	self->cur_sweep = NULL;
	gtk_widget_queue_draw(GTK_WIDGET(self->viewer));

	/* Start loading the new radar */
	soup_session_abort(self->http->soup);
	/* format: http://mesonet.agron.iastate.edu/data/nexrd2/raw/KABR/KABR_20090510_0323 */
	gchar *base  = gis_prefs_get_string(self->prefs, "aweather/nexrad_url", NULL);
	gchar *local = g_strdup_printf("%s/%s_%s", site, site, time);
	gchar *uri   = g_strconcat(base, "/", local, NULL);
	GisCacheType mode = gis_viewer_get_offline(self->viewer) ? GIS_LOCAL : GIS_UPDATE;
	gchar *path  = gis_http_fetch(self->http, uri, local, mode, _cache_chunk_cb, self);
	if (path) {
		_cache_done_cb(path, TRUE, self);
		g_free(path);
	}
}


/*************
 * Callbacks *
 *************/
static void _on_sweep_clicked(GtkRadioButton *button, gpointer _self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	_load_colormap(self, g_object_get_data(G_OBJECT(button), "type" ));
	_load_sweep   (self, g_object_get_data(G_OBJECT(button), "sweep"));
}

static void _on_time_changed(GisViewer *viewer, const char *time, gpointer _self)
{
	g_debug("GisPluginRadar: _on_time_changed");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	g_free(self->cur_time);
	self->cur_time = g_strdup(time);
	_set_radar(self, self->cur_site, self->cur_time);
}

static void _on_location_changed(GisViewer *viewer,
		gdouble lat, gdouble lon, gdouble elev,
		gpointer _self)
{
	g_debug("GisPluginRadar: _on_location_changed");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	gdouble min_dist = EARTH_R / 5;
	city_t *city, *min_city = NULL;
	for (city = cities; city->type; city++) {
		if (city->type != LOCATION_CITY)
			continue;
		gdouble city_loc[3] = {};
		gdouble eye_loc[3]  = {lat, lon, elev};
		lle2xyz(city->lat, city->lon, city->elev,
				&city_loc[0], &city_loc[1], &city_loc[2]);
		lle2xyz(lat, lon, elev,
				&eye_loc[0], &eye_loc[1], &eye_loc[2]);
		gdouble dist = distd(city_loc, eye_loc);
		if (dist < min_dist) {
			min_dist = dist;
			min_city = city;
		}
	}
	static city_t *last_city = NULL;
	if (min_city && min_city != last_city) {
		self->cur_site = min_city->code;
		_set_radar(self, self->cur_site, self->cur_time);
	}
	last_city = min_city;
}

static gpointer _draw_radar(GisCallback *callback, gpointer _self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);

	/* Draw wsr88d */
	if (self->cur_sweep == NULL)
		return NULL;
	g_debug("GisPluginRadar: _draw_radar");
	Sweep *sweep = self->cur_sweep;

#ifdef MARCHING
	/* Draw the surface */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	float light_ambient[]  = {0.1f, 0.1f, 0.0f};
	float light_diffuse[]  = {0.9f, 0.9f, 0.9f};
	float light_position[] = {-300000.0f, 500000.0f, 400000.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glColor4f(1,1,1,0.75);
	g_debug("ntri=%d", self->cur_num_triangles);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < self->cur_num_triangles; i++) {
		TRIANGLE t = self->cur_triangles[i];
		do_normal(t.p[0].x, t.p[0].y, t.p[0].z,
		          t.p[1].x, t.p[1].y, t.p[1].z,
		          t.p[2].x, t.p[2].y, t.p[2].z);
		glVertex3f(t.p[0].x, t.p[0].y, t.p[0].z);
		glVertex3f(t.p[1].x, t.p[1].y, t.p[1].z);
		glVertex3f(t.p[2].x, t.p[2].y, t.p[2].z);
	}
	glEnd();
	glPopMatrix();
#endif

	g_debug("GisPluginRadar: _draw_radar - setting camera");
	Radar_header *h = &self->cur_radar->h;
	gdouble lat  = (double)h->latd + (double)h->latm/60 + (double)h->lats/(60*60);
	gdouble lon  = (double)h->lond + (double)h->lonm/60 + (double)h->lons/(60*60);
	gdouble elev = h->height;
	gis_viewer_center_position(self->viewer, lat, lon, elev);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, -2.0);
	glColor4f(1,1,1,1);

	/* Draw the rays */
	glBindTexture(GL_TEXTURE_2D, self->cur_sweep_tex);
	g_message("Tex = %d", self->cur_sweep_tex);
	glBegin(GL_TRIANGLE_STRIP);
	for (int ri = 0; ri <= sweep->h.nrays; ri++) {
		Ray  *ray = NULL;
		double angle = 0;
		if (ri < sweep->h.nrays) {
			ray = sweep->ray[ri];
			angle = deg2rad(ray->h.azimuth - ((double)ray->h.beam_width/2.));
		} else {
			/* Do the right side of the last sweep */
			ray = sweep->ray[ri-1];
			angle = deg2rad(ray->h.azimuth + ((double)ray->h.beam_width/2.));
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
		// todo: correct range-height function
		double height = sin(deg2rad(ray->h.elev)) * far_dist;
		glTexCoord2f(1.0, (double)ri/sweep->h.nrays-0.01);
		glVertex3f(lx*far_dist,  ly*far_dist, height);
	}
	glEnd();
	//g_print("ri=%d, nr=%d, bw=%f\n", _ri, sweep->h.nrays, sweep->h.beam_width);

	/* Texture debug */
	//glBegin(GL_QUADS);
	//glTexCoord2d( 0.,  0.); glVertex3f(-500.,   0., 0.); // bot left
	//glTexCoord2d( 0.,  1.); glVertex3f(-500., 500., 0.); // top left
	//glTexCoord2d( 1.,  1.); glVertex3f( 0.,   500., 3.); // top right
	//glTexCoord2d( 1.,  0.); glVertex3f( 0.,     0., 3.); // bot right
	//glEnd();

	return NULL;
}

static gpointer _draw_hud(GisCallback *callback, gpointer _self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	if (self->cur_sweep == NULL)
		return NULL;
	g_debug("GisPluginRadar: _draw_hud");

	/* Print the color table */
	glMatrixMode(GL_MODELVIEW ); glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glBegin(GL_QUADS);
	int i;
	for (i = 0; i < 256; i++) {
		glColor4ubv(self->cur_colormap->data[i]);
		glVertex3f(-1.0, (float)((i  ) - 256/2)/(256/2), 0.0); // bot left
		glVertex3f(-1.0, (float)((i+1) - 256/2)/(256/2), 0.0); // top left
		glVertex3f(-0.9, (float)((i+1) - 256/2)/(256/2), 0.0); // top right
		glVertex3f(-0.9, (float)((i  ) - 256/2)/(256/2), 0.0); // bot right
	}
	glEnd();

	return NULL;
}


/***********
 * Methods *
 ***********/
GisPluginRadar *gis_plugin_radar_new(GisViewer *viewer, GisPrefs *prefs)
{
	/* TODO: move to constructor if possible */
	g_debug("GisPluginRadar: new");
	GisPluginRadar *self = g_object_new(GIS_TYPE_PLUGIN_RADAR, NULL);
	self->viewer = viewer;
	self->prefs  = prefs;
	self->time_changed_id = g_signal_connect(viewer, "time-changed",
			G_CALLBACK(_on_time_changed), self);
	self->location_changed_id = g_signal_connect(viewer, "location-changed",
			G_CALLBACK(_on_location_changed), self);

	for (city_t *city = cities; city->type; city++) {
		if (city->type != LOCATION_CITY)
			continue;
		g_debug("Adding marker for %s %s", city->code, city->label);
		GisMarker *marker = gis_marker_new(city->label);
		gis_point_set_lle(gis_object_center(GIS_OBJECT(marker)),
				city->lat, city->lon, city->elev);
		GIS_OBJECT(marker)->lod = EARTH_R/2;
		gis_viewer_add(self->viewer, GIS_OBJECT(marker), GIS_LEVEL_OVERLAY, FALSE);
	}

	/* Add renderers */
	GisCallback *radar_cb = gis_callback_new(_draw_radar, self);
	GisCallback *hud_cb   = gis_callback_new(_draw_hud, self);

	gis_viewer_add(viewer, GIS_OBJECT(radar_cb),    GIS_LEVEL_WORLD, TRUE);
	gis_viewer_add(viewer, GIS_OBJECT(hud_cb),      GIS_LEVEL_HUD,   FALSE);

	return self;
}

static GtkWidget *gis_plugin_radar_get_config(GisPlugin *_self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	return self->config_body;
}


/****************
 * GObject code *
 ****************/
/* Plugin init */
static void gis_plugin_radar_plugin_init(GisPluginInterface *iface);
G_DEFINE_TYPE_WITH_CODE(GisPluginRadar, gis_plugin_radar, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(GIS_TYPE_PLUGIN,
			gis_plugin_radar_plugin_init));
static void gis_plugin_radar_plugin_init(GisPluginInterface *iface)
{
	g_debug("GisPluginRadar: plugin_init");
	/* Add methods to the interface */
	iface->get_config = gis_plugin_radar_get_config;
}
/* Class/Object init */
static void gis_plugin_radar_init(GisPluginRadar *self)
{
	g_debug("GisPluginRadar: class_init");
	/* Set defaults */
	self->http = gis_http_new("/nexrad/level2/");
	self->config_body = gtk_alignment_new(0, 0, 1, 1);
	gtk_container_set_border_width(GTK_CONTAINER(self->config_body), 5);
	gtk_container_add(GTK_CONTAINER(self->config_body), gtk_label_new("No radar loaded"));
}
static void gis_plugin_radar_dispose(GObject *gobject)
{
	g_debug("GisPluginRadar: dispose");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(gobject);
	g_signal_handler_disconnect(self->viewer, self->time_changed_id);
	/* Drop references */
	G_OBJECT_CLASS(gis_plugin_radar_parent_class)->dispose(gobject);
}
static void gis_plugin_radar_finalize(GObject *gobject)
{
	g_debug("GisPluginRadar: finalize");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(gobject);
	/* Free data */
	gis_http_free(self->http);
	G_OBJECT_CLASS(gis_plugin_radar_parent_class)->finalize(gobject);

}
static void gis_plugin_radar_class_init(GisPluginRadarClass *klass)
{
	g_debug("GisPluginRadar: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = gis_plugin_radar_dispose;
	gobject_class->finalize = gis_plugin_radar_finalize;
}

