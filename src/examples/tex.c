#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glu.h>

guint tex, texl, texr;

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer _)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return FALSE;
}

gboolean on_expose(GtkWidget *drawing, GdkEventExpose *event, gpointer _)
{
	glClearColor(0.5, 0.5, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1, -1,1, 10,-10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5);

	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glVertex3f(-0.25, -0.75, 0.0);
	glVertex3f(-0.25,  0.75, 0.0);
	glVertex3f( 0.25,  0.75, 0.0);
	glVertex3f( 0.25, -0.75, 0.0);
	glEnd();

	/* Textures */
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	gdouble y = 0.875;

	/* Left */
	glBlendFunc(GL_ONE, GL_ZERO);
	glBindTexture(GL_TEXTURE_2D, texl);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, y); glVertex3f(-0.75,  0.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-0.75,  0.5, 0.0);
	glTexCoord2f(2.0, 1.0); glVertex3f( 0.75,  0.5, 0.0);
	glTexCoord2f(2.0, y); glVertex3f( 0.75,  0.0, 0.0);
	glEnd();

	/* Right */
	glBlendFunc(GL_ONE, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, texr);
	glBegin(GL_QUADS);
	glTexCoord2f(-1.0, y); glVertex3f(-0.75, 0.0, 0.0);
	glTexCoord2f(-1.0, 1.0); glVertex3f(-0.75, 0.5, 0.0);
	glTexCoord2f( 1.0, 1.0); glVertex3f( 0.75, 0.5, 0.0);
	glTexCoord2f( 1.0, y); glVertex3f( 0.75, 0.0, 0.0);
	glEnd();

	/* Bottom */
	glBlendFunc(GL_ONE, GL_ZERO);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-0.75, -0.5, 0.0);
	glTexCoord2f(0.0, 1.0-y); glVertex3f(-0.75, -0.0, 0.0);
	glTexCoord2f(1.0, 1.0-y); glVertex3f( 0.75, -0.0, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f( 0.75, -0.5, 0.0);
	glEnd();


	/* Flush */
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();
	return FALSE;
}
gboolean on_configure(GtkWidget *drawing, GdkEventConfigure *event, gpointer _)
{
	glViewport(0, 0,
		drawing->allocation.width,
		drawing->allocation.height);
	return FALSE;
}

guint load_tex(gchar *filename)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);
	int        alpha  = gdk_pixbuf_get_has_alpha(pixbuf);
	guint      tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
			(alpha ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	g_object_unref(pixbuf);
	return tex;
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	GtkWidget   *window   = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget   *drawing  = gtk_drawing_area_new();
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode((GdkGLConfigMode)(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA));
	g_signal_connect(window,  "destroy",         G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window,  "key-press-event", G_CALLBACK(on_key_press),  NULL);
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(on_expose),     NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(on_configure),  NULL);
	gtk_widget_set_gl_capability(drawing, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);
	gtk_container_add(GTK_CONTAINER(window), drawing);
	gtk_widget_show_all(window);

	/* OpenGL setup */
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(drawing));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(drawing));
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);

	/* Load texture */
	texl = load_tex("tex_png/texls.png");
	texr = load_tex("tex_png/texrs.png");
	tex  = load_tex("tex_png/tex.png");

	gtk_main();

	gdk_gl_drawable_gl_end(gldrawable);
}
