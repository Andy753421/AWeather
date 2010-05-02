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

#define _XOPEN_SOURCE
#include <sys/time.h>
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

void _gtk_bin_set_child(GtkBin *bin, GtkWidget *new)
{
	GtkWidget *old = gtk_bin_get_child(bin);
	if (old)
		gtk_widget_destroy(old);
	gtk_container_add(GTK_CONTAINER(bin), new);
	gtk_widget_show_all(new);
}

static gchar *_find_nearest(time_t time, GList *files,
		gsize offset, gchar *format)
{
	g_debug("GisPluginRadar: _find_nearest ...");
	time_t  nearest_time = 0;
	char   *nearest_file = NULL;

	struct tm tm = {};
	for (GList *cur = files; cur; cur = cur->next) {
		gchar *file = cur->data;
		strptime(file+offset, format, &tm);
		if (ABS(time - mktime(&tm)) <
		    ABS(time - nearest_time)) {
			nearest_file = file;
			nearest_time = mktime(&tm);
		}
	}

	g_debug("GisPluginRadar: _find_nearest = %s", nearest_file);
	if (nearest_file)
		return g_strdup(nearest_file);
	else
		return NULL;
}


/**************
 * RadarSites *
 **************/
typedef enum {
	STATUS_UNLOADED,
	STATUS_LOADING,
	STATUS_LOADED,
} RadarSiteStatus;
struct _RadarSite {
	/* Information */
	city_t    *city;
	GisMarker *marker;     // Map marker for libgis
	gpointer  *marker_ref; // Reference to maker

	/* Stuff from the parents */
	GisViewer     *viewer;
	GisHttp       *http;
	GisPrefs      *prefs;
	GtkWidget     *pconfig;

	/* When loaded */
	RadarSiteStatus status;     // Loading status for the site
	GtkWidget      *config;
	AWeatherLevel2 *level2;     // The Level2 structure for the current volume
	gpointer        level2_ref; // GisViewer reference to the added radar

	/* Internal data */
	time_t   time;        // Current timestamp of the level2
	gchar   *message;     // Error message set while updating
	guint    time_id;     // "time-changed"     callback ID
	guint    refresh_id;  // "refresh"          callback ID
	guint    location_id; // "locaiton-changed" callback ID
};

