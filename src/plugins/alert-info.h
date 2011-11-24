/*
 * Copyright (C) 2010-2011 Andy Spencer <andy753421@gmail.com>
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

#ifndef __ALERT_INFO_H__
#define __ALERT_INFO_H__

#include <glib.h>

typedef struct {
	char    *title;    // Title, "Tornado Warning"
	char    *category; // Category, warning/watch/etc
	char    *abbr;     // Abbreviation, for button label
	int      prior;    // Priority, for county color picking
	gboolean hidden;   // Show this alert type?
	gboolean current;  // Are the currently alerts for this type?
	gboolean ispoly;   // May contain a polygon warning area
	guint8   color[3]; // Color to use for drawing alert
} AlertInfo;

extern AlertInfo alert_info[];

AlertInfo *alert_info_find(gchar *title);

#endif
