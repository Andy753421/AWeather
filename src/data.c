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
#include <stdio.h>
#include <glib.h>
#include <libsoup/soup.h>

#include "data.h"

typedef struct {
	AWeatherCacheDoneCallback callback;
	gpointer user_data;
	gchar *local;
	FILE  *fp;
} cache_file_end_t;

/*
 * Open a file, creating parent directories if needed
 */
static FILE *fopen_p(const gchar *path, const gchar *mode)
{
	gchar *parent = g_path_get_dirname(path);
	if (!g_file_test(parent, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(parent, 0755);
	g_free(parent);
	return fopen(path, mode);
}

static void cache_file_cb(SoupSession *session, SoupMessage *message, gpointer _info)
{
	cache_file_end_t *info = _info;
	gchar *uri = soup_uri_to_string(soup_message_get_uri(message), FALSE);
	g_debug("data: cache_file_cb");

	if (message->status_code == 416) {
		/* Range unsatisfiable, file already complete */
		info->callback(info->local, FALSE, info->user_data);
	} else if (SOUP_STATUS_IS_SUCCESSFUL(message->status_code)) {
		gint wrote = fwrite(message->response_body->data,  1,
				message->response_body->length, info->fp);
		g_debug("data: status=%u wrote=%d/%lld",
				message->status_code,
				wrote, message->response_body->length);
		fclose(info->fp);
		info->callback(info->local, TRUE, info->user_data);
	} else {
		g_warning("data: cache_file_cb - error copying file, status=%d\n"
				"\tsrc=%s\n"
				"\tdst=%s",
				message->status_code, uri, info->local);
	}
	g_free(uri);
	g_free(info->local);
	g_object_unref(session);
}

static void do_cache(gchar *uri, gchar *local, gboolean truncate, gchar *reason,
		AWeatherCacheDoneCallback callback, gpointer user_data)
{
	char *name = g_path_get_basename(uri);
	g_debug("data: do_cache - Caching file %s: %s", name, reason);
	g_free(name);

	cache_file_end_t *info = g_malloc0(sizeof(cache_file_end_t));
	info->callback  = callback;
	info->user_data = user_data;
	info->local     = local;

	/* TODO: move this to callback so we don't end up with 0 byte files
	 * Then change back to check for valid file after download */
	if (truncate) info->fp = fopen_p(local, "w");
	else          info->fp = fopen_p(local, "a");
	long bytes = ftell(info->fp);

	SoupSession *session = soup_session_async_new();
	SoupMessage *message = soup_message_new("GET", uri);
	if (message == NULL)
		g_error("message is null, cannot parse uri");
	if (bytes != 0)
		soup_message_headers_set_range(message->request_headers, bytes, -1);
	soup_session_queue_message(session, message, cache_file_cb, info);
}

/*
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
void cache_file(char *base, char *path, AWeatherCacheType update,
		AWeatherCacheDoneCallback callback, gpointer user_data)
{
	gchar *uri   = g_strconcat(base, path, NULL);
	gchar *local = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);

	if (update == AWEATHER_REFRESH)
		return do_cache(uri, local, TRUE, "cache forced",
				callback, user_data);

	if (update == AWEATHER_UPDATE)
		return do_cache(uri, local, FALSE, "attempting updating",
				callback, user_data);

	if (update == AWEATHER_ONCE && !g_file_test(local, G_FILE_TEST_EXISTS))
		return do_cache(uri, local, TRUE, "local does not exist",
				callback, user_data);

	/* No nead to cache, run the callback now and clean up */
	callback(local, FALSE, user_data);
	g_free(local);
	g_free(uri);
	return;
}
