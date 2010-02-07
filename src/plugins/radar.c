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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gio/gio.h>
#include <GL/gl.h>
#include <math.h>
#include <rsl.h>

#include <gis.h>

#include "radar.h"
#include "level2.h"
#include "../aweather-location.h"

/* Drawing functions */
static gpointer _draw_hud(GisCallback *callback, gpointer _self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	if (!self->colormap)
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
		glColor4ubv(self->colormap->data[i]);
		glVertex3f(-1.0, (float)((i  ) - 256/2)/(256/2), 0.0); // bot left
		glVertex3f(-1.0, (float)((i+1) - 256/2)/(256/2), 0.0); // top left
		glVertex3f(-0.9, (float)((i+1) - 256/2)/(256/2), 0.0); // top right
		glVertex3f(-0.9, (float)((i  ) - 256/2)/(256/2), 0.0); // bot right
	}
	glEnd();

	return NULL;
}


/***************
 * GUI loading *
 ***************/
/* Clear the config area */ 
static void _load_gui_clear(GisPluginRadar *self)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(self->config_body));
	if (child)
		gtk_widget_destroy(child);
}

/* Setup a loading screen in the tab */
static void _load_gui_pre(GisPluginRadar *self)
{
	g_debug("GisPluginRadar: _load_gui_pre");

	/* Set up progress bar */
	_load_gui_clear(self);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	self->progress_bar   = gtk_progress_bar_new();
	self->progress_label = gtk_label_new("Loading radar...");
	gtk_box_pack_start(GTK_BOX(vbox), self->progress_bar,   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), self->progress_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self->config_body), vbox);
	gtk_widget_show_all(self->config_body);
}

/* Update pogress bar of loading screen */
static void _load_gui_update(char *path, goffset cur, goffset total, gpointer _self)
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

/* Update pogress bar of loading screen */
static void _load_gui_error(GisPluginRadar *self, gchar *error)
{
	gchar *msg = g_strdup_printf(
			"GisPluginRadar: error loading radar - %s", error);
	g_warning("%s", msg);
	_load_gui_clear(self);
	gtk_container_add(GTK_CONTAINER(self->config_body), gtk_label_new(msg));
	gtk_widget_show_all(self->config_body);
	g_free(msg);
}

/* Clear loading screen and add sweep selectors */
static void _on_sweep_clicked(GtkRadioButton *button, gpointer _self);
static void _load_gui_success(GisPluginRadar *self, AWeatherLevel2 *level2)
{
	Radar *radar = level2->radar;
	g_debug("GisPluginRadar: _load_gui_success - %p", radar);
	/* Clear existing items */
	_load_gui_clear(self);

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

			g_object_set_data(G_OBJECT(button), "level2", (gpointer)level2);
			g_object_set_data(G_OBJECT(button), "type",   (gpointer)vi);
			g_object_set_data(G_OBJECT(button), "elev",   (gpointer)(int)(elev*100));
			g_signal_connect(button, "clicked", G_CALLBACK(_on_sweep_clicked), self);
		}
	}
	gtk_container_add(GTK_CONTAINER(self->config_body), table);
	gtk_widget_show_all(table);
}

static gchar *_download_radar(GisPluginRadar *self, const gchar *site, const gchar *time)
{
	g_debug("GisPluginRadar: _download_radar - %s, %s", site, time);

	/* format: http://mesonet.agron.iastate.edu/data/nexrd2/raw/KABR/KABR_20090510_0323 */
	gchar *base  = gis_prefs_get_string(self->prefs, "aweather/nexrad_url", NULL);
	gchar *local = g_strdup_printf("%s/%s_%s", site, site, time);
	gchar *uri   = g_strconcat(base, "/", local, NULL);
	GisCacheType mode = gis_viewer_get_offline(self->viewer) ? GIS_LOCAL : GIS_UPDATE;
	gchar *file  =  gis_http_fetch(self->http, uri, local, mode, _load_gui_update, self);
	g_free(base);
	g_free(local);
	g_free(uri);
	return file;
}

struct SetRadarData {
	GisPluginRadar *self;
	gchar *site;
	gchar *time;
};
static gpointer _set_radar_cb(gpointer _data)
{
	g_debug("GisPluginRadar: _set_radar_cb");
	struct SetRadarData *data = _data;
	GisPluginRadar *self = data->self;

	gdk_threads_enter();
	_load_gui_pre(self);
	if (self->radar) {
		gis_viewer_remove(self->viewer, self->radar);
		self->radar = NULL;
	}
	gchar *file = _download_radar(self, data->site, data->time);
	if (!file) {
		_load_gui_error(self, "Download failed");
		goto out;
	}
	AWeatherLevel2 *level2 = aweather_level2_new_from_file(
			self->viewer, colormaps, file, data->site);
	if (!level2) {
		_load_gui_error(self, "Loading radar failed");
		//g_remove(file);
		goto out;
	}
	self->colormap = level2->sweep_colors;
	self->radar = gis_viewer_add(self->viewer, GIS_OBJECT(level2), GIS_LEVEL_WORLD, TRUE);
	_load_gui_success(self, level2);

out:
	gdk_threads_leave();
	g_free(file);
	g_free(data->site);
	g_free(data->time);
	g_free(data);
	return NULL;
}
static void _set_radar(GisPluginRadar *self, const gchar *site, const gchar *time)
{
	if (!site || !time)
		return;
	//soup_session_abort(self->http->soup);
	//g_mutex_lock(self->load_mutex);
	struct SetRadarData *data = g_new(struct SetRadarData, 1);
	data->self = self;
	data->site = g_strdup(site);
	data->time = g_strdup(time);
	g_thread_create(_set_radar_cb, data, FALSE, NULL);
}

/*************
 * Callbacks *
 *************/
static void _on_sweep_clicked(GtkRadioButton *button, gpointer _self)
{
	GisPluginRadar *self   = GIS_PLUGIN_RADAR(_self);
	AWeatherLevel2 *level2 = g_object_get_data(G_OBJECT(button), "level2");
	gint type = (gint)g_object_get_data(G_OBJECT(button), "type");
	gint elev = (gint)g_object_get_data(G_OBJECT(button), "elev");
	aweather_level2_set_sweep(level2, type, (float)elev/100);
	self->colormap = level2->sweep_colors;
}

static void _on_time_changed(GisViewer *viewer, const char *time, gpointer _self)
{
	g_debug("GisPluginRadar: _on_time_changed");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
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
	if (min_city && min_city != last_city){
		self->cur_site = min_city->code;
		_set_radar(self, self->cur_site, self->cur_time);
	}
	last_city = min_city;
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

	GisCallback *hud_cb = gis_callback_new(_draw_hud, self);
	gis_viewer_add(viewer, GIS_OBJECT(hud_cb), GIS_LEVEL_HUD, FALSE);

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
	//self->load_mutex = g_mutex_new();
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
	//g_mutex_free(self->load_mutex);
	G_OBJECT_CLASS(gis_plugin_radar_parent_class)->finalize(gobject);

}
static void gis_plugin_radar_class_init(GisPluginRadarClass *klass)
{
	g_debug("GisPluginRadar: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = gis_plugin_radar_dispose;
	gobject_class->finalize = gis_plugin_radar_finalize;
}

