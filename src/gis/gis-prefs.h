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

#ifndef __GIS_PREFS_H__
#define __GIS_PREFS_H__

#include <glib-object.h>

/* Type macros */
#define GIS_TYPE_PREFS            (gis_prefs_get_type())
#define GIS_PREFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GIS_TYPE_PREFS, GisPrefs))
#define GIS_IS_PREFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GIS_TYPE_PREFS))
#define GIS_PREFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GIS_TYPE_PREFS, GisPrefsClass))
#define GIS_IS_PREFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GIS_TYPE_PREFS))
#define GIS_PREFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GIS_TYPE_PREFS, GisPrefsClass))

typedef struct _GisPrefs      GisPrefs;
typedef struct _GisPrefsClass GisPrefsClass;

struct _GisPrefs {
	GObject parent_instance;

	/* instance members */
	gchar    *key_path;
	GKeyFile *key_file;
};

struct _GisPrefsClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType gis_prefs_get_type(void);

/* Methods */
GisPrefs *gis_prefs_new(const gchar *prog);

gchar    *gis_prefs_get_string   (GisPrefs *prefs, const gchar *key);
gboolean  gis_prefs_get_boolean  (GisPrefs *prefs, const gchar *key);
gint      gis_prefs_get_integer  (GisPrefs *prefs, const gchar *key);
gdouble   gis_prefs_get_double   (GisPrefs *prefs, const gchar *key);

gchar    *gis_prefs_get_string_v (GisPrefs *prefs, const gchar *group, const gchar *key);
gboolean  gis_prefs_get_boolean_v(GisPrefs *prefs, const gchar *group, const gchar *key);
gint      gis_prefs_get_integer_v(GisPrefs *prefs, const gchar *group, const gchar *key);
gdouble   gis_prefs_get_double_v (GisPrefs *prefs, const gchar *group, const gchar *key);

void      gis_prefs_set_string   (GisPrefs *prefs, const gchar *key, const gchar *string);
void      gis_prefs_set_boolean  (GisPrefs *prefs, const gchar *key, gboolean value);
void      gis_prefs_set_integer  (GisPrefs *prefs, const gchar *key, gint value);
void      gis_prefs_set_double   (GisPrefs *prefs, const gchar *key, gdouble value);

void      gis_prefs_set_string_v (GisPrefs *prefs, const gchar *group, const gchar *key, const gchar *string);
void      gis_prefs_set_boolean_v(GisPrefs *prefs, const gchar *group, const gchar *key, gboolean value);
void      gis_prefs_set_integer_v(GisPrefs *prefs, const gchar *group, const gchar *key, gint value);
void      gis_prefs_set_double_v (GisPrefs *prefs, const gchar *group, const gchar *key, gdouble value);
#endif