/* format: http://mesonet.agron.iastate.edu/data/nexrd2/raw/KABR/KABR_20090510_0323 */
void _site_update_loading(gchar *file, goffset cur,
		goffset total, gpointer _site)
{
	RadarSite *site = _site;
	GtkWidget *progress_bar = gtk_bin_get_child(GTK_BIN(site->config));
	double percent = (double)cur/total;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), MIN(percent, 1.0));
	gchar *msg = g_strdup_printf("Loading... %5.1f%% (%.2f/%.2f MB)",
			percent*100, (double)cur/1000000, (double)total/1000000);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), msg);
	g_free(msg);
}
gboolean _site_update_end(gpointer _site)
{
	RadarSite *site = _site;
	if (site->message) {
		g_warning("GisPluginRadar: _update_end - %s", site->message);
		_gtk_bin_set_child(GTK_BIN(site->config), gtk_label_new(site->message));
	} else {
		_gtk_bin_set_child(GTK_BIN(site->config),
				aweather_level2_get_config(site->level2));
	}
	site->status = STATUS_LOADED;
	return FALSE;
}
gpointer _site_update_thread(gpointer _site)
{
	RadarSite *site = _site;
	g_debug("GisPluginRadar: _update - %s", site->city->code);
	site->status = STATUS_LOADING;
	site->message = NULL;

	gboolean offline = gis_viewer_get_offline(site->viewer);
	gchar *nexrad_url = gis_prefs_get_string(site->prefs,
			"aweather/nexrad_url", NULL);

	/* Remove old volume */
	g_debug("GisPluginRadar: _update - remove - %s", site->city->code);
	if (site->level2_ref) {
		gis_viewer_remove(site->viewer, site->level2_ref);
		site->level2_ref = NULL;
	}

	/* Find nearest volume (temporally) */
	g_debug("GisPluginRadar: _update - find nearest - %s", site->city->code);
	gchar *dir_list = g_strconcat(nexrad_url, "/", site->city->code,
			"/", "dir.list", NULL);
	GList *files = gis_http_available(site->http,
			"^K\\w{3}_\\d{8}_\\d{4}$", site->city->code,
			"\\d+ (.*)", (offline ? NULL : dir_list));
	g_free(dir_list);
	gchar *nearest = _find_nearest(site->time, files, 5, "%Y%m%d_%H%M");
	g_list_foreach(files, (GFunc)g_free, NULL);
	g_list_free(files);
	if (!nearest) {
		site->message = "No suitable files found";
		goto out;
	}

	/* Fetch new volume */
	g_debug("GisPluginRadar: _update - fetch");
	gchar *local = g_strconcat(site->city->code, "/", nearest, NULL);
	gchar *uri   = g_strconcat(nexrad_url, "/", local,   NULL);
	gchar *file = gis_http_fetch(site->http, uri, local,
			offline ? GIS_LOCAL : GIS_UPDATE,
			_site_update_loading, site);
	g_free(nexrad_url);
	g_free(nearest);
	g_free(local);
	g_free(uri);
	if (!file) {
		site->message = "Fetch failed";
		goto out;
	}

	/* Load and add new volume */
	g_debug("GisPluginRadar: _update - load - %s", site->city->code);
	site->level2 = aweather_level2_new_from_file(
			site->viewer, colormaps, file, site->city->code);
	g_free(file);
	if (!site->level2) {
		site->message = "Load failed";
		goto out;
	}
	site->level2_ref = gis_viewer_add(site->viewer,
			GIS_OBJECT(site->level2), GIS_LEVEL_WORLD, TRUE);

out:
	g_idle_add(_site_update_end, site);
	return NULL;
}
void _site_update(RadarSite *site)
{
	site->time = gis_viewer_get_time(site->viewer);
	g_debug("GisPluginRadar: _on_time_changed %s - %d",
			site->city->code, (gint)site->time);

	/* Add a progress bar */
	GtkWidget *progress = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "Loading...");
	_gtk_bin_set_child(GTK_BIN(site->config), progress);

	/* Fork loading right away so updating the
	 * list of times doesn't take too long */
	g_thread_create(_site_update_thread, site, FALSE, NULL);
}

/* RadarSite methods */
void radar_site_unload(RadarSite *site)
{
	if (site->status != STATUS_LOADED)
		return; // Abort if it's still loading

	g_debug("GisPluginRadar: radar_site_unload %s", site->city->code);

	if (site->time_id)
		g_signal_handler_disconnect(site->viewer, site->time_id);
	if (site->refresh_id)
		g_signal_handler_disconnect(site->viewer, site->refresh_id);

	/* Remove tab */
	if (site->config)
		gtk_widget_destroy(site->config);

	/* Remove radar */
	if (site->level2_ref) {
		gis_viewer_remove(site->viewer, site->level2_ref);
		site->level2_ref = NULL;
	}

	site->status = STATUS_UNLOADED;
}

