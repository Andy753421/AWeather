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

/**
 * http://www.nasa.network.com/elev?
 * SERVICE=WMS&
 * VERSION=1.1.0&
 * REQUEST=GetMap&
 * LAYERS=bmng200406&
 * STYLES=&
 * SRS=EPSG:4326&
 * BBOX=-180,-90,180,90&
 * FORMAT=image/jpeg&
 * WIDTH=600&
 * HEIGHT=300
 * 
 * http://www.nasa.network.com/elev?
 * SERVICE=WMS&
 * VERSION=1.1.0&
 * REQUEST=GetMap&
 * LAYERS=srtm30&
 * STYLES=&
 * SRS=EPSG:4326&
 * BBOX=-180,-90,180,90&
 * FORMAT=application/bil32&
 * WIDTH=600&
 * HEIGHT=300
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <libsoup/soup.h>

#include "wms.h"

/* TODO: try to remove these */
#include "gis-world.h"
#include <GL/gl.h>

gchar *wms_make_uri(WmsInfo *info, gdouble xmin, gdouble ymin, gdouble xmax, gdouble ymax)
{
	return g_strdup_printf(
		"%s?"
		"SERVICE=WMS&"
		"VERSION=1.1.0&"
		"REQUEST=GetMap&"
		"LAYERS=%s&"
		"STYLES=&"
		"SRS=EPSG:4326&"
		"BBOX=%f,%f,%f,%f&"
		"FORMAT=%s&"
		"WIDTH=%d&"
		"HEIGHT=%d",
		info->uri_prefix,
		info->uri_layer,
		xmin, ymin, xmax, ymax,
		info->uri_format,
		info->width,
		info->height);
}

/****************
 * WmsCacheNode *
 ****************/
WmsCacheNode *wms_cache_node_new(WmsCacheNode *parent,
		gdouble xmin, gdouble ymin, gdouble xmax, gdouble ymax, gint width)
{
	WmsCacheNode *self = g_new0(WmsCacheNode, 1);
	//g_debug("WmsCacheNode: new - %p %+7.3f,%+7.3f,%+7.3f,%+7.3f",
	//		self, xmin, ymin, xmax, ymax);
	self->latlon[0] = xmin;
	self->latlon[1] = ymin;
	self->latlon[2] = xmax;
	self->latlon[3] = ymax;
	self->parent    = parent;
	if (ymin <= 0 && ymax >= 0)
		self->res = ll2m(xmax-xmin, 0)/width;
	else
		self->res = ll2m(xmax-xmin, MIN(ABS(ymin),ABS(ymax)))/width;
	return self;
}

void wms_cache_node_free(WmsCacheNode *node, WmsFreeer freeer)
{
	//g_debug("WmsCacheNode: free - %p", node);
	if (node->data) {
		freeer(node);
		node->data = NULL;
	}
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			if (node->children[x][y])
				wms_cache_node_free(node->children[x][y], freeer);
	g_free(node);
}

/***********
 * WmsInfo *
 ***********/
WmsInfo *wms_info_new(WmsLoader loader, WmsFreeer freeer,
	gchar *uri_prefix, gchar *uri_layer, gchar *uri_format,
	gchar *cache_prefix, gchar *cache_ext,
	gint resolution, gint width, gint height
) {
	WmsInfo *self = g_new0(WmsInfo, 1);
	self->loader       = loader;
	self->freeer       = freeer;
	self->uri_prefix   = uri_prefix;
	self->uri_layer    = uri_layer;
	self->uri_format   = uri_format;
	self->cache_prefix = cache_prefix;
	self->cache_ext    = cache_ext;
	self->resolution   = resolution;
	self->width        = width;
	self->height       = height;

	self->max_age      = 60;
	self->atime        = time(NULL);
	self->gc_source    = g_timeout_add_seconds(1, (GSourceFunc)wms_info_gc, self);
	self->cache_root   = wms_cache_node_new(NULL, -180, -90, 180, 90, width);
	self->soup         = soup_session_async_new();
	return self;
}

