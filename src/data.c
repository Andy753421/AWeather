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
	GFile *src;
	GFile *dst;
	gchar *user_data;
} cache_file_end_t;

static goffset g_file_get_size(GFile *file)
{
	GError *error = NULL;
	GFileInfo *info = g_file_query_info(file,
			G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, &error);
	if (error){
		g_warning("unable to get file size: %s", error->message);
		g_error_free(error);
	}
	goffset size = g_file_info_get_size(info);
	g_file_info_remove_attribute(info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
	g_object_unref(info);
	return size;
}

static void cache_file_cb(GObject *source_object, GAsyncResult *res, gpointer _info)
{
	cache_file_end_t *info = _info;
	char   *url   = g_file_get_path(info->src);
	char   *local = g_file_get_path(info->dst);
	GError *error = NULL;
	g_file_copy_finish(G_FILE(source_object), res, &error);
	if (error) {
		g_warning("error copying file ([%s]->[%s]): %s",
			url, local, error->message);
		g_error_free(error);
	} else {
		info->callback(local, TRUE, info->user_data);
	}
	g_object_unref(info->src);
	g_object_unref(info->dst);
	g_free(info);
	g_free(url);
	g_free(local);
}

static void do_cache(GFile *src, GFile *dst, char *reason,
		AWeatherCacheDoneCallback callback, gpointer user_data)
{
	char *name = g_file_get_basename(dst);
	g_debug("data: do_cache - Caching file %s: %s", name, reason);
	g_free(name);

	GFile *parent = g_file_get_parent(dst);
	if (!g_file_query_exists(parent, NULL))
		g_file_make_directory_with_parents(parent, NULL, NULL);
	g_object_unref(parent);

	cache_file_end_t *info = g_malloc0(sizeof(cache_file_end_t));
	info->callback  = callback;
	info->src       = src;
	info->dst       = dst;
	info->user_data = user_data;
	g_file_copy_async(src, dst,
		G_FILE_COPY_OVERWRITE,   // GFileCopyFlags flags,
		G_PRIORITY_DEFAULT_IDLE, // int io_priority,
		NULL,                    // GCancellable *cancellable,
		NULL,                    // GFileProgressCallback progress_callback,
		NULL,                    // gpointer progress_callback_data,
		cache_file_cb,           // GAsyncReadyCallback callback,
		info);                   // gpointer user_data
	return;
}

/*
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
void cache_file(char *base, char *path, AWeatherPolicyType update,
		AWeatherCacheDoneCallback callback, gpointer user_data)
{
	gchar *url   = g_strconcat(base, path, NULL);
	gchar *local = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);
	GFile *src   = g_file_new_for_uri(url);
	GFile *dst   = g_file_new_for_path(local);

	if (update == AWEATHER_ALWAYS)
		return do_cache(src, dst, "cache forced", callback, user_data);

	if (!g_file_test(local, G_FILE_TEST_EXISTS))
		return do_cache(src, dst, "local does not exist", callback, user_data);

	if (update == AWEATHER_AUTOMATIC && g_file_get_size(src) != g_file_get_size(dst))
		return do_cache(src, dst, "size mismatch", callback, user_data);

	/* No nead to cache, run the callback now and clean up */
	callback(local, FALSE, user_data);
	g_object_unref(src);
	g_object_unref(dst);
	g_free(local);
	g_free(url);
	return;
}
