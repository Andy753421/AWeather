#include <config.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <GL/gl.h>

#include <stdio.h>

#include "aweather-gui.h"

enum {
	LAYER_TOPO,
	LAYER_COUNTY,
	LAYER_RIVERS,
	LAYER_HIGHWAYS,
	LAYER_CITY,
	LAYER_COUNT
};

static struct {
	char *fmt;
	float z;
	guint tex;
} layers[] = {
	[LAYER_TOPO]     = { "Overlays/" "Topo/"     "Short/" "%s_Topo_Short.jpg",     1, 0 },
	[LAYER_COUNTY]   = { "Overlays/" "County/"   "Short/" "%s_County_Short.gif",   3, 0 },
	[LAYER_RIVERS]   = { "Overlays/" "Rivers/"   "Short/" "%s_Rivers_Short.gif",   4, 0 },
	[LAYER_HIGHWAYS] = { "Overlays/" "Highways/" "Short/" "%s_Highways_Short.gif", 5, 0 },
	[LAYER_CITY]     = { "Overlays/" "Cities/"   "Short/" "%s_City_Short.gif",     6, 0 },
};

static CURL *curl_handle;


/**
 * Cache a image from Ridge to the local disk
 * \param  path  Path to the Ridge file, starting after /ridge/
 * \return The local path to the cached image
 */
char *cache_image(char *path)
{
	gchar base[] = "http://radar.weather.gov/ridge/";
	gchar *url   = g_strconcat(base, path, NULL);
	gchar *local = g_build_filename(g_get_user_cache_dir(), PACKAGE, path, NULL);
	if (!g_file_test(local, G_FILE_TEST_EXISTS)) {
		if (!g_file_test(g_path_get_dirname(local), G_FILE_TEST_IS_DIR)) {
			//g_printerr("Making directory %s\n", g_path_get_dirname(local));
			g_mkdir_with_parents(g_path_get_dirname(local), 0755);
		}
		//g_printerr("Fetching image %s -> %s\n", url, local);
		long http_code;
		FILE *cached_image = fopen(local, "w+");
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, cached_image);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_perform(curl_handle);
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
		fflush(cached_image);
		if (http_code != 200) {
			g_message("http %ld while fetching %s", http_code, url);
			remove(local);
			return NULL;
		}
	}
	return local;
}

/**
 * Load an image into an OpenGL texture
 * \param  filename  Path to the image file
 * \return The OpenGL identifier for the texture
 */
guint load_texture(char *filename)
{
	/* Load image */
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);
	int        format = gdk_pixbuf_get_has_alpha(pixbuf) ? GL_RGBA : GL_RGB;

	/* Create Texture */
	guint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);   // 2d texture (x and y size)

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			format, GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	g_message("loaded image:  w=%-3d  h=%-3d  fmt=%x  px=(%02x,%02x,%02x,%02x)  img=%s",
		width, height, format, pixels[0], pixels[1], pixels[2], pixels[3],
		g_path_get_basename(filename));
	return id;
}

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	g_message("ridge:expose");
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

	for (int i = 0; i < LAYER_COUNT; i++) {
		glBindTexture(GL_TEXTURE_2D, layers[i].tex);

		glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 0.0); glVertex3f(500*1000*-1.0, 500*1000* 1.0, layers[i].z);
		glTexCoord2f(0.0, 1.0); glVertex3f(500*1000*-1.0, 500*1000*-1.0, layers[i].z);
		glTexCoord2f(1.0, 1.0); glVertex3f(500*1000* 1.0, 500*1000*-1.0, layers[i].z);
		glTexCoord2f(1.0, 0.0); glVertex3f(500*1000* 1.0, 500*1000* 1.0, layers[i].z);
		glEnd();
	}

	glPopMatrix();
	return FALSE;
}

static gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	for (int i = 0; i < LAYER_COUNT; i++) {
		if (layers[i].tex != 0)
			continue;
		char *path  = g_strdup_printf(layers[i].fmt, "IND");
		char *local = cache_image(path);
		layers[i].tex = load_texture(local);
		g_free(local);
		g_free(path);
	}
	return FALSE;
}

gboolean ridge_init(AWeatherGui *gui)
{
	GtkDrawingArea *drawing = aweather_gui_get_drawing(gui);

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(expose),    NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(configure), NULL);

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

	return TRUE;
}