void radar_site_load(RadarSite *site)
{
	g_debug("GisPluginRadar: radar_site_load %s", site->city->code);
	site->status = STATUS_LOADING;

	/* Add tab page */
	site->config = gtk_alignment_new(0, 0, 1, 1);
	GtkWidget *tab   = gtk_hbox_new(FALSE, 0);
	GtkWidget *close = gtk_button_new();
	GtkWidget *label = gtk_label_new(site->city->name);
	gtk_container_add(GTK_CONTAINER(close),
			gtk_image_new_from_stock(GTK_STOCK_CLOSE,
				GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(close), GTK_RELIEF_NONE);
	g_signal_connect_swapped(close, "clicked",
			G_CALLBACK(radar_site_unload), site);
	gtk_box_pack_start(GTK_BOX(tab), label, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(tab), close, FALSE, FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(site->pconfig),
			site->config, tab);
	gtk_widget_show_all(site->config);
	gtk_widget_show_all(tab);

	/* Set up radar loading */
	site->time_id = g_signal_connect_swapped(site->viewer, "time-changed",
			G_CALLBACK(_site_update), site);
	site->refresh_id = g_signal_connect_swapped(site->viewer, "refresh",
			G_CALLBACK(_site_update), site);
	_site_update(site);
}

void _site_on_location_changed(GisViewer *viewer,
		gdouble lat, gdouble lon, gdouble elev,
		gpointer _site)
{
	static gdouble min_dist = EARTH_R / 20;
	RadarSite *site = _site;

	/* Calculate distance, could cache xyz values */
	gdouble eye_xyz[3], site_xyz[3];
	lle2xyz(lat, lon, elev, &eye_xyz[0], &eye_xyz[1], &eye_xyz[2]);
	lle2xyz(site->city->pos.lat, site->city->pos.lon, site->city->pos.elev,
			&site_xyz[0], &site_xyz[1], &site_xyz[2]);
	gdouble dist = distd(site_xyz, eye_xyz);

	/* Load or unload the site if necessasairy */
	if (dist <= min_dist && dist < elev*1.25 && site->status == STATUS_UNLOADED)
		radar_site_load(site);
	else if (dist > 2*min_dist &&  site->status != STATUS_UNLOADED)
		radar_site_unload(site);
}

gboolean _site_add_marker(gpointer _site)
{
	RadarSite *site = _site;
	site->marker = gis_marker_new(site->city->name);
	GIS_OBJECT(site->marker)->center = site->city->pos;
	GIS_OBJECT(site->marker)->lod    = EARTH_R*site->city->lod;
	site->marker_ref = gis_viewer_add(site->viewer,
			GIS_OBJECT(site->marker), GIS_LEVEL_OVERLAY, FALSE);
	return FALSE;
}
RadarSite *radar_site_new(city_t *city, GtkWidget *pconfig,
		GisViewer *viewer, GisPrefs *prefs, GisHttp *http)
{
	RadarSite *site = g_new0(RadarSite, 1);
	site->viewer  = g_object_ref(viewer);
	site->prefs   = g_object_ref(prefs);
	site->http    = http;
	site->city    = city;
	site->pconfig = pconfig;

	/* Add marker */
	g_idle_add_full(G_PRIORITY_LOW, _site_add_marker, site, NULL);


	/* Connect signals */
	site->location_id  = g_signal_connect(viewer, "location-changed",
			G_CALLBACK(_site_on_location_changed), site);
	return site;
}

void radar_site_free(RadarSite *site)
{
	radar_site_unload(site);
	gis_viewer_remove(site->viewer, site->marker_ref);
	if (site->location_id)
		g_signal_handler_disconnect(site->viewer, site->location_id);
	g_object_unref(site->viewer);
	g_object_unref(site->prefs);
	g_free(site);
}


/**************
 * RadarConus *
 **************/
struct _RadarConus {
	GisViewer   *viewer;
	GisHttp     *http;
	GtkWidget   *config;
	time_t       time;
	GisTile     *tile;
	gpointer    *tile_ref;
	GdkPixbuf   *pixbuf;
	const gchar *message;
	gchar       *nearest;

	guint        time_id;     // "time-changed"     callback ID
	guint        refresh_id;  // "refresh"          callback ID
};

void _conus_update_loading(gchar *file, goffset cur,
		goffset total, gpointer _conus)
{
	RadarConus *conus = _conus;
	GtkWidget *progress_bar = gtk_bin_get_child(GTK_BIN(conus->config));
	double percent = (double)cur/total;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), MIN(percent, 1.0));
	gchar *msg = g_strdup_printf("Loading... %5.1f%% (%.2f/%.2f MB)",
			percent*100, (double)cur/1000000, (double)total/1000000);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), msg);
	g_free(msg);
}

