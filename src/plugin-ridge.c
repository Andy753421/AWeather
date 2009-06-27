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
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <GL/gl.h>

#include <stdio.h>

#include "aweather-gui.h"
#include "plugin-ridge.h"
#include "data.h"

/****************
 * GObject code *
 ****************/
/* Plugin init */
static void aweather_ridge_plugin_init(AWeatherPluginInterface *iface);
static void aweather_ridge_expose(AWeatherPlugin *_ridge);
G_DEFINE_TYPE_WITH_CODE(AWeatherRidge, aweather_ridge, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(AWEATHER_TYPE_PLUGIN,
			aweather_ridge_plugin_init));
static void aweather_ridge_plugin_init(AWeatherPluginInterface *iface)
{
	g_debug("AWeatherRidge: plugin_init");
	/* Add methods to the interface */
	iface->expose = aweather_ridge_expose;
}
/* Class/Object init */
static void aweather_ridge_init(AWeatherRidge *ridge)
{
	g_debug("AWeatherRidge: init");
	/* Set defaults */
	ridge->gui = NULL;
}
static void aweather_ridge_dispose(GObject *gobject)
{
	g_debug("AWeatherRidge: dispose");
	AWeatherRidge *self = AWEATHER_RIDGE(gobject);
	/* Drop references */
	G_OBJECT_CLASS(aweather_ridge_parent_class)->dispose(gobject);
}
static void aweather_ridge_finalize(GObject *gobject)
{
	g_debug("AWeatherRidge: finalize");
	AWeatherRidge *self = AWEATHER_RIDGE(gobject);
	/* Free data */
	G_OBJECT_CLASS(aweather_ridge_parent_class)->finalize(gobject);

}
static void aweather_ridge_class_init(AWeatherRidgeClass *klass)
{
	g_debug("AWeatherRidge: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = aweather_ridge_dispose;
	gobject_class->finalize = aweather_ridge_finalize;
}

/*********************
 * Overlay constants *
 *********************/
enum {
	LAYER_TOPO,
	LAYER_COUNTY,
	LAYER_RIVERS,
	LAYER_HIGHWAYS,
	LAYER_CITY,
	LAYER_COUNT
};

typedef struct {
	gchar *name;
	gchar *fmt;
	gboolean enabled;
	float z;
	guint tex;
} layer_t;

static layer_t layers[] = {
	[LAYER_TOPO]     = {"Topo",     "Overlays/Topo/Short/%s_Topo_Short.jpg",         TRUE,  1, 0},
	[LAYER_COUNTY]   = {"Counties", "Overlays/County/Short/%s_County_Short.gif",     TRUE,  3, 0},
	[LAYER_RIVERS]   = {"Rivers",   "Overlays/Rivers/Short/%s_Rivers_Short.gif",     FALSE, 4, 0},
	[LAYER_HIGHWAYS] = {"Highways", "Overlays/Highways/Short/%s_Highways_Short.gif", FALSE, 5, 0},
	[LAYER_CITY]     = {"Cities",   "Overlays/Cities/Short/%s_City_Short.gif",       TRUE,  6, 0},
};


/***********
 * Helpers *
 ***********/
/*
 * Load an image into an OpenGL texture
 * \param  filename  Path to the image file
 * \return The OpenGL identifier for the texture
 */
void load_texture(AWeatherRidge *self, layer_t *layer, gchar *filename)
{
	aweather_gui_gl_begin(self->gui);

	/* Load image */
	GError *error = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	if (!pixbuf || error) {
		g_warning("Failed to load texture: %s", filename);
		return;
	}
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);
	int        format = gdk_pixbuf_get_has_alpha(pixbuf) ? GL_RGBA : GL_RGB;

	/* Create Texture */
	glDeleteTextures(1, &layer->tex);
	glGenTextures(1, &layer->tex);
	glBindTexture(GL_TEXTURE_2D, layer->tex); // 2d texture (x and y size)

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			format, GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	char *base = g_path_get_basename(filename);
	g_debug("AWeatherRidge: load_texture - "
		"w=%-3d  h=%-3d  fmt=%x  px=(%02x,%02x,%02x,%02x)  img=%s",
		width, height, format, pixels[0], pixels[1], pixels[2], pixels[3],
		base);
	g_free(base);

	aweather_gui_gl_end(self->gui);

	g_object_unref(pixbuf);

	/* Redraw */
	aweather_gui_gl_redraw(self->gui);
}


