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

typedef enum {
	AWEATHER_ALWAYS,
	AWEATHER_AUTOMATIC,
	AWEATHER_NEVER,
} AWeatherPolicyType;

typedef void (*AWeatherCacheDoneCallback)(gchar *file, gboolean updated,
		gpointer user_data);

void cache_file(char *base, char *path, AWeatherPolicyType update,
		AWeatherCacheDoneCallback callback, gpointer user_data);

#endif
