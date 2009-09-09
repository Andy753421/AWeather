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

#include <glib.h>
#include <math.h>
#include <string.h>
#include "gpqueue.h"
#include <GL/gl.h>
#include <GL/glu.h>

#include "roam.h"

/**
 * TODO:
 *   - Optimize for memory consumption
 *   - Profile for computation speed
 *   - Target polygon count/detail
 */

/* Misc */
RoamView *roam_view_get()
{
	RoamView *view = g_new0(RoamView, 1);
	glGetDoublev (GL_MODELVIEW_MATRIX,  view->model); 
	glGetDoublev (GL_PROJECTION_MATRIX, view->proj); 
	glGetIntegerv(GL_VIEWPORT,          view->view);          
	return view;
}

/* For GPQueue comparators */
static gint tri_cmp(RoamTriangle *a, RoamTriangle *b, gpointer data)
{
	if      (a->error < b->error) return  1;
	else if (a->error > b->error) return -1;
	else                          return  0;
}
static gint dia_cmp(RoamDiamond *a, RoamDiamond *b, gpointer data)
{
	if      (a->error < b->error) return -1;
	else if (a->error > b->error) return  1;
	else                          return  0;
}

/*************
 * RoamPoint *
 *************/
RoamPoint *roam_point_new(double x, double y, double z)
{
	RoamPoint *self = g_new0(RoamPoint, 1);
	self->x = x;
	self->y = y;
	self->z = z;
	return self;
}
RoamPoint *roam_point_dup(RoamPoint *self)
{
	RoamPoint *new = g_memdup(self, sizeof(RoamPoint));
	new->tris = 0;
	return new;
}
void roam_point_add_triangle(RoamPoint *self, RoamTriangle *triangle)
{
	for (int i = 0; i < 3; i++) {
		self->norm[i] *= self->tris;
		self->norm[i] += triangle->norm[i];
	}
	self->tris++;
	for (int i = 0; i < 3; i++)
		self->norm[i] /= self->tris;
}
void roam_point_remove_triangle(RoamPoint *self, RoamTriangle *triangle)
{
	for (int i = 0; i < 3; i++) {
		self->norm[i] *= self->tris;
		self->norm[i] -= triangle->norm[i];
	}
	self->tris--;
	for (int i = 0; i < 3; i++)
		self->norm[i] /= self->tris;
}
void roam_point_update(RoamPoint *self, RoamSphere *sphere, gboolean do_height)
{
	if (!self->cached) {
		/* Cache height */
		if (do_height)
			sphere->height_func(self, sphere->user_data);

		/* Cache projection */
		gluProject(self->x, self->y, self->z,
			sphere->view->model, sphere->view->proj, sphere->view->view,
			&self->px, &self->py, &self->pz);

		self->cached = TRUE;
	}
}
void roam_point_clear(RoamPoint *self)
{
	self->cached = FALSE;
}

/****************
 * RoamTriangle *
 ****************/
RoamTriangle *roam_triangle_new(RoamPoint *l, RoamPoint *m, RoamPoint *r)
{
	RoamTriangle *self = g_new0(RoamTriangle, 1);

	self->p.l = l;
	self->p.m = m;
	self->p.r = r;

	self->error = 0;

	/* Update normal */
	double pa[3];
	double pb[3];
	pa[0] = self->p.l->x - self->p.m->x;
	pa[1] = self->p.l->y - self->p.m->y;
	pa[2] = self->p.l->z - self->p.m->z;

	pb[0] = self->p.r->x - self->p.m->x;
	pb[1] = self->p.r->y - self->p.m->y;
	pb[2] = self->p.r->z - self->p.m->z;

	self->norm[0] = pa[1] * pb[2] - pa[2] * pb[1];
	self->norm[1] = pa[2] * pb[0] - pa[0] * pb[2];
	self->norm[2] = pa[0] * pb[1] - pa[1] * pb[0];

	double total = sqrt(self->norm[0] * self->norm[0] +
	                    self->norm[1] * self->norm[1] +
	                    self->norm[2] * self->norm[2]);

	self->norm[0] /= total;
	self->norm[1] /= total;
	self->norm[2] /= total;
	
	//g_message("roam_triangle_new: %p", self);
	return self;
}

