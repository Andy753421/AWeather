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

#ifndef __DATA_H__
#define __DATA_H__

#include <libsoup/soup.h>

typedef enum {
	GIS_ONCE,    // Cache the file if it does not exist
	GIS_UPDATE,  // Append additional data to cached copy (resume)
	GIS_REFRESH, // Delete existing file and cache a new copy
} GisDataCacheType;

typedef void (*GisDataCacheDoneCallback)(gchar *file, gboolean updated,
		gpointer user_data);

typedef void (*GisDataCacheChunkCallback)(gchar *file, goffset cur,
		goffset total, gpointer user_data);

SoupSession *cache_file(char *base, char *path, GisDataCacheType update,
		GisDataCacheChunkCallback user_chunk_cb,
		GisDataCacheDoneCallback user_done_cb,
		gpointer user_data);

#endif
