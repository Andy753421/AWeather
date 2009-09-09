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

/* Tessellation, "finding intersecting triangles" */
/* http://research.microsoft.com/pubs/70307/tr-2006-81.pdf */
/* http://www.opengl.org/wiki/Alpha_Blending */

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "gis-opengl.h"
#include "roam.h"
#include "wms.h"

#define FOV_DIST   2000.0
#define MPPX(dist) (4*dist/FOV_DIST)

// #define ROAM_DEBUG

/*************
 * ROAM Code *
 *************/
void roam_queue_draw(WmsCacheNode *node, gpointer _self)
{
	gtk_widget_queue_draw(GTK_WIDGET(_self));
}

void roam_height_func(RoamPoint *point, gpointer _self)
{
	GisOpenGL *self = _self;

	gdouble lat, lon, elev;
	xyz2lle(point->x, point->y, point->z, &lat, &lon, &elev);

#ifdef ROAM_DEBUG
	lle2xyz(lat, lon, 0, &point->x, &point->y, &point->z);
	return;
#endif

	gdouble cam_lle[3], cam_xyz[3];
	gis_view_get_location(self->view, &cam_lle[0], &cam_lle[1], &cam_lle[2]);
	lle2xyz(cam_lle[0], cam_lle[1], cam_lle[2], &cam_xyz[0], &cam_xyz[1], &cam_xyz[2]);

	gdouble res = MPPX(distd(cam_xyz, (double*)point));
	//g_message("lat=%f, lon=%f, res=%f", lat, lon, res);

	point->node = wms_info_fetch_cache(self->srtm, point->node,
			res, lat, lon, NULL, roam_queue_draw, self);

	if (point->node) {
		WmsBil *bil = point->node->data;

		gint w = bil->width;
		gint h = bil->height;

		gdouble xmin  = point->node->latlon[0];
		gdouble ymin  = point->node->latlon[1];
		gdouble xmax  = point->node->latlon[2];
		gdouble ymax  = point->node->latlon[3];

		gdouble xdist = xmax - xmin;
		gdouble ydist = ymax - ymin;

		gdouble x =    (lon-xmin)/xdist  * w;
		gdouble y = (1-(lat-ymin)/ydist) * h;

		gdouble x_rem = x - (int)x;
		gdouble y_rem = y - (int)y;
		guint x_flr = (int)x;
		guint y_flr = (int)y;

		/* TODO: Fix interpolation at edges:
		 *   - Pad these at the edges instead of wrapping/truncating
		 *   - Figure out which pixels to index (is 0,0 edge, center, etc) */
		gint16 px00 = bil->data[MIN((y_flr  ),h-1)*w + MIN((x_flr  ),w-1)];
                gint16 px10 = bil->data[MIN((y_flr  ),h-1)*w + MIN((x_flr+1),w-1)];
                gint16 px01 = bil->data[MIN((y_flr+1),h-1)*w + MIN((x_flr  ),w-1)];
                gint16 px11 = bil->data[MIN((y_flr+1),h-1)*w + MIN((x_flr+1),w-1)];

		elev =  px00 * (1-x_rem) * (1-y_rem) +
		        px10 * (  x_rem) * (1-y_rem) +
		        px01 * (1-x_rem) * (  y_rem) +
		        px11 * (  x_rem) * (  y_rem);
		//g_message("elev=%f -- %hd %hd %hd %hd",
		//	elev, px00, px10, px01, px11);
	} else {
		elev = 0;
	}

	lle2xyz(lat, lon, elev, &point->x, &point->y, &point->z);
}