void roam_triangle_add(RoamTriangle *self,
		RoamTriangle *left, RoamTriangle *base, RoamTriangle *right,
		RoamSphere *sphere)
{
	self->t.l = left;
	self->t.b = base;
	self->t.r = right;

	roam_point_add_triangle(self->p.l, self);
	roam_point_add_triangle(self->p.m, self);
	roam_point_add_triangle(self->p.r, self);

	if (sphere->view)
		roam_triangle_update(self, sphere);

	self->handle = g_pqueue_push(sphere->triangles, self);
}

void roam_triangle_remove(RoamTriangle *self, RoamSphere *sphere)
{
	/* Update vertex normals */
	roam_point_remove_triangle(self->p.l, self);
	roam_point_remove_triangle(self->p.m, self);
	roam_point_remove_triangle(self->p.r, self);

	g_pqueue_remove(sphere->triangles, self->handle);
}

void roam_triangle_sync_neighbors(RoamTriangle *new, RoamTriangle *old, RoamTriangle *neigh)
{
	if      (neigh->t.l == old) neigh->t.l = new;
	else if (neigh->t.b == old) neigh->t.b = new;
	else if (neigh->t.r == old) neigh->t.r = new;
	else g_assert_not_reached();
}

gboolean roam_point_visible(RoamPoint *self, RoamSphere *sphere)
{
	gint *view = sphere->view->view;
	return self->px > view[0] && self->px < view[2] &&
	       self->py > view[1] && self->py < view[3] &&
	       self->pz > 0       && self->pz < 1;
	//double x, y, z;
	//int view[4] = {0,0,1,1};
	//gluProject(self->x, self->y, self->z,
	//		sphere->view->model, sphere->view->proj, view,
	//		&x, &y, &z);
	//return !(x < 0 || x > 1 ||
	//         y < 0 || y > 1 ||
	//         z < 0 || z > 1);
}
gboolean roam_triangle_visible(RoamTriangle *self, RoamSphere *sphere)
{
	/* Do this with a bounding box */
	return roam_point_visible(self->p.l, sphere) ||
	       roam_point_visible(self->p.m, sphere) ||
	       roam_point_visible(self->p.r, sphere);
}

void roam_triangle_update(RoamTriangle *self, RoamSphere *sphere)
{
	/* Update points */
	roam_point_update(self->p.l, sphere, TRUE);
	roam_point_update(self->p.m, sphere, TRUE);
	roam_point_update(self->p.r, sphere, TRUE);

	/* Not exactly correct, could be out on both sides (middle in) */
	if (!roam_triangle_visible(self, sphere)) {
		self->error = -1;
	} else {
		RoamPoint *l = self->p.l;
		RoamPoint *m = self->p.m;
		RoamPoint *r = self->p.r;

		/* TODO: share this with the base */
		RoamPoint base = { (l->x + r->x)/2,
		                   (l->y + r->y)/2,
		                   (l->z + r->z)/2 };
		RoamPoint good = base;
		roam_point_update(&base, sphere, FALSE);
		roam_point_update(&good, sphere, TRUE);

		self->error = sqrt((base.px - good.px) * (base.px - good.px) +
				   (base.py - good.py) * (base.py - good.py));

		/* Multiply by size of triangle */
		double size = -( l->px * (m->py - r->py) +
		                 m->px * (r->py - l->py) +
		                 r->px * (l->py - m->py) ) / 2.0;

		/* Size < 0 == backface */
		self->error *= size;
	}
}

void roam_triangle_clear(RoamTriangle *self)
{
	/* Clear points */
	roam_point_clear(self->p.l);
	roam_point_clear(self->p.m);
	roam_point_clear(self->p.r);
}

