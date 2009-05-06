#include <config.h>
#include <math.h>
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
	g_message("opengl:expose_end\n");

	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();

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


	double width  = da->allocation.width;
	double height = da->allocation.height;
	double dist   = 500*1000; // 500 km

	/* Misc */
	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.8f, 0.8f, 1.0f, 0.0f);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	/* Depth test */
	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/* Perspective */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double rad = atan(height/2*1000.0/dist); // 1px = 1000 meters
	double deg = (rad*180)/M_PI;
	gluPerspective(deg*2, width/height, 0.1f, 1000000000.0f);

	/* Camera position? */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -dist);
	//glRotatef(-45, 1, 0, 0);


	return FALSE;
}
static gboolean configure_end(GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
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
