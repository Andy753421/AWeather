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
	AWEATHER_ONCE,    // Cache the file if it does not exist
	AWEATHER_UPDATE,  // Append additional data to cached copy (resume)
	AWEATHER_REFRESH, // Delete existing file and cache a new copy
} AWeatherCacheType;

typedef void (*AWeatherCacheDoneCallback)(gchar *file, gboolean updated,
		gpointer user_data);

typedef void (*AWeatherCacheChunkCallback)(gchar *file, goffset cur,
		goffset total, gpointer user_data);

SoupSession *cache_file(char *base, char *path, AWeatherCacheType update,
		AWeatherCacheChunkCallback user_chunk_cb,
		AWeatherCacheDoneCallback user_done_cb,
		gpointer user_data);

#endif
