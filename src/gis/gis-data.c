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

#include "gis-data.h"

typedef struct {
	gchar *uri;
	gchar *local;
	FILE  *fp;
	GisDataCacheDoneCallback  user_done_cb;
	GisDataCacheChunkCallback user_chunk_cb;
	gpointer user_data;
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

static void done_cb(SoupSession *session, SoupMessage *message, gpointer _info)
{
	cache_file_end_t *info = _info;
	g_debug("data: done_cb");

	if (message->status_code == 416)
		/* Range unsatisfiable, file already complete */
		info->user_done_cb(info->local, FALSE, info->user_data);
	else if (SOUP_STATUS_IS_SUCCESSFUL(message->status_code))
		info->user_done_cb(info->local, TRUE, info->user_data);
	else
		g_warning("data: done_cb - error copying file, status=%d\n"
				"\tsrc=%s\n"
				"\tdst=%s",
				message->status_code, info->uri, info->local);
	g_free(info->uri);
	g_free(info->local);
	fclose(info->fp);
	g_free(info);
	//g_object_unref(session); This is probably leaking
}

void chunk_cb(SoupMessage *message, SoupBuffer *chunk, gpointer _info)
{
	cache_file_end_t *info = _info;
	if (!SOUP_STATUS_IS_SUCCESSFUL(message->status_code))
		return;

	if (!fwrite(chunk->data, chunk->length, 1, info->fp))
		g_error("data: chunk_cb - Unable to write data");
	goffset cur   = ftell(info->fp);
	//goffset total = soup_message_headers_get_range(message->response_headers);
	goffset start=0, end=0, total=0;
	soup_message_headers_get_content_range(message->response_headers,
			&start, &end, &total);

	if (info->user_chunk_cb)
		info->user_chunk_cb(info->local, cur, total, info->user_data);
}

static SoupSession *do_cache(cache_file_end_t *info, gboolean truncate, gchar *reason)
{
	char *name = g_path_get_basename(info->uri);
	g_debug("data: do_cache - Caching file %s: %s", name, reason);
	g_free(name);

	/* TODO: move this to callback so we don't end up with 0 byte files
	 * Then change back to check for valid file after download */
	if (truncate) info->fp = fopen_p(info->local, "w");
	else          info->fp = fopen_p(info->local, "a");
	long bytes = ftell(info->fp);

	SoupSession *session = soup_session_async_new();
	g_object_set(session, "user-agent", PACKAGE_STRING, NULL);
	SoupMessage *message = soup_message_new("GET", info->uri);
	if (message == NULL)
		g_error("message is null, cannot parse uri");
	g_signal_connect(message, "got-chunk", G_CALLBACK(chunk_cb), info);
	soup_message_headers_set_range(message->request_headers, bytes, -1);
	soup_session_queue_message(session, message, done_cb, info);
	return session;
}

/*
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
SoupSession *cache_file(char *base, char *path, GisDataCacheType update,
		GisDataCacheChunkCallback user_chunk_cb,
		GisDataCacheDoneCallback user_done_cb,
		gpointer user_data)
{
	g_debug("GisData: cache_file - base=%s, path=%s", base, path);
	if (base == NULL) {
		g_warning("GisData: cache_file - base is null");
		return NULL;
	}
	if (path == NULL) {
		g_warning("GisData: cache_file - base is null");
		return NULL;
	}
	cache_file_end_t *info = g_malloc0(sizeof(cache_file_end_t));
	info->uri           = g_strconcat(base, path, NULL);
	info->local         = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);
	info->fp            = NULL;
	info->user_chunk_cb = user_chunk_cb;
	info->user_done_cb  = user_done_cb;
	info->user_data     = user_data;

	if (update == GIS_REFRESH)
		return do_cache(info, TRUE, "cache forced");

	if (update == GIS_UPDATE)
		return do_cache(info, FALSE, "attempting updating");

	if (update == GIS_ONCE && !g_file_test(info->local, G_FILE_TEST_EXISTS))
		return do_cache(info, TRUE, "local does not exist");

	/* No nead to cache, run the callback now and clean up */
	user_done_cb(info->local, FALSE, user_data);
	g_free(info->uri);
	g_free(info->local);
	g_free(info);
	return NULL;
}