gboolean _conus_update_end(gpointer _conus)
{
	RadarConus *conus = _conus;

	guchar    *pixels = gdk_pixbuf_get_pixels(conus->pixbuf);
	gboolean   alpha  = gdk_pixbuf_get_has_alpha(conus->pixbuf);
	gint       width  = gdk_pixbuf_get_width(conus->pixbuf);
	gint       height = gdk_pixbuf_get_height(conus->pixbuf);

	if (!conus->tile->data) {
		conus->tile->data = g_new0(guint, 1);
		glGenTextures(1, conus->tile->data);
	}

	guint *tex = conus->tile->data;
	glBindTexture(GL_TEXTURE_2D, *tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
			(alpha ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glFlush();

	/* finish */
	_gtk_bin_set_child(GTK_BIN(conus->config),
			gtk_label_new(conus->nearest));
	gtk_widget_queue_draw(GTK_WIDGET(conus->viewer));

	/* free data */
	g_object_unref(conus->pixbuf);
	g_free(conus->nearest);

	return FALSE;
}

gpointer _conus_update_thread(gpointer _conus)
{
	RadarConus *conus = _conus;

	/* Find nearest */
	gboolean offline = gis_viewer_get_offline(conus->viewer);
	gchar *conus_url = "http://radar.weather.gov/Conus/RadarImg/";
	GList *files = gis_http_available(conus->http,
			"^Conus_[^\"]*_N0Ronly.gif$", "",
			NULL, (offline ? NULL : conus_url));
	conus->nearest = _find_nearest(conus->time, files, 6, "%Y%m%d_%H%M");
	g_list_foreach(files, (GFunc)g_free, NULL);
	g_list_free(files);
	if (!conus->nearest) {
		conus->message = "No suitable files";
		goto out;
	}

	/* Fetch the image */
	gchar *uri     = g_strconcat(conus_url, conus->nearest, NULL);
	gchar *path    = gis_http_fetch(conus->http, uri, conus->nearest, GIS_ONCE,
			_conus_update_loading, conus);
	g_free(uri);
	if (!path) {
		conus->message = "Fetch failed";
		goto out;
	}

	/* Load the image to a pixbuf */
	GError *error = NULL;
	conus->pixbuf = gdk_pixbuf_new_from_file(path, &error);
	g_free(path);
	if (!gdk_pixbuf_get_has_alpha(conus->pixbuf)) {
		GdkPixbuf *tmp = gdk_pixbuf_add_alpha(conus->pixbuf, TRUE, 0xff, 0xff, 0xff);
		g_object_unref(conus->pixbuf);
		conus->pixbuf = tmp;
	}

	/* Map the pixbuf's alpha values */
	const guchar colormap[][2][4] = {
		{{0x04, 0xe9, 0xe7}, {0x04, 0xe9, 0xe7, 0x30}},
		{{0x01, 0x9f, 0xf4}, {0x01, 0x9f, 0xf4, 0x60}},
		{{0x03, 0x00, 0xf4}, {0x03, 0x00, 0xf4, 0x90}},
	};
	guchar *pixels = gdk_pixbuf_get_pixels(conus->pixbuf);
	gint    height = gdk_pixbuf_get_height(conus->pixbuf);
	gint    width  = gdk_pixbuf_get_width(conus->pixbuf);
	for (int i = 0; i < width*height; i++) {
		for (int j = 0; j < G_N_ELEMENTS(colormap); j++) {
			if (pixels[i*4+0] > 0xe0 &&
			    pixels[i*4+1] > 0xe0 &&
			    pixels[i*4+2] > 0xe0) {
				pixels[i*4+3] = 0x00;
				break;
			}
			if (pixels[i*4+0] == colormap[j][0][0] &&
			    pixels[i*4+1] == colormap[j][0][1] &&
			    pixels[i*4+2] == colormap[j][0][2]) {
				pixels[i*4+0] = colormap[j][1][0];
				pixels[i*4+1] = colormap[j][1][1];
				pixels[i*4+2] = colormap[j][1][2];
				pixels[i*4+3] = colormap[j][1][3];
				break;
			}
		}
	}

out:
	g_idle_add(_conus_update_end, conus);
	return NULL;
}

void _conus_update(RadarConus *conus)
{
	conus->time = gis_viewer_get_time(conus->viewer);
	g_debug("GisPluginRadar: _conus_update - %d",
			(gint)conus->time);

	/* Add a progress bar */
	GtkWidget *progress = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "Loading...");
	_gtk_bin_set_child(GTK_BIN(conus->config), progress);

	g_thread_create(_conus_update_thread, conus, FALSE, NULL);
}

RadarConus *radar_conus_new(GtkWidget *pconfig,
		GisViewer *viewer, GisHttp *http)
{
	RadarConus *conus = g_new0(RadarConus, 1);
	conus->viewer  = g_object_ref(viewer);
	conus->http    = http;
	conus->config  = gtk_alignment_new(0, 0, 1, 1);
	conus->tile    = gis_tile_new(NULL,
			50.406626367301044,   50.406626367301044-0.017971305190311*1600,
			-127.620375523875420+0.017971305190311*3400, -127.620375523875420);
	conus->tile->zindex = 1;
	conus->tile_ref = gis_viewer_add(viewer,
			GIS_OBJECT(conus->tile), GIS_LEVEL_WORLD, TRUE);

	conus->time_id = g_signal_connect_swapped(viewer, "time-changed",
			G_CALLBACK(_conus_update), conus);
	conus->refresh_id = g_signal_connect_swapped(viewer, "refresh",
			G_CALLBACK(_conus_update), conus);

	gtk_notebook_append_page(GTK_NOTEBOOK(pconfig), conus->config, gtk_label_new("Conus"));
	_conus_update(conus);
	return conus;
}

void radar_conus_free(RadarConus *conus)
{
	g_signal_handler_disconnect(conus->viewer, conus->time_id);
	g_signal_handler_disconnect(conus->viewer, conus->refresh_id);

	if (conus->tile->data) {
		glDeleteTextures(1, conus->tile->data);
		g_free(conus->tile->data);
	}
	gis_viewer_remove(conus->viewer, conus->tile_ref);

	g_object_unref(conus->viewer);
	g_free(conus);
}


/******************
 * GisPluginRadar *
 ******************/
static void _draw_hud(GisCallback *callback, gpointer _self)
{
	/* TODO */
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	if (!self->colormap)
		return;

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
}

/* Methods */
GisPluginRadar *gis_plugin_radar_new(GisViewer *viewer, GisPrefs *prefs)
{
	/* TODO: move to constructor if possible */
	g_debug("GisPluginRadar: new");
	GisPluginRadar *self = g_object_new(GIS_TYPE_PLUGIN_RADAR, NULL);
	self->viewer = viewer;
	self->prefs  = prefs;

	/* Load HUD */
	GisCallback *hud_cb = gis_callback_new(_draw_hud, self);
	self->hud_ref = gis_viewer_add(viewer, GIS_OBJECT(hud_cb), GIS_LEVEL_HUD, FALSE);

	/* Load Conus */
	self->conus = radar_conus_new(self->config, self->viewer, self->conus_http);

	/* Load radar sites */
	for (city_t *city = cities; city->type; city++) {
		if (city->type != LOCATION_CITY)
			continue;
		RadarSite *site = radar_site_new(city, self->config,
				self->viewer, self->prefs, self->sites_http);
		g_hash_table_insert(self->sites, city->code, site);
	}

	return self;
}

static GtkWidget *gis_plugin_radar_get_config(GisPlugin *_self)
{
	GisPluginRadar *self = GIS_PLUGIN_RADAR(_self);
	return self->config;
}

/* GObject code */
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
static void gis_plugin_radar_init(GisPluginRadar *self)
{
	g_debug("GisPluginRadar: class_init");
	/* Set defaults */
	self->sites_http = gis_http_new(G_DIR_SEPARATOR_S "nexrad" G_DIR_SEPARATOR_S "level2" G_DIR_SEPARATOR_S);
	self->conus_http = gis_http_new(G_DIR_SEPARATOR_S "nexrad" G_DIR_SEPARATOR_S "conus" G_DIR_SEPARATOR_S);
	self->sites      = g_hash_table_new_full(g_str_hash, g_str_equal,
				NULL, (GDestroyNotify)radar_site_free);
	self->config     = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(self->config), GTK_POS_TOP);
}
static void gis_plugin_radar_dispose(GObject *gobject)
{
	g_debug("GisPluginRadar: dispose");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(gobject);
	gis_viewer_remove(self->viewer, self->hud_ref);
	radar_conus_free(self->conus);
	/* Drop references */
	G_OBJECT_CLASS(gis_plugin_radar_parent_class)->dispose(gobject);
}
static void gis_plugin_radar_finalize(GObject *gobject)
{
	g_debug("GisPluginRadar: finalize");
	GisPluginRadar *self = GIS_PLUGIN_RADAR(gobject);
	/* Free data */
	gis_http_free(self->conus_http);
	gis_http_free(self->sites_http);
	g_hash_table_destroy(self->sites);
	gtk_widget_destroy(self->config);
	G_OBJECT_CLASS(gis_plugin_radar_parent_class)->finalize(gobject);

}
static void gis_plugin_radar_class_init(GisPluginRadarClass *klass)
{
	g_debug("GisPluginRadar: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = gis_plugin_radar_dispose;
	gobject_class->finalize = gis_plugin_radar_finalize;
}
