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

#ifndef __ROAM_H__
#define __ROAM_H__

#include "gpqueue.h"
#include "wms.h"

/* Roam */
typedef struct _RoamView     RoamView;
typedef struct _RoamPoint    RoamPoint;
typedef struct _RoamTriangle RoamTriangle;
typedef struct _RoamDiamond  RoamDiamond;
typedef struct _RoamSphere   RoamSphere;
typedef void (*RoamTriFunc)(RoamTriangle *triangle, gpointer user_data);
typedef void (*RoamHeightFunc)(RoamPoint *point, gpointer user_data);

/* Misc */
struct _RoamView {
	gdouble model[16];
	gdouble proj[16]; 
	gint view[4]; 
};

/*************
 * RoamPoint *
 *************/
struct _RoamPoint {
	gdouble  x,y,z;     // Model coordinates
	gdouble  px,py,pz;  // Projected coordinates

	gboolean cached;    // Height/projection cached

	gint     tris;      // Associated triangles
	gdouble  norm[3];   // Vertex normal

	WmsCacheNode *node; // TODO: don't depend on wms
};
RoamPoint *roam_point_new(double x, double y, double z);
void roam_point_add_triangle(RoamPoint *point, RoamTriangle *triangle);
void roam_point_remove_triangle(RoamPoint *point, RoamTriangle *triangle);
void roam_point_update(RoamPoint *point, RoamSphere *sphere, gboolean do_height);
void roam_point_clear(RoamPoint *self);

/****************
 * RoamTriangle *
 ****************/
struct _RoamTriangle {
	struct { RoamPoint    *l,*m,*r; } p;
	struct { RoamTriangle *l,*b,*r; } t;
	RoamDiamond *parent;
	double norm[3];
	double error;
	GPQueueHandle handle;

	WmsCacheNode *nodes[5]; // TODO: don't depend on wms
};
RoamTriangle *roam_triangle_new(RoamPoint *l, RoamPoint *m, RoamPoint *r);
void roam_triangle_add(RoamTriangle *triangle,
		RoamTriangle *left, RoamTriangle *base, RoamTriangle *right,
		RoamSphere *sphere);
void roam_triangle_remove(RoamTriangle *triangle, RoamSphere *sphere);
void roam_triangle_update(RoamTriangle *triangle, RoamSphere *sphere);
void roam_triangle_split(RoamTriangle *triangle, RoamSphere *sphere);
void roam_triangle_draw_normal(RoamTriangle *triangle);

/***************
 * RoamDiamond *
 ***************/
struct _RoamDiamond {
	RoamTriangle *kids[4];
	RoamTriangle *parents[2];
	double error;
	gboolean active;
	GPQueueHandle handle;
};
RoamDiamond *roam_diamond_new(
		RoamTriangle *parent0, RoamTriangle *parent1,
		RoamTriangle *kid0, RoamTriangle *kid1,
		RoamTriangle *kid2, RoamTriangle *kid3);
void roam_diamond_add(RoamDiamond *diamond, RoamSphere *sphere);
void roam_diamond_remove(RoamDiamond *diamond, RoamSphere *sphere);
void roam_diamond_merge(RoamDiamond *diamond, RoamSphere *sphere);
void roam_diamond_update(RoamDiamond *self, RoamSphere *sphere);

/**************
 * RoamSphere *
 **************/
struct _RoamSphere {
	GPQueue *triangles;
	GPQueue *diamonds;
	RoamView *view;
	RoamTriFunc tri_func;
	RoamHeightFunc height_func;
	gpointer user_data;
	gint polys;
};
RoamSphere *roam_sphere_new(RoamTriFunc tri_func, RoamHeightFunc height_func, gpointer user_data);
void roam_sphere_update(RoamSphere *sphere);
void roam_sphere_split_one(RoamSphere *sphere);
void roam_sphere_merge_one(RoamSphere *sphere);
gint roam_sphere_split_merge(RoamSphere *sphere);
void roam_sphere_draw(RoamSphere *sphere);
void roam_sphere_draw_normals(RoamSphere *sphere);
void roam_sphere_free(RoamSphere *sphere);

#endif