void roam_tri_func(RoamTriangle *tri, gpointer _self)
{
#ifdef ROAM_DEBUG
	glBegin(GL_TRIANGLES);
	glNormal3dv(tri->p.r->norm); glVertex3dv((double*)tri->p.r); 
	glNormal3dv(tri->p.m->norm); glVertex3dv((double*)tri->p.m); 
	glNormal3dv(tri->p.l->norm); glVertex3dv((double*)tri->p.l); 
	glEnd();
	return;
#endif

	GisOpenGL *self = _self;
	if (tri->error < 0) return;

	/* Get lat-lon min and maxes for the triangle */
	gdouble lat[3], lon[3], elev[3];
	xyz2lle(tri->p.r->x, tri->p.r->y, tri->p.r->z, &lat[0], &lon[0], &elev[0]);
	xyz2lle(tri->p.m->x, tri->p.m->y, tri->p.m->z, &lat[1], &lon[1], &elev[1]);
	xyz2lle(tri->p.l->x, tri->p.l->y, tri->p.l->z, &lat[2], &lon[2], &elev[2]);
	gdouble lat_max = MAX(MAX(lat[0], lat[1]), lat[2]);
	gdouble lat_min = MIN(MIN(lat[0], lat[1]), lat[2]);
	gdouble lat_avg = (lat_min+lat_max)/2;
	gdouble lon_max = MAX(MAX(lon[0], lon[1]), lon[2]);
	gdouble lon_min = MIN(MIN(lon[0], lon[1]), lon[2]);
	gdouble lon_avg = (lon_min+lon_max)/2;

	/* Get target resolution */
	gdouble cam_lle[3], cam_xyz[3];
	gis_view_get_location(self->view, &cam_lle[0], &cam_lle[1], &cam_lle[2]);
	lle2xyz(cam_lle[0], cam_lle[1], cam_lle[2], &cam_xyz[0], &cam_xyz[1], &cam_xyz[2]);
	gdouble distr = distd(cam_xyz, (double*)tri->p.r);
	gdouble distm = distd(cam_xyz, (double*)tri->p.m);
	gdouble distl = distd(cam_xyz, (double*)tri->p.l);
	double res = MPPX(MIN(MIN(distr, distm), distl));

	/* TODO: 
	 *   - Fetch needed textures, not all corners
	 *   - Also fetch center textures that aren't touched by a corner
	 *   - Idea: send {lat,lon}{min,max} to fetch_cache and handle it in the recursion */
	/* Fetch textures */
	tri->nodes[0] = wms_info_fetch_cache(self->bmng, tri->nodes[0], res, lat_min, lon_min, NULL, roam_queue_draw, self);
	tri->nodes[1] = wms_info_fetch_cache(self->bmng, tri->nodes[1], res, lat_max, lon_min, NULL, roam_queue_draw, self);
	tri->nodes[2] = wms_info_fetch_cache(self->bmng, tri->nodes[2], res, lat_min, lon_max, NULL, roam_queue_draw, self);
	tri->nodes[3] = wms_info_fetch_cache(self->bmng, tri->nodes[3], res, lat_max, lon_max, NULL, roam_queue_draw, self);
	tri->nodes[4] = wms_info_fetch_cache(self->bmng, tri->nodes[4], res, lat_avg, lon_avg, NULL, roam_queue_draw, self);
	/* Hopefully get all textures at the same resolution to prevent overlaps */
	//gdouble maxres = 0;
	//for (int i = 0; i < 5; i++)
	//	if (tri->nodes[i] && tri->nodes[i]->res > maxres)
	//		maxres = tri->nodes[i]->res;
	//if (maxres != 0) {
	//	tri->nodes[0] = wms_info_fetch_cache(self->bmng, tri->nodes[0], maxres, lat_min, lon_min, NULL, roam_queue_draw, self);
	//	tri->nodes[1] = wms_info_fetch_cache(self->bmng, tri->nodes[1], maxres, lat_max, lon_min, NULL, roam_queue_draw, self);
	//	tri->nodes[2] = wms_info_fetch_cache(self->bmng, tri->nodes[2], maxres, lat_min, lon_max, NULL, roam_queue_draw, self);
	//	tri->nodes[3] = wms_info_fetch_cache(self->bmng, tri->nodes[3], maxres, lat_max, lon_max, NULL, roam_queue_draw, self);
	//	tri->nodes[4] = wms_info_fetch_cache(self->bmng, tri->nodes[4], maxres, lat_avg, lon_avg, NULL, roam_queue_draw, self);
	//}

	/* Vertex color for hieght map viewing, 8848m == Everest */
	gfloat colors[] = {
		(elev[0]-EARTH_R)/8848,
		(elev[1]-EARTH_R)/8848,
		(elev[2]-EARTH_R)/8848,
	};

	/* Draw each texture */
	/* TODO: Prevent double exposure when of hi-res textures on top of
	 * low-res textures when some high-res textures are not yet loaded. */
	glBlendFunc(GL_ONE, GL_ZERO);
	for (int i = 0; i < 5; i++) {
		/* Skip missing textures */
		if (tri->nodes[i] == NULL)
			continue;
		/* Skip already drawn textures */
		switch (i) {
		case 4: if (tri->nodes[i] == tri->nodes[3]) continue;
		case 3: if (tri->nodes[i] == tri->nodes[2]) continue;
		case 2: if (tri->nodes[i] == tri->nodes[1]) continue;
		case 1: if (tri->nodes[i] == tri->nodes[0]) continue;
		}

		WmsCacheNode *node = tri->nodes[i];

		if (node->latlon[0] == -180) {
			if (lon[0] < -90 || lon[1] < -90 || lon[2] < -90) {
				if (lon[0] > 90) lon[0] -= 360;
				if (lon[1] > 90) lon[1] -= 360;
				if (lon[2] > 90) lon[2] -= 360;
			}
		} else if (node->latlon[2] == 180.0) {
			if (lon[0] < -90) lon[0] += 360;
			if (lon[1] < -90) lon[1] += 360;
			if (lon[2] < -90) lon[2] += 360;
		}

		gdouble xmin  = node->latlon[0];
		gdouble ymin  = node->latlon[1];
		gdouble xmax  = node->latlon[2];
		gdouble ymax  = node->latlon[3];

		gdouble xdist = xmax - xmin;
		gdouble ydist = ymax - ymin;

		gdouble xy[][3] = {
			{(lon[0]-xmin)/xdist, 1-(lat[0]-ymin)/ydist},
			{(lon[1]-xmin)/xdist, 1-(lat[1]-ymin)/ydist},
			{(lon[2]-xmin)/xdist, 1-(lat[2]-ymin)/ydist},
		};

		glBindTexture(GL_TEXTURE_2D, *(guint*)node->data);

		glBegin(GL_TRIANGLES);
		glColor3fv(colors); glNormal3dv(tri->p.r->norm); glTexCoord2dv(xy[0]); glVertex3dv((double*)tri->p.r); 
		glColor3fv(colors); glNormal3dv(tri->p.m->norm); glTexCoord2dv(xy[1]); glVertex3dv((double*)tri->p.m); 
		glColor3fv(colors); glNormal3dv(tri->p.l->norm); glTexCoord2dv(xy[2]); glVertex3dv((double*)tri->p.l); 
		glEnd();
		glBlendFunc(GL_ONE, GL_ONE);
	}
}