struct _CacheImageState {
	WmsInfo *info;
	gchar *path;
	FILE *output;
	WmsCacheNode *node;
	WmsChunkCallback user_chunk_cb;
	WmsDoneCallback user_done_cb;
	gpointer user_data;
};
void wms_info_soup_chunk_cb(SoupMessage *message, SoupBuffer *chunk, gpointer _state)
{
	struct _CacheImageState *state = _state;
	if (!SOUP_STATUS_IS_SUCCESSFUL(message->status_code))
		return;

	goffset total = soup_message_headers_get_content_length(message->response_headers);
	if (fwrite(chunk->data, chunk->length, 1, state->output) != 1)
		g_warning("WmsInfo: soup_chunk_cb - eror writing data");

	gdouble cur = (gdouble)ftell(state->output);
	if (state->user_chunk_cb)
		state->user_chunk_cb(cur, total, state->user_data);
}
void wms_info_soup_done_cb(SoupSession *session, SoupMessage *message, gpointer _state)
{
	struct _CacheImageState *state = _state;
	if (!SOUP_STATUS_IS_SUCCESSFUL(message->status_code))
		return;
	gchar *dest = g_strndup(state->path, strlen(state->path)-5);
	g_rename(state->path, dest);
	state->node->atime = time(NULL);
	state->info->loader(state->node, dest, state->info->width, state->info->height);
	if (state->user_done_cb)
		state->user_done_cb(state->node, state->user_data);
	state->node->caching = FALSE;
	fclose(state->output);
	g_free(state->path);
	g_free(dest);
	g_free(state);
}
gboolean wms_info_cache_loader_cb(gpointer _state)
{
	struct _CacheImageState *state = _state;
	state->node->atime = time(NULL);
	state->info->loader(state->node, state->path, state->info->width, state->info->height);
	if (state->user_done_cb)
		state->user_done_cb(state->node, state->user_data);
	state->node->caching = FALSE;
	g_free(state->path);
	g_free(state);
	return FALSE;
}
/**
 * Cache required tiles
 * 1. Load closest tile that's stored on disk
 * 2. Fetch the correct tile from the remote server
 */
void wms_info_cache(WmsInfo *info, gdouble resolution, gdouble lat, gdouble lon,
		WmsChunkCallback chunk_callback, WmsDoneCallback done_callback,
		gpointer user_data)
{
	/* Base cache path */
	gdouble x=lon, y=lat;
	gdouble xmin=-180, ymin=-90, xmax=180, ymax=90;
	gdouble xdist = xmax - xmin;
	gdouble ydist = ymax - ymin;
	int xpos=0, ypos=0;
	gdouble cur_lat = 0;

	WmsCacheNode *target_node = info->cache_root;
	WmsCacheNode *approx_node = NULL;

	GString *target_path = g_string_new(g_get_user_cache_dir());
	g_string_append(target_path, G_DIR_SEPARATOR_S);
	g_string_append(target_path, "wms");
	g_string_append(target_path, G_DIR_SEPARATOR_S);
	g_string_append(target_path, info->cache_prefix);
	gchar *approx_path = NULL;

	/* Create nodes to tiles, determine paths and lat-lon coords */
	while (TRUE) {
		/* Update the best approximation if it exists on disk */
		gchar *tmp = g_strconcat(target_path->str, info->cache_ext, NULL);
		if (g_file_test(tmp, G_FILE_TEST_EXISTS)) {
			g_free(approx_path);
			approx_node = target_node;
			approx_path = tmp;
		} else {
			g_free(tmp);
		}

		/* Break if current resolution (m/px) is good enough */
		if (ll2m(xdist, cur_lat)/info->width <= resolution  ||
		    ll2m(xdist, cur_lat)/info->width <= info->resolution)
		    	break;

		/* Get locations for the correct sub-tile */
		xpos = (int)(((x - xmin) / xdist) * 4);
		ypos = (int)(((y - ymin) / ydist) * 4);
		if (xpos == 4) xpos--;
		if (ypos == 4) ypos--;
		xdist /= 4;
		ydist /= 4;
		xmin = xmin + xdist*(xpos+0);
		ymin = ymin + ydist*(ypos+0);
		xmax = xmin + xdist;
		ymax = ymin + ydist; 
		cur_lat = MIN(ABS(ymin), ABS(ymax));

		/* Update target for correct sub-tile */
		g_string_append_printf(target_path, "/%d%d", xpos, ypos);
		if (target_node->children[xpos][ypos] == NULL)
			target_node->children[xpos][ypos] =
				wms_cache_node_new(target_node,
					xmin, ymin, xmax, ymax, info->width);
		target_node = target_node->children[xpos][ypos];
	}

	/* Load disk on-disk approximation, TODO: async */
	if (approx_node && !approx_node->data && !approx_node->caching) {
		approx_node->caching = TRUE;
		struct _CacheImageState *state = g_new0(struct _CacheImageState, 1);
		state->info          = info;
		state->path          = approx_path;
		state->node          = approx_node;
		state->user_done_cb  = done_callback;
		state->user_data     = user_data;
		g_idle_add(wms_info_cache_loader_cb, state);
	} else { 
		g_free(approx_path);
	}

	/* If target image is not on-disk, download it now */
	if (target_node != approx_node && !target_node->caching) {
		target_node->caching = TRUE;
		g_string_append(target_path, info->cache_ext);
		g_string_append(target_path, ".part");

		gchar *dirname = g_path_get_dirname(target_path->str);
		g_mkdir_with_parents(dirname, 0755);
		g_free(dirname);

		struct _CacheImageState *state = g_new0(struct _CacheImageState, 1);
		state->info          = info;
		state->path          = target_path->str;
		state->output        = fopen(target_path->str, "a");
		state->node          = target_node;
		state->user_chunk_cb = chunk_callback;
		state->user_done_cb  = done_callback;
		state->user_data     = user_data;

		gchar *uri = wms_make_uri(info, xmin, ymin, xmax, ymax);
		SoupMessage *message = soup_message_new("GET", uri);
		g_signal_connect(message, "got-chunk", G_CALLBACK(wms_info_soup_chunk_cb), state);
		soup_message_headers_set_range(message->request_headers, ftell(state->output), -1);

		soup_session_queue_message(info->soup, message, wms_info_soup_done_cb, state);

		g_debug("Caching file: %s -> %s", uri, state->path);
		g_free(uri);
		g_string_free(target_path, FALSE);
	} else {
		g_string_free(target_path, TRUE);
	}
}
/* TODO:
 *   - Store WmsCacheNode in point and then use parent pointers to go up/down
 *   - If resolution doesn't change, tell caller to skip remaining calculations
 */
