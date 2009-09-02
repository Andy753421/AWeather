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
 *   - Remove/free unused memory
 *   - Optimize for memory consumption
 *   - Profile for computation speed
 *   - Implement good error algorithm
 *   - Target polygon count/detail
 */

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
void roam_point_add_triangle(RoamPoint *self, RoamTriangle *triangle)
{
	for (int i = 0; i < 3; i++) {
		self->norm[i] *= self->refs;
		self->norm[i] += triangle->norm[i];
	}
	self->refs++;
	for (int i = 0; i < 3; i++)
		self->norm[i] /= self->refs;
}
void roam_point_remove_triangle(RoamPoint *self, RoamTriangle *triangle)
{
	for (int i = 0; i < 3; i++) {
		self->norm[i] *= self->refs;
		self->norm[i] -= triangle->norm[i];
	}
	self->refs--;
	for (int i = 0; i < 3; i++)
		self->norm[i] /= self->refs;
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

	roam_triangle_update_error(self, sphere, NULL);

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

gboolean roam_triangle_update_error_visible(RoamPoint *point, double *model, double *proj)
{
	double x, y, z;
	int view[4] = {0,0,1,1};
	if (!gluProject(point->x, point->y, point->z, model, proj, view, &x, &y, &z)) {
		g_warning("RoamTriangle: update_error_visible - gluProject failed");
		return TRUE;
	}
	return !(x < 0 || x > 1 ||
	         y < 0 || y > 1 ||
	         z < 0 || z > 1);
}

void roam_triangle_update_error(RoamTriangle *self, RoamSphere *sphere, GPQueue *triangles)
{
	RoamPoint cur = {
		(self->p.l->x + self->p.r->x)/2,
		(self->p.l->y + self->p.r->y)/2,
		(self->p.l->z + self->p.r->z)/2,
	};
	RoamPoint good = cur;
	roam_sphere_update_point(sphere, &good);

	//g_message("cur = (%+5.2f %+5.2f %+5.2f)  good = (%+5.2f %+5.2f %+5.2f)",
	//	cur[0], cur[1], cur[2], good[0], good[1], good[2]);

	double model[16], proj[16]; 
	int view[4]; 
	glGetError();
	glGetDoublev(GL_MODELVIEW_MATRIX, model); 
	glGetDoublev(GL_PROJECTION_MATRIX, proj); 
	glGetIntegerv(GL_VIEWPORT, view);          
	if (glGetError() != GL_NO_ERROR) {
		g_warning("RoamTriangle: update_error - glGet failed");
		return;
	}

	double scur[3], sgood[3];
	gluProject( cur.x, cur.y, cur.z, model,proj,view,  &scur[0], &scur[1], &scur[2]);
	gluProject(good.x,good.y,good.z, model,proj,view, &sgood[0],&sgood[1],&sgood[2]);

	/* Not exactly correct, could be out on both sides (middle in) */
	if (!roam_triangle_update_error_visible(self->p.l, model, proj) && 
	    !roam_triangle_update_error_visible(self->p.m, model, proj) &&
	    !roam_triangle_update_error_visible(self->p.r, model, proj) &&
	    !roam_triangle_update_error_visible(&good,     model, proj)) {
		self->error = -1;
	} else {
		self->error = sqrt((scur[0]-sgood[0])*(scur[0]-sgood[0]) +
				   (scur[1]-sgood[1])*(scur[1]-sgood[1]));

		/* Multiply by size of triangle (todo, do this in screen space) */
		double sa[3], sb[3], sc[3];
		double *a = (double*)self->p.l;
		double *b = (double*)self->p.m;
		double *c = (double*)self->p.r;
		gluProject(a[0],a[1],a[2], model,proj,view, &sa[0],&sa[1],&sa[2]);
		gluProject(b[0],b[1],b[2], model,proj,view, &sb[0],&sb[1],&sb[2]);
		gluProject(c[0],c[1],c[2], model,proj,view, &sc[0],&sc[1],&sc[2]);
		double size = -( sa[0]*(sb[1]-sc[1]) + sb[0]*(sc[1]-sa[1]) + sc[0]*(sa[1]-sb[1]) ) / 2.0;
		//g_message("%f * %f = %f", self->error, size, self->error * size);
		/* Size < 0 == backface */
		self->error *= size;
	}

	if (triangles)
		g_pqueue_priority_changed(triangles, self->handle);
}

void roam_triangle_split(RoamTriangle *self, RoamSphere *sphere)
{
	//g_message("roam_triangle_split: %p, e=%f\n", self, self->error);

	sphere->polys += 2;

	if (self != self->t.b->t.b)
		roam_triangle_split(self->t.b, sphere);

	RoamTriangle *base = self->t.b;

	/* Calculate midpoint */
	RoamPoint *mid = roam_point_new(
		(self->p.l->x + self->p.r->x)/2,
		(self->p.l->y + self->p.r->y)/2,
		(self->p.l->z + self->p.r->z)/2
	);
	roam_sphere_update_point(sphere, mid);

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
	roam_diamond_add(diamond, sphere);
	roam_diamond_remove(self->diamond, sphere);
	roam_diamond_remove(base->diamond, sphere);
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

	self->kid[0] = kid0;
	self->kid[1] = kid1;
	self->kid[2] = kid2;
	self->kid[3] = kid3;

	kid0->diamond = self; 
	kid1->diamond = self; 
	kid2->diamond = self; 
	kid3->diamond = self; 

	self->parent[0] = parent0;
	self->parent[1] = parent1;

	return self;
}
void roam_diamond_add(RoamDiamond *self, RoamSphere *sphere)
{
	self->active = TRUE;
	roam_diamond_update_error(self, sphere, NULL);
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
	RoamTriangle **kid = self->kid;

	/* Create triangles */
	RoamTriangle *tri  = self->parent[0];
	RoamTriangle *base = self->parent[1];

	roam_triangle_add(tri,  kid[0]->t.b, base, kid[1]->t.b, sphere);
	roam_triangle_add(base, kid[2]->t.b, tri,  kid[3]->t.b, sphere);

	roam_triangle_sync_neighbors(tri,  kid[0], kid[0]->t.b);
	roam_triangle_sync_neighbors(tri,  kid[1], kid[1]->t.b);
	roam_triangle_sync_neighbors(base, kid[2], kid[2]->t.b);
	roam_triangle_sync_neighbors(base, kid[3], kid[3]->t.b);

	/* Remove triangles */
	roam_triangle_remove(kid[0], sphere);
	roam_triangle_remove(kid[1], sphere);
	roam_triangle_remove(kid[2], sphere);
	roam_triangle_remove(kid[3], sphere);

	/* Add/Remove triangles */
	if (tri->t.l->t.l == tri->t.r->t.r &&
	    tri->t.l->t.l != tri && tri->diamond)
		roam_diamond_add(tri->diamond, sphere);

	if (base->t.l->t.l == base->t.r->t.r &&
	    base->t.l->t.l != base && base->diamond)
		roam_diamond_add(base->diamond, sphere);
 
	/* Remove and free diamond and child triangles */
	roam_diamond_remove(self, sphere);
	g_assert(self->kid[0]->p.m == self->kid[1]->p.m &&
	         self->kid[1]->p.m == self->kid[2]->p.m &&
	         self->kid[2]->p.m == self->kid[3]->p.m);
	g_assert(self->kid[0]->p.m->refs == 0);
	g_free(self->kid[0]->p.m);
	g_free(self->kid[0]);
	g_free(self->kid[1]);
	g_free(self->kid[2]);
	g_free(self->kid[3]);
	g_free(self);
}
void roam_diamond_update_error(RoamDiamond *self, RoamSphere *sphere, GPQueue *diamonds)
{
	roam_triangle_update_error(self->parent[0], sphere, NULL);
	roam_triangle_update_error(self->parent[1], sphere, NULL);
	self->error = MAX(self->parent[0]->error, self->parent[1]->error);
	if (diamonds)
		g_pqueue_priority_changed(diamonds, self->handle);
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

	for (int i = 0; i < 8; i++)
		roam_sphere_update_point(self, vertexes[i]);
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
static void roam_sphere_update_errors_cb(gpointer obj, GPtrArray *ptrs)
{
	g_ptr_array_add(ptrs, obj);
}
void roam_sphere_update_errors(RoamSphere *self)
{
	g_debug("RoamSphere: update_errors - polys=%d", self->polys);
	GPtrArray *ptrs = g_ptr_array_new();
	g_pqueue_foreach(self->triangles, (GFunc)roam_sphere_update_errors_cb, ptrs);
	for (int i = 0; i < ptrs->len; i++)
		roam_triangle_update_error(ptrs->pdata[i], self, self->triangles);
	g_ptr_array_free(ptrs, TRUE);

	ptrs = g_ptr_array_new();
	g_pqueue_foreach(self->diamonds, (GFunc)roam_sphere_update_errors_cb, ptrs);
	for (int i = 0; i < ptrs->len; i++)
		roam_diamond_update_error(ptrs->pdata[i], self, self->diamonds);
	g_ptr_array_free(ptrs, TRUE);
}
void roam_sphere_update_point(RoamSphere *self, RoamPoint *point)
{
	self->height_func(point, self->user_data);
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
	while (self->polys < target && iters++ < max_iters)
		roam_sphere_split_one(self);
	while (self->polys > target && iters++ < max_iters)
		roam_sphere_merge_one(self);
	while (((RoamTriangle*)g_pqueue_peek(self->triangles))->error >
	       ((RoamDiamond *)g_pqueue_peek(self->diamonds ))->error &&
	       iters++ < max_iters) {
		roam_sphere_split_one(self);
		if (((RoamTriangle*)g_pqueue_peek(self->triangles))->error >
		    ((RoamDiamond *)g_pqueue_peek(self->diamonds ))->error)
			roam_sphere_merge_one(self);
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
	if (--tri->p.l->refs == 0) g_free(tri->p.l);
	if (--tri->p.m->refs == 0) g_free(tri->p.m);
	if (--tri->p.r->refs == 0) g_free(tri->p.r);
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
