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
#include <math.h>
#include <GL/gl.h>
#include <glib/gstdio.h>
#include <gis.h>
#include <rsl.h>

#include "level2.h"


/**************************
 * Data loading functions *
 **************************/
/* Convert a sweep to an 2d array of data points */
static void _bscan_sweep(Sweep *sweep, AWeatherColormap *colormap,
		guint8 **data, int *width, int *height)
{
	g_debug("AWeatherLevel2: _bscan_sweep - %p, %p, %p",
			sweep, colormap, data);
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

/* Load a sweep into an OpenGL texture */
static void _load_sweep_gl(Sweep *sweep, AWeatherColormap *colormap, guint *tex)
{
	g_debug("AWeatherLevel2: _load_sweep_gl");
	int height, width;
	guint8 *data;
	_bscan_sweep(sweep, colormap, &data, &width, &height);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, data);
	g_free(data);
}

/* Decompress a radar file using wsr88dec */
static gboolean _decompress_radar(const gchar *file, const gchar *raw)
{
	g_debug("AWeatherLevel2: _decompress_radar - \n\t%s\n\t%s", file, raw);
	char *argv[] = {"wsr88ddec", (gchar*)file, (gchar*)raw, NULL};
	gint status;
	GError *error = NULL;
	g_spawn_sync(
		NULL,    // const gchar *working_directory
		argv,    // gchar **argv
		NULL,    // gchar **envp
		G_SPAWN_SEARCH_PATH, // GSpawnFlags flags
		NULL,    // GSpawnChildSetupFunc child_setup
		NULL,    // gpointer user_data
		NULL,    // gchar *standard_output
		NULL,    // gchar *standard_output
		&status, // gint *exit_status
		&error); // GError **error
	if (error) {
		g_warning("AWeatherLevel2: _decompress_radar - %s", error->message);
		g_error_free(error);
		return FALSE;
	}
	if (status != 0) {
		gchar *msg = g_strdup_printf("wsr88ddec exited with status %d", status);
		g_warning("AWeatherLevel2: _decompress_radar - %s", msg);
		g_free(msg);
		return FALSE;
	}
	return TRUE;
}


/*********************
 * Drawing functions *
 *********************/
static void _draw_radar(GisCallback *_self, gpointer _viewer)
{
	AWeatherLevel2 *self = AWEATHER_LEVEL2(_self);
	if (!self->sweep || !self->sweep_tex)
		return;

	/* Draw wsr88d */
	Sweep *sweep = self->sweep;
	Radar_header *h = &self->radar->h;
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
	glBindTexture(GL_TEXTURE_2D, self->sweep_tex);
	g_message("Tex = %d", self->sweep_tex);
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
}


/***********
 * Methods *
 ***********/
static gboolean _set_sweep_cb(gpointer _self)
{
	g_debug("AWeatherLevel2: _set_sweep_cb");
	AWeatherLevel2 *self = _self;
	if (self->sweep_tex)
		glDeleteTextures(1, &self->sweep_tex);
	_load_sweep_gl(self->sweep, self->sweep_colors, &self->sweep_tex);
	gtk_widget_queue_draw(GTK_WIDGET(self->viewer));
	return FALSE;
}
void aweather_level2_set_sweep(AWeatherLevel2 *self,
		int type, float elev)
{
	g_debug("AWeatherLevel2: set_sweep - %d %f", type, elev);

	/* Find sweep */
	Volume *volume = RSL_get_volume(self->radar, type);
	if (!volume) return;
	self->sweep = RSL_get_closest_sweep(volume, elev, 90);
	if (!self->sweep) return;

	/* Find colormap */
	self->sweep_colors = NULL;
	for (int i = 0; self->colormap[i].name; i++)
		if (self->colormap[i].type == type)
			self->sweep_colors = &self->colormap[i];
	if (!self->sweep_colors) return;

	/* Load data */
	g_idle_add(_set_sweep_cb, self);
}

AWeatherLevel2 *aweather_level2_new(GisViewer *viewer,
		AWeatherColormap *colormap, Radar *radar)
{
	g_debug("AWeatherLevel2: new");
	AWeatherLevel2 *self = g_object_new(AWEATHER_TYPE_LEVEL2, NULL);
	self->viewer   = viewer;
	self->radar    = radar;
	self->colormap = colormap;
	aweather_level2_set_sweep(self, DZ_INDEX, 0);
	return self;
}

AWeatherLevel2 *aweather_level2_new_from_file(GisViewer *viewer,
		AWeatherColormap *colormap,
		const gchar *file, const gchar *site)
{
	g_debug("AWeatherLevel2: new_from_file %s %s", site, file);

	/* Decompress radar */
	gchar *raw = g_strconcat(file, ".raw", NULL);
	if (g_file_test(raw, G_FILE_TEST_EXISTS)) {
		struct stat files, raws;
		g_stat(file, &files);
		g_stat(raw,  &raws);
		if (files.st_mtime < raws.st_mtime)
			if (!_decompress_radar(file, raw))
				return NULL;
	} else {
		if (!_decompress_radar(file, raw))
			return NULL;
	}

	/* Load the radar file */
	RSL_read_these_sweeps("all", NULL);
	Radar *radar = RSL_wsr88d_to_radar(raw, (gchar*)site);
	g_free(raw);
	if (!radar)
		return NULL;

	return aweather_level2_new(viewer, colormaps, radar);
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(AWeatherLevel2, aweather_level2, GIS_TYPE_CALLBACK);
static void aweather_level2_init(AWeatherLevel2 *self)
{
	GIS_CALLBACK(self)->callback  = _draw_radar;
	GIS_CALLBACK(self)->user_data = self;
}

static void aweather_level2_class_init(AWeatherLevel2Class *klass)
{
}

