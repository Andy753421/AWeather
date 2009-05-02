#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>

static gboolean expose_start(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext)) {
		g_assert_not_reached();
	}

	/* draw in here */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return TRUE;
}
/* Plugins run stuff here */
static gboolean expose_end(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();

	gdk_gl_drawable_gl_end(gldrawable);

	return FALSE;
}
static gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	glLoadIdentity();
	glViewport(0, 0, da->allocation.width, da->allocation.height);
	glOrtho(-10,10,-10,10,-20050,10000);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glScalef(10., 10., 10.);
	
	gdk_gl_drawable_gl_end(gldrawable);

	return FALSE;
}

gboolean opengl_init(GtkDrawingArea *drawing, GtkNotebook *config)
{
	/* Set up OpenGL Stuff */
	g_signal_connect(drawing, "configure-event", G_CALLBACK(configure), NULL);
	
	//g_signal_connect(drawing, "expose-event", G_CALLBACK(expose), NULL);
	g_signal_connect      (drawing, "expose-event", G_CALLBACK(expose_start), NULL);
	g_signal_connect_after(drawing, "expose-event", G_CALLBACK(expose_end),   NULL);
	return TRUE;
}
