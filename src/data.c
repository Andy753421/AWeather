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

/**
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
void cache_file(char *base, char *path, AWeatherCacheDoneCallback callback, gpointer user_data)
{
	gchar *url   = g_strconcat(base, path, NULL);
	gchar *local = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);
	if (g_file_test(local, G_FILE_TEST_EXISTS)) {
		callback(local, user_data);
	} else {
		if (!g_file_test(g_path_get_dirname(local), G_FILE_TEST_IS_DIR))
			g_mkdir_with_parents(g_path_get_dirname(local), 0755);
		cache_file_end_t *info = g_malloc0(sizeof(cache_file_end_t));
		info->callback  = callback;
		info->src       = url;
		info->dst       = local;
		info->user_data = user_data;
		GFile *src = g_file_new_for_uri(url);
		GFile *dst = g_file_new_for_path(local);
		g_file_copy_async(src, dst,
			G_FILE_COPY_OVERWRITE, // GFileCopyFlags flags,
			0,                     // int io_priority,
			NULL,                  // GCancellable *cancellable,
			NULL,                  // GFileProgressCallback progress_callback,
			NULL,                  // gpointer progress_callback_data,
			cache_file_cb,         // GAsyncReadyCallback callback,
			info);                 // gpointer user_data
	}
}
