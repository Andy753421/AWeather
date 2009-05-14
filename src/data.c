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
#include <glib.h>
#include <gio/gio.h>

#include "data.h"

typedef struct {
	AWeatherCacheDoneCallback callback;
	gchar *src;
	gchar *dst;
	gchar *user_data;
} cache_file_end_t;

static void cache_file_cb(GObject *source_object, GAsyncResult *res, gpointer _info)
{
	cache_file_end_t *info = _info;
	GError *error = NULL;
	g_file_copy_finish(G_FILE(source_object), res, &error);
	if (error) {
		g_message("error copying file ([%s]->[%s]): %s",
			info->src, info->dst, error->message);
	} else {
		info->callback(info->dst, info->user_data);
	}
	g_free(info->src);
	g_free(info->dst);
	g_free(info);
}

static goffset g_file_get_size(GFile *file)
{
	GError *error = NULL;
	GFileInfo *info = g_file_query_info(file,
			G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, &error);
	if (error)
		g_warning("unable to get file size: %s", error->message);
	return g_file_info_get_size(info);
}

/**
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
void cache_file(char *base, char *path, AWeatherCacheDoneCallback callback, gpointer user_data)
{
	gchar *url   = g_strconcat(base, path, NULL);
	gchar *local = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);
	GFile *src = g_file_new_for_uri(url);
	GFile *dst = g_file_new_for_path(local);

	if (!g_file_test(local, G_FILE_TEST_EXISTS))
		g_message("Caching file: local does not exist - %s", local);
	else if (g_file_get_size(src) != g_file_get_size(dst))
		g_message("Caching file: sizes mismatch - %lld != %lld",
				g_file_get_size(src), g_file_get_size(dst));
	else {
		callback(local, user_data);
		g_free(local);
		g_free(url);
		return;
	}

	char *dir = g_path_get_dirname(local);
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(dir, 0755);
	g_free(dir);
	cache_file_end_t *info = g_malloc0(sizeof(cache_file_end_t));
	info->callback  = callback;
	info->src       = url;
	info->dst       = local;
	info->user_data = user_data;
	g_file_copy_async(src, dst,
		G_FILE_COPY_OVERWRITE, // GFileCopyFlags flags,
		0,                     // int io_priority,
		NULL,                  // GCancellable *cancellable,
		NULL,                  // GFileProgressCallback progress_callback,
		NULL,                  // gpointer progress_callback_data,
		cache_file_cb,         // GAsyncReadyCallback callback,
		info);                 // gpointer user_data
}