static void set_camera(GisOpenGL *self)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	double lat, lon, elev, rx, ry, rz;
	gis_view_get_location(self->view, &lat, &lon, &elev);
	gis_view_get_rotation(self->view, &rx, &ry, &rz);
	glRotatef(rx, 1, 0, 0);
	glRotatef(rz, 0, 0, 1);
	glTranslatef(0, 0, -elev2rad(elev));
	glRotatef(lat, 1, 0, 0);
	glRotatef(-lon, 0, 1, 0);
}

static void set_visuals(GisOpenGL *self)
{
	/* Lighting */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#ifdef ROAM_DEBUG
	float light_ambient[]  = {0.7f, 0.7f, 0.7f, 1.0f};
	float light_diffuse[]  = {2.0f, 2.0f, 2.0f, 1.0f};
#else
	float light_ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
	float light_diffuse[]  = {5.0f, 5.0f, 5.0f, 1.0f};
#endif
	float light_position[] = {-13*EARTH_R, 1*EARTH_R, 3*EARTH_R, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	float material_ambient[]  = {0.2, 0.2, 0.2, 1.0};
	float material_diffuse[]  = {0.8, 0.8, 0.8, 1.0};
	float material_specular[] = {0.0, 0.0, 0.0, 1.0};
	float material_emission[] = {0.0, 0.0, 0.0, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_emission);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_COLOR_MATERIAL);

	/* Camera */
	set_camera(self);

	/* Misc */
	gdouble lat, lon, elev;
	gis_view_get_location(self->view, &lat, &lon, &elev);
	gdouble rg   = MAX(0, 1-(elev/20000));
	gdouble blue = MAX(0, 1-(elev/50000));
	glClearColor(MIN(0.65,rg), MIN(0.65,rg), MIN(1,blue), 1.0f);

	glDisable(GL_ALPHA_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

#ifndef ROAM_DEBUG
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
#endif

	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LINE_SMOOTH);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glShadeModel(GL_FLAT);
}


/*************
 * Callbacks *
 *************/
static void on_realize(GisOpenGL *self, gpointer _)
{
	set_visuals(self);
}
static gboolean on_configure(GisOpenGL *self, GdkEventConfigure *event, gpointer _)
{
	g_debug("GisOpenGL: on_confiure");
	gis_opengl_begin(self);

	double width  = GTK_WIDGET(self)->allocation.width;
	double height = GTK_WIDGET(self)->allocation.height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double ang = atan(height/FOV_DIST);
	gluPerspective(rad2deg(ang)*2, width/height, 1, 20*EARTH_R);

#ifndef ROAM_DEBUG
	roam_sphere_update(self->sphere);
#endif

	gis_opengl_end(self);
	return FALSE;
}