void roam_triangle_split(RoamTriangle *self, RoamSphere *sphere)
{
	//g_message("roam_triangle_split: %p, e=%f\n", self, self->error);

	sphere->polys += 2;

	if (self != self->t.b->t.b)
		roam_triangle_split(self->t.b, sphere);

	RoamTriangle *base = self->t.b;

	/* Duplicate midpoint */
	RoamPoint *mid = roam_point_new(
			(self->p.l->x + self->p.r->x)/2,
			(self->p.l->y + self->p.r->y)/2,
			(self->p.l->z + self->p.r->z)/2);
	roam_point_update(mid, sphere, TRUE);

	/* Add new triangles */
	RoamTriangle *sl = roam_triangle_new(self->p.m, mid, self->p.l); // Self Left
	RoamTriangle *sr = roam_triangle_new(self->p.r, mid, self->p.m); // Self Right
	RoamTriangle *bl = roam_triangle_new(base->p.m, mid, base->p.l); // Base Left
	RoamTriangle *br = roam_triangle_new(base->p.r, mid, base->p.m); // Base Right

	/*                tri,l,  base,      r,  sphere */
	roam_triangle_add(sl, sr, self->t.l, br, sphere);
	roam_triangle_add(sr, bl, self->t.r, sl, sphere);
	roam_triangle_add(bl, br, base->t.l, sr, sphere);
	roam_triangle_add(br, sl, base->t.r, bl, sphere);

	roam_triangle_sync_neighbors(sl, self, self->t.l);
	roam_triangle_sync_neighbors(sr, self, self->t.r);
	roam_triangle_sync_neighbors(bl, base, base->t.l);
	roam_triangle_sync_neighbors(br, base, base->t.r);

	/* Remove old triangles */
	roam_triangle_remove(self, sphere);
	roam_triangle_remove(base, sphere);

	/* Add/Remove diamonds */
	RoamDiamond *diamond = roam_diamond_new(self, base, sl, sr, bl, br);
	roam_diamond_update(diamond, sphere);
	roam_diamond_add(diamond, sphere);
	roam_diamond_remove(self->parent, sphere);
	roam_diamond_remove(base->parent, sphere);
}

void roam_triangle_draw_normal(RoamTriangle *self)
{
	double center[] = {
		(self->p.l->x + self->p.m->x + self->p.r->x)/3.0,
		(self->p.l->y + self->p.m->y + self->p.r->y)/3.0,
		(self->p.l->z + self->p.m->z + self->p.r->z)/3.0,
	};
	double end[] = {
		center[0]+self->norm[0]/2,
		center[1]+self->norm[1]/2,
		center[2]+self->norm[2]/2,
	};
	glBegin(GL_LINES);
	glVertex3dv(center);
	glVertex3dv(end);
	glEnd();
}

/***************
 * RoamDiamond *
 ***************/
