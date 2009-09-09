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

#ifndef __WMS_H__
#define __WMS_H__

#include <math.h>
#include <time.h>
#include <libsoup/soup.h>

typedef struct _WmsCacheNode WmsCacheNode;
typedef struct _WmsInfo WmsInfo;

typedef void (*WmsChunkCallback)(gsize cur, gsize total, gpointer user_data);
typedef void (*WmsDoneCallback)(WmsCacheNode *node, gpointer user_data);
typedef void (*WmsLoader)(WmsCacheNode *node, const gchar *path, gint width, gint height);
typedef void (*WmsFreeer)(WmsCacheNode *node);

/****************
 * WmsCacheNode *
 ****************/
struct _WmsCacheNode {
	gpointer data;
	gdouble  latlon[4]; // xmin,ymin,xmax,ymax
	gdouble  res;       // xmin,ymin,xmax,ymax
	gboolean caching;
	time_t   atime;
	WmsCacheNode *parent;
	WmsCacheNode *children[4][4];
};

WmsCacheNode *wms_cache_node_new(WmsCacheNode *parent, gdouble xmin, gdouble ymin, gdouble xmax, gdouble ymax, gint width);

void wms_cache_node_free(WmsCacheNode *node, WmsFreeer freeer);

/***********
 * WmsInfo *
 ***********/
struct _WmsInfo {
	gchar *uri_prefix;
	gchar *uri_layer;
	gchar *uri_format;
	gchar *cache_prefix;
	gchar *cache_ext;
	gint   resolution;  // m/px
	gint   width;
	gint   height;

	guint  max_age;
	guint  gc_source;
	time_t atime;
	WmsLoader     loader;
	WmsFreeer     freeer;
	WmsCacheNode *cache_root;
	SoupSession  *soup;
};

WmsInfo *wms_info_new(WmsLoader loader, WmsFreeer freeer,
		gchar *uri_prefix, gchar *uri_layer, gchar *uri_format,
		gchar *cache_prefix, gchar *cache_ext,
		gint   resolution, gint   width, gint   height);

void wms_info_cache(WmsInfo *info, gdouble resolution, gdouble lat, gdouble lon,
		WmsChunkCallback chunk_callback, WmsDoneCallback done_callback,
		gpointer user_data);

WmsCacheNode *wms_info_fetch(WmsInfo *info, WmsCacheNode *root,
		gdouble resolution, gdouble lat, gdouble lon,
		gboolean *correct);

WmsCacheNode *wms_info_fetch_cache(WmsInfo *info, WmsCacheNode *root,
		gdouble resolution, gdouble lat, gdouble lon,
		WmsChunkCallback chunk_callback, WmsDoneCallback done_callback,
		gpointer user_data);

gboolean wms_info_gc(WmsInfo *self);

void wms_info_free(WmsInfo *info);


/********************************
 * Specific things (bmng, srtm) *
 ********************************/
typedef struct _WmsBil WmsBil;
struct _WmsBil {
	gint16 *data;
	gint width;
	gint height;
};

void bmng_opengl_loader(WmsCacheNode *node, const gchar *path, gint width, gint height);
void bmng_opengl_freeer(WmsCacheNode *node);

void bmng_pixbuf_loader(WmsCacheNode *node, const gchar *path, gint width, gint height);
void bmng_pixbuf_freeer(WmsCacheNode *node);

WmsInfo *wms_info_new_for_bmng(WmsLoader loader, WmsFreeer freeer);

void srtm_bil_loader(WmsCacheNode *node, const gchar *path, gint width, gint height);
void srtm_bil_freeer(WmsCacheNode *node);

void srtm_pixbuf_loader(WmsCacheNode *node, const gchar *path, gint width, gint height);
void srtm_pixbuf_freeer(WmsCacheNode *node);

WmsInfo *wms_info_new_for_srtm(WmsLoader loader, WmsFreeer freeer);

#endif