WmsCacheNode *wms_info_fetch(WmsInfo *info, WmsCacheNode *root,
		gdouble resolution, gdouble lat, gdouble lon,
		gboolean *correct)
{
	if (root && root->data && !root->caching &&
	    root->latlon[0] <= lon && lon <= root->latlon[2] &&
	    root->latlon[1] <= lat && lat <= root->latlon[3] &&
	    root->res <= resolution &&
	    (!root->parent || root->parent->res > resolution)) {
	    	*correct = TRUE;
		info->atime = time(NULL);
		root->atime = info->atime;
		return root;
	}

	if (info->cache_root == NULL) {
		*correct = FALSE;
		return NULL;
	}
	WmsCacheNode *node = info->cache_root;
	WmsCacheNode *best = (node && node->data ? node : NULL);
	gdouble xmin=-180, ymin=-90, xmax=180, ymax=90, xdist=360, ydist=180;
	gdouble cur_lat = 0;
	int xpos=0, ypos=0;
	gdouble cur_res = ll2m(xdist, cur_lat)/info->width;
	while (cur_res > resolution  &&
	       cur_res > info->resolution) {

		xpos = ((lon - xmin) / xdist) * 4;
		ypos = ((lat - ymin) / ydist) * 4;
		if (xpos == 4) xpos--;
		if (ypos == 4) ypos--;
		xdist /= 4;
		ydist /= 4;
		xmin = xmin + xdist*(xpos+0);
		ymin = ymin + ydist*(ypos+0);
		cur_lat = MIN(ABS(ymin), ABS(ymax));

		node = node->children[xpos][ypos];
		if (node == NULL)
			break;
		if (node->data)
			best = node;

		cur_res = ll2m(xdist, cur_lat)/info->width;
	}
	if (correct)
		*correct = (node && node == best);
	info->atime = time(NULL);
	if (best)
		best->atime = info->atime;
	return best;
}

WmsCacheNode *wms_info_fetch_cache(WmsInfo *info, WmsCacheNode *root,
		gdouble res, gdouble lat, gdouble lon,
		WmsChunkCallback chunk_callback, WmsDoneCallback done_callback, gpointer user_data)
{
	/* Fetch a node, if it isn't cached, cache it, also keep it's parent cached */
	gboolean correct;
	WmsCacheNode *node = wms_info_fetch(info, root, res, lat, lon, &correct);
	if (!node || !correct)
		wms_info_cache(info, res, lat, lon, chunk_callback, done_callback, user_data);
	//else if (node->parent && node->parent->data == NULL)
	//	wms_info_cache(info, node->parent->res, lat, lon, chunk_callback, done_callback, user_data);
	//else if  (node->parent)
	//	node->parent->atime = node->atime;
	return node;
}

/* Delete unused nodes and prune empty branches */
static WmsCacheNode *wms_info_gc_cb(WmsInfo *self, WmsCacheNode *node)
{
	gboolean empty = FALSE;
	if (self->atime - node->atime > self->max_age &&
	    node->data && node != self->cache_root && !node->caching) {
		g_debug("WmsInfo: gc - expired node %p", node);
		self->freeer(node);
		node->data = NULL;
		empty = TRUE;
	}
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			if (node->children[x][y]) {
				node->children[x][y] =
					wms_info_gc_cb(self, node->children[x][y]);
				empty = FALSE;
			}
	if (empty) {
		g_debug("WmsInfo: gc - empty branch %p", node);
		/* 
		 * TODO: Don't prune nodes while we're caching WmsCacheNodes in the Roam triangles
		 * and points
		g_free(node);
		return NULL;
		*/
		return node;
	} else {
		return node;
	}
}

