/*
 * Copyright (C) 2009-2011 Andy Spencer <andy753421@gmail.com>
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

#ifndef __LOCATION_H__
#define __LOCATION_H__

#include "grits-util.h"

enum {
	LOCATION_END,   // Special marker for end-of-list
	LOCATION_CITY,  // Cities (actually radars)
	LOCATION_STATE, // States (for indexing)
	LOCATION_NOP,   // Missing from IDD for some reason
};

typedef struct {
	gint type;
	gchar *code;
	gchar *name;
	GritsPoint pos;
	gdouble lod;
} city_t;

extern city_t cities[];


#endif