RoamDiamond *roam_diamond_new(
		RoamTriangle *parent0, RoamTriangle *parent1,
		RoamTriangle *kid0, RoamTriangle *kid1,
		RoamTriangle *kid2, RoamTriangle *kid3)
{
	RoamDiamond *self = g_new0(RoamDiamond, 1);

	self->kids[0] = kid0;
	self->kids[1] = kid1;
	self->kids[2] = kid2;
	self->kids[3] = kid3;

	kid0->parent = self; 
	kid1->parent = self; 
	kid2->parent = self; 
	kid3->parent = self; 

	self->parents[0] = parent0;
	self->parents[1] = parent1;

	return self;
}
void roam_diamond_add(RoamDiamond *self, RoamSphere *sphere)
{
	self->active = TRUE;
	self->error  = MAX(self->parents[0]->error, self->parents[1]->error);
	self->handle = g_pqueue_push(sphere->diamonds, self);
}
void roam_diamond_remove(RoamDiamond *self, RoamSphere *sphere)
{
	if (self && self->active) {
		self->active = FALSE;
		g_pqueue_remove(sphere->diamonds, self->handle);
	}
}
void roam_diamond_merge(RoamDiamond *self, RoamSphere *sphere)
{
	//g_message("roam_diamond_merge: %p, e=%f\n", self, self->error);

	sphere->polys -= 2;

	/* TODO: pick the best split */
	RoamTriangle **kids = self->kids;

	/* Create triangles */
	RoamTriangle *tri  = self->parents[0];
	RoamTriangle *base = self->parents[1];

	roam_triangle_add(tri,  kids[0]->t.b, base, kids[1]->t.b, sphere);
	roam_triangle_add(base, kids[2]->t.b, tri,  kids[3]->t.b, sphere);

	roam_triangle_sync_neighbors(tri,  kids[0], kids[0]->t.b);
	roam_triangle_sync_neighbors(tri,  kids[1], kids[1]->t.b);
	roam_triangle_sync_neighbors(base, kids[2], kids[2]->t.b);
	roam_triangle_sync_neighbors(base, kids[3], kids[3]->t.b);

	/* Remove triangles */
	roam_triangle_remove(kids[0], sphere);
	roam_triangle_remove(kids[1], sphere);
	roam_triangle_remove(kids[2], sphere);
	roam_triangle_remove(kids[3], sphere);

	/* Add/Remove triangles */
	if (tri->t.l->t.l == tri->t.r->t.r &&
	    tri->t.l->t.l != tri && tri->parent) {
		roam_diamond_update(tri->parent, sphere);
		roam_diamond_add(tri->parent, sphere);
	}

	if (base->t.l->t.l == base->t.r->t.r &&
	    base->t.l->t.l != base && base->parent) {
		roam_diamond_update(base->parent, sphere);
		roam_diamond_add(base->parent, sphere);
	}
 
	/* Remove and free diamond and child triangles */
	roam_diamond_remove(self, sphere);
	g_assert(self->kids[0]->p.m == self->kids[1]->p.m &&
	         self->kids[1]->p.m == self->kids[2]->p.m &&
	         self->kids[2]->p.m == self->kids[3]->p.m);
	g_assert(self->kids[0]->p.m->tris == 0);
	g_free(self->kids[0]->p.m);
	g_free(self->kids[0]);
	g_free(self->kids[1]);
	g_free(self->kids[2]);
	g_free(self->kids[3]);
	g_free(self);
}
void roam_diamond_update(RoamDiamond *self, RoamSphere *sphere)
{
	roam_triangle_update(self->parents[0], sphere);
	roam_triangle_update(self->parents[1], sphere);
	self->error = MAX(self->parents[0]->error, self->parents[1]->error);
}

/**************
 * RoamSphere *
 **************/
