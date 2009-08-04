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

#ifndef __LOCATION_H__
#define __LOCATION_H__

enum {
	LOCATION_CITY,
	LOCATION_STATE
};

typedef struct {
	int type;
	char *code;
	char *label;
	struct {
		gint north;
		gint ne;
		gint east;
		gint se;
		gint south;
		gint sw;
		gint west;
		gint nw;
	} neighbors;
} city_t;

extern city_t cities[];


#endif
