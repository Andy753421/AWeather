#include <config.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <GL/gl.h>

#include <stdio.h>

#include "aweather-gui.h"
#include "data.h"

enum {
	LAYER_TOPO,
	LAYER_COUNTY,
	LAYER_RIVERS,
	LAYER_HIGHWAYS,
	LAYER_CITY,
	LAYER_COUNT
};

typedef struct {
	gchar *fmt;
	float z;
	guint tex;
} layer_t;

static layer_t layers[] = {
	[LAYER_TOPO]     = { "Overlays/" "Topo/"     "Short/" "%s_Topo_Short.jpg",     1, 0 },
	[LAYER_COUNTY]   = { "Overlays/" "County/"   "Short/" "%s_County_Short.gif",   3, 0 },
	[LAYER_RIVERS]   = { "Overlays/" "Rivers/"   "Short/" "%s_Rivers_Short.gif",   4, 0 },
	[LAYER_HIGHWAYS] = { "Overlays/" "Highways/" "Short/" "%s_Highways_Short.gif", 5, 0 },
	[LAYER_CITY]     = { "Overlays/" "Cities/"   "Short/" "%s_City_Short.gif",     6, 0 },
};

static AWeatherGui *gui = NULL;

/**
 * Load an image into an OpenGL texture
 * \param  filename  Path to the image file
 * \return The OpenGL identifier for the texture
 */
void load_texture(const gchar *filename, gpointer _layer)
{
	layer_t *layer = _layer;
	aweather_gui_gl_begin(gui);

	/* Load image */
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	if (!pixbuf)
		g_warning("Failed to load texture: %s", filename);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);
	int        format = gdk_pixbuf_get_has_alpha(pixbuf) ? GL_RGBA : GL_RGB;

	/* Create Texture */
	glGenTextures(1, &layer->tex);
	glBindTexture(GL_TEXTURE_2D, layer->tex); // 2d texture (x and y size)

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			format, GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	g_message("loaded image:  w=%-3d  h=%-3d  fmt=%x  px=(%02x,%02x,%02x,%02x)  img=%s",
		width, height, format, pixels[0], pixels[1], pixels[2], pixels[3],
		g_path_get_basename(filename));

	aweather_gui_gl_end(gui);

	/* Redraw */
	gtk_widget_queue_draw(GTK_WIDGET(aweather_gui_get_drawing(gui)));
}

static void set_site(AWeatherView *view, gchar *site, gpointer user_data)
{
	g_message("location changed to %s", site);
	for (int i = 0; i < LAYER_COUNT; i++) {
		gchar *base = "http://radar.weather.gov/ridge/";
		gchar *path  = g_strdup_printf(layers[i].fmt, site);
		cache_file(base, path, load_texture, &layers[i]);
		g_free(path);
	}
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

gboolean ridge_init(AWeatherGui *_gui)
{
	gui = _gui;
	GtkDrawingArea *drawing = aweather_gui_get_drawing(gui);
	AWeatherView   *view    = aweather_gui_get_view(gui);

	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",     G_CALLBACK(expose),    NULL);
	g_signal_connect(view,    "location-changed", G_CALLBACK(set_site),  NULL);

	return TRUE;
}