gboolean wms_info_gc(WmsInfo *self)
{
	if (!wms_info_gc_cb(self, self->cache_root))
		g_warning("WmsInfo: gc - root not should not be empty");
	return TRUE;
}

void wms_info_free(WmsInfo *self)
{
	wms_cache_node_free(self->cache_root, self->freeer);
	g_object_unref(self->soup);
	g_free(self);
}


/************************
 * Blue Marble Next Gen *
 ************************/
void bmng_opengl_loader(WmsCacheNode *node, const gchar *path, gint width, gint height)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	node->data = g_new0(guint, 1);

	/* Load image */
	guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int     alpha  = gdk_pixbuf_get_has_alpha(pixbuf);
	int     nchan  = 4; // gdk_pixbuf_get_n_channels(pixbuf);

	/* Create Texture */
	glGenTextures(1, node->data);
	glBindTexture(GL_TEXTURE_2D, *(guint*)node->data);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, nchan, width, height, 0,
			(alpha ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	g_object_unref(pixbuf);
	g_debug("WmsCacheNode: bmng_opengl_loader: %s -> %p", path, node->data);
}
void bmng_opengl_freeer(WmsCacheNode *node)
{
	g_debug("WmsCacheNode: bmng_opengl_freeer: %p", node->data);
	glDeleteTextures(1, node->data);
	g_free(node->data);
}

void bmng_pixbuf_loader(WmsCacheNode *node, const gchar *path, gint width, gint height)
{
	node->data = gdk_pixbuf_new_from_file(path, NULL);
	g_debug("WmsCacheNode: bmng_opengl_loader: %s -> %p", path, node->data);
}
void bmng_pixbuf_freeer(WmsCacheNode *node)
{
	g_debug("WmsCacheNode: bmng_opengl_freeer: %p", node->data);
	g_object_unref(node->data);
}

WmsInfo *wms_info_new_for_bmng(WmsLoader loader, WmsFreeer freeer)
{
	loader = loader ?: bmng_opengl_loader;
	freeer = freeer ?: bmng_opengl_freeer;
	return wms_info_new(loader, freeer,
		"http://www.nasa.network.com/wms", "bmng200406", "image/jpeg",
		"bmng", ".jpg", 500, 512, 256);
}

/********************************************
 * Shuttle Radar Topography Mission 30 Plus *
 ********************************************/
void srtm_bil_loader(WmsCacheNode *node, const gchar *path, gint width, gint height)
{
	WmsBil *bil = g_new0(WmsBil, 1);
	gchar **char_data = (gchar**)&bil->data;
	g_file_get_contents(path, char_data, NULL, NULL);
	bil->width  = width;
	bil->height = height;
	node->data = bil;
	g_debug("WmsCacheNode: srtm_opengl_loader: %s -> %p", path, node->data);
}
void srtm_bil_freeer(WmsCacheNode *node)
{
	g_debug("WmsCacheNode: srtm_opengl_freeer: %p", node);
	g_free(((WmsBil*)node->data)->data);
	g_free(node->data);
}

void srtm_pixbuf_loader(WmsCacheNode *node, const gchar *path, gint width, gint height)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	gint       stride = gdk_pixbuf_get_rowstride(pixbuf);

	gint16 *data; 
	gchar **char_data = (gchar**)&data; 
	g_file_get_contents(path, char_data, NULL, NULL);
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++) {
			gint16 value = data[r*width + c];
			//guchar color = (float)(MAX(value,0))/8848 * 255;
			guchar color = (float)value/8848 * 255;
			pixels[r*stride + c*3 + 0] = color;
			pixels[r*stride + c*3 + 1] = color;
			pixels[r*stride + c*3 + 2] = color;
		}
	}
	g_free(data);

	node->data = pixbuf;
	g_debug("WmsCacheNode: srtm_opengl_loader: %s -> %p", path, node->data);
}
void srtm_pixbuf_freeer(WmsCacheNode *node)
{
	g_debug("WmsCacheNode: srtm_opengl_freeer: %p", node);
	g_object_unref(node->data);
}

WmsInfo *wms_info_new_for_srtm(WmsLoader loader, WmsFreeer freeer)
{
	loader = loader ?: srtm_bil_loader;
	freeer = freeer ?: srtm_bil_freeer;
	return wms_info_new(loader, freeer,
		"http://www.nasa.network.com/elev", "srtm30", "application/bil",
		"srtm", ".bil", 500, 512, 256);
}