RoamSphere *roam_sphere_new(RoamTriFunc tri_func, RoamHeightFunc height_func, gpointer user_data)
{
	RoamSphere *self = g_new0(RoamSphere, 1);
	self->tri_func    = tri_func;
	self->height_func = height_func;
	self->user_data   = user_data;
	self->polys       = 12;
	self->triangles   = g_pqueue_new((GCompareDataFunc)tri_cmp, NULL);
	self->diamonds    = g_pqueue_new((GCompareDataFunc)dia_cmp, NULL);

	RoamPoint *vertexes[] = {
		roam_point_new( 1, 1, 1), // 0
		roam_point_new( 1, 1,-1), // 1
		roam_point_new( 1,-1, 1), // 2
		roam_point_new( 1,-1,-1), // 3
		roam_point_new(-1, 1, 1), // 4
		roam_point_new(-1, 1,-1), // 5
		roam_point_new(-1,-1, 1), // 6
		roam_point_new(-1,-1,-1), // 7
	};
	int _triangles[][2][3] = {
		/*lv mv rv   ln, bn, rn */
		{{3,2,0}, {10, 1, 7}}, // 0
		{{0,1,3}, { 9, 0, 2}}, // 1
		{{7,3,1}, {11, 3, 1}}, // 2
		{{1,5,7}, { 8, 2, 4}}, // 3
		{{6,7,5}, {11, 5, 3}}, // 4
		{{5,4,6}, { 8, 4, 6}}, // 5
		{{2,6,4}, {10, 7, 5}}, // 6
		{{4,0,2}, { 9, 6, 0}}, // 7
		{{4,5,1}, { 5, 9, 3}}, // 8
		{{1,0,4}, { 1, 8, 7}}, // 9
		{{6,2,3}, { 6,11, 0}}, // 10
		{{3,7,6}, { 2,10, 4}}, // 11
	};
	RoamTriangle *triangles[12];

	for (int i = 0; i < 12; i++)
		triangles[i] = roam_triangle_new(
			vertexes[_triangles[i][0][0]],
			vertexes[_triangles[i][0][1]],
			vertexes[_triangles[i][0][2]]);
	for (int i = 0; i < 12; i++)
		roam_triangle_add(triangles[i],
			triangles[_triangles[i][1][0]],
			triangles[_triangles[i][1][1]],
			triangles[_triangles[i][1][2]],
			self);

	return self;
}
void roam_sphere_update(RoamSphere *self)
{
	g_debug("RoamSphere: update - polys=%d", self->polys);
	if (self->view) g_free(self->view);
	self->view = roam_view_get();

	GPtrArray *tris = g_pqueue_get_array(self->triangles);
	GPtrArray *dias = g_pqueue_get_array(self->diamonds);

	for (int i = 0; i < tris->len; i++) {
		/* Note: this also updates points */
		RoamTriangle *tri = tris->pdata[i];
		roam_triangle_clear(tri);
		roam_triangle_update(tri, self);
		g_pqueue_priority_changed(self->triangles, tri->handle);
	}

	for (int i = 0; i < dias->len; i++) {
		RoamDiamond *dia = dias->pdata[i];
		roam_diamond_update(dia, self);
		g_pqueue_priority_changed(self->diamonds, dia->handle);
	}

	g_ptr_array_free(tris, TRUE);
	g_ptr_array_free(dias, TRUE);
}

void roam_sphere_split_one(RoamSphere *self)
{
	RoamTriangle *to_split = g_pqueue_peek(self->triangles);
	if (!to_split) return;
	roam_triangle_split(to_split, self);
}
void roam_sphere_merge_one(RoamSphere *self)
{
	RoamDiamond *to_merge = g_pqueue_peek(self->diamonds);
	if (!to_merge) return;
	roam_diamond_merge(to_merge, self);
}
gint roam_sphere_split_merge(RoamSphere *self)
{
	gint iters = 0, max_iters = 500;
	gint target = 2000;

	if (!self->view)
		return 0;

	if (target - self->polys > 100)
		while (self->polys < target && iters++ < max_iters)
			roam_sphere_split_one(self);

	if (self->polys - target > 100)
		while (self->polys > target && iters++ < max_iters)
			roam_sphere_merge_one(self);

	while (((RoamTriangle*)g_pqueue_peek(self->triangles))->error >
	       ((RoamDiamond *)g_pqueue_peek(self->diamonds ))->error &&
	       iters++ < max_iters) {
		roam_sphere_merge_one(self);
		roam_sphere_split_one(self);
	}

	return iters;
}
void roam_sphere_draw(RoamSphere *self)
{
	g_pqueue_foreach(self->triangles, (GFunc)self->tri_func, self->user_data);
}
void roam_sphere_draw_normals(RoamSphere *self)
{
	g_pqueue_foreach(self->triangles, (GFunc)roam_triangle_draw_normal, NULL);
}
static void roam_sphere_free_tri(RoamTriangle *tri)
{
	if (--tri->p.l->tris == 0) g_free(tri->p.l);
	if (--tri->p.m->tris == 0) g_free(tri->p.m);
	if (--tri->p.r->tris == 0) g_free(tri->p.r);
	g_free(tri);
}
void roam_sphere_free(RoamSphere *self)
{
	/* Slow method, but it should work */
	while (self->polys > 12)
		roam_sphere_merge_one(self);
	/* TODO: free points */
	g_pqueue_foreach(self->triangles, (GFunc)roam_sphere_free_tri, NULL);
	g_pqueue_free(self->triangles);
	g_pqueue_free(self->diamonds);
	g_free(self);
}