static void on_expose_plugin(GisPlugin *plugin, gchar *name, GisOpenGL *self)
{
	set_visuals(self);
	glMatrixMode(GL_PROJECTION); glPushMatrix();
	glMatrixMode(GL_MODELVIEW);  glPushMatrix();
	gis_plugin_expose(plugin);
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}
static gboolean on_expose(GisOpenGL *self, GdkEventExpose *event, gpointer _)
{
	g_debug("GisOpenGL: on_expose - begin");
	gis_opengl_begin(self);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef ROAM_DEBUG
	set_visuals(self);
	glEnable(GL_TEXTURE_2D);
	roam_sphere_draw(self->sphere);
#endif

#ifdef  ROAM_DEBUG
	set_visuals(self);
	glColor4f(0.0, 0.0, 9.0, 0.6);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	roam_sphere_draw(self->sphere);
#endif

	//glDisable(GL_TEXTURE_2D);
	//glEnable(GL_COLOR_MATERIAL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//roam_sphere_draw(self->sphere);

	gis_plugins_foreach(self->plugins, G_CALLBACK(on_expose_plugin), self);

	set_visuals(self);
	gis_opengl_end(self);
	gis_opengl_flush(self);
	g_debug("GisOpenGL: on_expose - end\n");
	return FALSE;
}

static gboolean on_button_press(GisOpenGL *self, GdkEventButton *event, gpointer _)
{
	g_debug("GisOpenGL: on_button_press - Grabbing focus");
	gtk_widget_grab_focus(GTK_WIDGET(self));
	return TRUE;
}

static gboolean on_key_press(GisOpenGL *self, GdkEventKey *event, gpointer _)
{
	g_debug("GisOpenGL: on_key_press - key=%x, state=%x, plus=%x",
			event->keyval, event->state, GDK_plus);

	double lat, lon, elev, pan;
	gis_view_get_location(self->view, &lat, &lon, &elev);
	pan = MIN(elev/(EARTH_R/2), 30);
	guint kv = event->keyval;
	if      (kv == GDK_Left  || kv == GDK_h) gis_view_pan(self->view,  0,  -pan, 0);
	else if (kv == GDK_Down  || kv == GDK_j) gis_view_pan(self->view, -pan, 0,   0);
	else if (kv == GDK_Up    || kv == GDK_k) gis_view_pan(self->view,  pan, 0,   0);
	else if (kv == GDK_Right || kv == GDK_l) gis_view_pan(self->view,  0,   pan, 0);
	else if (kv == GDK_minus || kv == GDK_o) gis_view_zoom(self->view, 10./9);
	else if (kv == GDK_plus  || kv == GDK_i) gis_view_zoom(self->view, 9./10);
	else if (kv == GDK_H) gis_view_rotate(self->view,  0,  0, -10);
	else if (kv == GDK_J) gis_view_rotate(self->view,  10, 0,  0);
	else if (kv == GDK_K) gis_view_rotate(self->view, -10, 0,  0);
	else if (kv == GDK_L) gis_view_rotate(self->view,  0,  0,  10);

	/* Testing */
#ifdef ROAM_DEBUG
	else if (kv == GDK_n) roam_sphere_split_one(self->sphere);
	else if (kv == GDK_p) roam_sphere_merge_one(self->sphere);
	else if (kv == GDK_r) roam_sphere_split_merge(self->sphere);
	else if (kv == GDK_u) roam_sphere_update(self->sphere);
	gtk_widget_queue_draw(GTK_WIDGET(self));
#endif

	return TRUE;
}

static void on_view_changed(GisView *view,
		gdouble _1, gdouble _2, gdouble _3, GisOpenGL *self)
{
	gis_opengl_begin(self);
	set_visuals(self);
#ifndef ROAM_DEBUG
	roam_sphere_update(self->sphere);
#endif
	gis_opengl_redraw(self);
	gis_opengl_end(self);
}

static gboolean on_idle(GisOpenGL *self)
{
	gis_opengl_begin(self);
	if (roam_sphere_split_merge(self->sphere))
		gis_opengl_redraw(self);
	gis_opengl_end(self);
	return TRUE;
}


