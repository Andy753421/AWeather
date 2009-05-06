#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

static gboolean expose_start(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	g_message("opengl:expose_start");
	GdkGLContext *glcontext = gtk_widget_get_gl_context(da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext)) {
		g_assert_not_reached();
	}

	/* draw in here */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return FALSE;
}
/* Plugins run stuff here */
static gboolean expose_end(GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	g_message("opengl:expose_end");

	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();

	gdk_gl_drawable_gl_end(gldrawable);

	return FALSE;
}
static gboolean configure_start(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	glViewport(0, 0, da->allocation.width, da->allocation.height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glRotatef(0, 0, 2, 45);
	gluPerspective(45.0f, 1, 0.1f, 10000000.0f);
	//glFrustum(-1, 1, -1, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	double scale = 500*1000; // 500 km
	//glOrtho(-scale,scale,-scale,scale,-10000,10000);
	glTranslatef(0.0, 0.0, -2.5*scale);
	glRotatef(-45, 1, 0, 0);

	return FALSE;
}
static gboolean configure_end(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(da);
	gdk_gl_drawable_gl_end(gldrawable);
	return FALSE;
}

gboolean opengl_init(GtkDrawingArea *drawing, GtkNotebook *config)
{
	/* Set up OpenGL Stuff */
	g_signal_connect      (drawing, "configure-event", G_CALLBACK(configure_start), NULL);
	g_signal_connect_after(drawing, "configure-event", G_CALLBACK(configure_end),   NULL);
	g_signal_connect      (drawing, "expose-event",    G_CALLBACK(expose_start),    NULL);
	g_signal_connect_after(drawing, "expose-event",    G_CALLBACK(expose_end),      NULL);
	return TRUE;
}