/*****************
 * ASync helpers *
 *****************/
typedef struct {
	AWeatherRidge *self;
	layer_t *layer;
} cached_t;
void cached_cb(gchar *filename, gboolean updated, gpointer _udata)
{
	cached_t *udata = _udata;
	load_texture(udata->self, udata->layer, filename);
	g_free(udata);
}

/*************
 * callbacks *
 *************/
static void on_site_changed(AWeatherView *view, gchar *site, AWeatherRidge *self)
{
	g_debug("AWeatherRidge: on_site_changed - site=%s", site);
	for (int i = 0; i < LAYER_COUNT; i++) {
		gchar *base = "http://radar.weather.gov/ridge/";
		gchar *path  = g_strdup_printf(layers[i].fmt, site+1);
		cached_t *udata = g_malloc(sizeof(cached_t));
		udata->self  = self;
		udata->layer = &layers[i];
		cache_file(base, path, AWEATHER_ONCE, cached_cb, udata);
		//cache_file(base, path, AWEATHER_UPDATE, cached_cb, udata);
		g_free(path);
	}
}

void toggle_layer(GtkToggleButton *check, AWeatherGui *gui)
{
	layer_t *layer = g_object_get_data(G_OBJECT(check), "layer");
	layer->enabled = gtk_toggle_button_get_active(check);
	aweather_gui_gl_redraw(gui);
}

/***********
 * Methods *
 ***********/
AWeatherRidge *aweather_ridge_new(AWeatherGui *gui)
{
	AWeatherRidge *ridge = g_object_new(AWEATHER_TYPE_RIDGE, NULL);
	ridge->gui = gui;

	AWeatherView *view    = aweather_gui_get_view(gui);
	GtkWidget    *drawing = aweather_gui_get_widget(gui, "drawing");
	GtkWidget    *config  = aweather_gui_get_widget(gui, "tabs");

	/* Add configuration tab */
	GtkWidget *tab  = gtk_label_new("Ridge");
	GtkWidget *body = gtk_alignment_new(0.5, 0, 0, 0);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
	for (int i = 0; i < LAYER_COUNT; i++) {
		GtkWidget *check = gtk_check_button_new_with_label(layers[i].name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), layers[i].enabled);
		gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, TRUE, 0);
		g_object_set_data(G_OBJECT(check), "layer", &layers[i]);
		g_signal_connect(check, "toggled", G_CALLBACK(toggle_layer), gui);
	}
	gtk_container_add(GTK_CONTAINER(body), hbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(config), body, tab);

	g_signal_connect(view, "site-changed", G_CALLBACK(on_site_changed),  ridge);

	return ridge;
}

static void aweather_ridge_expose(AWeatherPlugin *_ridge)
{
	AWeatherRidge *ridge = AWEATHER_RIDGE(_ridge);

	g_debug("AWeatherRidge: expose");
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

	for (int i = 0; i < LAYER_COUNT; i++) {
		if (!layers[i].enabled)
			continue;
		glBindTexture(GL_TEXTURE_2D, layers[i].tex);
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 0.0); glVertex3f(240*1000*-1.0, 282*1000* 1.0, layers[i].z);
		glTexCoord2f(0.0, 1.0); glVertex3f(240*1000*-1.0, 282*1000*-1.0, layers[i].z);
		glTexCoord2f(1.0, 1.0); glVertex3f(240*1000* 1.0, 282*1000*-1.0, layers[i].z);
		glTexCoord2f(1.0, 0.0); glVertex3f(240*1000* 1.0, 282*1000* 1.0, layers[i].z);
		glEnd();
	}

	glPopMatrix();
}
