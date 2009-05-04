#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <math.h>

static guint topo_tex;

guint load_texture(char *filename)
{
	/* Load image */
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);

	/* Create Texture */
	guint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);   // 2d texture (x and y size)

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	g_message("loaded %s: w=%d h=%d", filename, width, height);
	return id;
}

static gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	//g_message("ridge:expose");
	glPushMatrix();
	glScaled(500*1000, 500*1000, 0);

	glBindTexture(GL_TEXTURE_2D, topo_tex);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,  1.0, 0.1);
	glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 0.1);
	glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, -1.0, 0.1);
	glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, 0.1);
	glEnd();

	glPopMatrix();
	return FALSE;
}

static gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	topo_tex = load_texture("../data/topo.jpg");
	return FALSE;
}

gboolean ridge_init(GtkDrawingArea *drawing, GtkNotebook *config)
{
	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(expose),    NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(configure), NULL);
	return TRUE;
}