/***********
 * Methods *
 ***********/
GisOpenGL *gis_opengl_new(GisWorld *world, GisView *view, GisPlugins *plugins)
{
	g_debug("GisOpenGL: new");
	GisOpenGL *self = g_object_new(GIS_TYPE_OPENGL, NULL);
	self->world   = world;
	self->view    = view;
	self->plugins = plugins;
	g_object_ref(world);
	g_object_ref(view);

	g_signal_connect(self->view, "location-changed", G_CALLBACK(on_view_changed), self);
	g_signal_connect(self->view, "rotation-changed", G_CALLBACK(on_view_changed), self);

	/* TODO: update point eights sometime later so we have heigh-res heights for them */
	self->sphere = roam_sphere_new(roam_tri_func, roam_height_func, self);

	return g_object_ref(self);
}

void gis_opengl_center_position(GisOpenGL *self, gdouble lat, gdouble lon, gdouble elev)
{
	set_camera(self);
	glRotatef(lon, 0, 1, 0);
	glRotatef(-lat, 1, 0, 0);
	glTranslatef(0, 0, elev2rad(elev));
}

void gis_opengl_redraw(GisOpenGL *self)
{
	g_debug("GisOpenGL: gl_redraw");
	gtk_widget_queue_draw(GTK_WIDGET(self));
}
void gis_opengl_begin(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));

	GdkGLContext   *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(self));
	GdkGLDrawable  *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();
}
void gis_opengl_end(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));
	gdk_gl_drawable_gl_end(gldrawable);
}
void gis_opengl_flush(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));
	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();
	gdk_gl_drawable_gl_end(gldrawable);
}


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisOpenGL, gis_opengl, GTK_TYPE_DRAWING_AREA);
static void gis_opengl_init(GisOpenGL *self)
{
	g_debug("GisOpenGL: init");
	self->bmng = wms_info_new_for_bmng(NULL, NULL);
	self->srtm = wms_info_new_for_srtm(NULL, NULL);

	/* OpenGL setup */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(GTK_WIDGET(self),
				glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");
	g_object_unref(glconfig);

	gtk_widget_set_size_request(GTK_WIDGET(self), 600, 550);
	gtk_widget_set_events(GTK_WIDGET(self),
			GDK_BUTTON_PRESS_MASK |
			GDK_ENTER_NOTIFY_MASK |
			GDK_KEY_PRESS_MASK);
	g_object_set(self, "can-focus", TRUE, NULL);

#ifndef ROAM_DEBUG
	self->sm_source = g_timeout_add(10, (GSourceFunc)on_idle, self);
#endif

	g_signal_connect(self, "realize",            G_CALLBACK(on_realize),      NULL);
	g_signal_connect(self, "configure-event",    G_CALLBACK(on_configure),    NULL);
	g_signal_connect(self, "expose-event",       G_CALLBACK(on_expose),       NULL);

	g_signal_connect(self, "button-press-event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(self, "enter-notify-event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(self, "key-press-event",    G_CALLBACK(on_key_press),    NULL);
}
static GObject *gis_opengl_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("GisOpengl: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(gis_opengl_parent_class);
	return parent_class->constructor(gtype, n_properties, properties);
}
static void gis_opengl_dispose(GObject *_self)
{
	g_debug("GisOpenGL: dispose");
	GisOpenGL *self = GIS_OPENGL(_self);
	if (self->sm_source) {
		g_source_remove(self->sm_source);
		self->sm_source = 0;
	}
	if (self->sphere) {
		roam_sphere_free(self->sphere);
		self->sphere = NULL;
	}
	if (self->world) {
		g_object_unref(self->world);
		self->world = NULL;
	}
	if (self->view) {
		g_object_unref(self->view);
		self->view = NULL;
	}
	G_OBJECT_CLASS(gis_opengl_parent_class)->dispose(_self);
}
static void gis_opengl_finalize(GObject *_self)
{
	g_debug("GisOpenGL: finalize");
	GisOpenGL *self = GIS_OPENGL(_self);
	wms_info_free(self->bmng);
	wms_info_free(self->srtm);
	G_OBJECT_CLASS(gis_opengl_parent_class)->finalize(_self);
}
static void gis_opengl_class_init(GisOpenGLClass *klass)
{
	g_debug("GisOpenGL: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = gis_opengl_constructor;
	gobject_class->dispose      = gis_opengl_dispose;
	gobject_class->finalize     = gis_opengl_finalize;
}
