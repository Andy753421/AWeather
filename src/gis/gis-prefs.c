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
#include "gis-marshal.h"
#include "gis-prefs.h"


enum {
	SIG_PREF_CHANGED,
	NUM_SIGNALS,
};
static guint signals[NUM_SIGNALS];


/***********
 * Methods *
 ***********/
GisPrefs *gis_prefs_new(const gchar *prog)
{
	g_debug("GisPrefs: new - %s", prog);
	GisPrefs *self = g_object_new(GIS_TYPE_PREFS, NULL);
	self->key_path = g_build_filename(g_get_user_config_dir(),
			prog, "config.ini", NULL);
	GError *error = NULL;
	g_key_file_load_from_file(self->key_file, self->key_path,
			G_KEY_FILE_KEEP_COMMENTS, &error);
	if (error) {
		g_debug("GisPrefs: new - Trying %s defaults", prog);
		g_clear_error(&error);
		gchar *tmp = g_build_filename(DATADIR, prog, "defaults.ini", NULL);
		g_key_file_load_from_file(self->key_file, tmp,
				G_KEY_FILE_KEEP_COMMENTS, &error);
		g_free(tmp);
	}
	if (error) {
		g_debug("GisPrefs: new - Trying GIS defaults");
		g_clear_error(&error);
		gchar *tmp = g_build_filename(DATADIR, "gis", "defaults.ini", NULL);
		g_key_file_load_from_file(self->key_file, tmp,
				G_KEY_FILE_KEEP_COMMENTS, &error);
		g_free(tmp);
	}
	if (error) {
		g_clear_error(&error);
		g_warning("GisPrefs: new - Unable to load key file `%s': %s",
			self->key_path, error->message);
	}
	return self;
}

#define make_pref_type(name, c_type, g_type)                                         \
c_type gis_prefs_get_##name##_v(GisPrefs *self,                                      \
		const gchar *group, const gchar *key)                                \
{                                                                                    \
	GError *error = NULL;                                                        \
	c_type value = g_key_file_get_##name(self->key_file, group, key, &error);    \
	if (error && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)                \
		g_warning("GisPrefs: get_value_##name - error getting key %s: %s\n", \
				key, error->message);                                \
	return value;                                                                \
}                                                                                    \
c_type gis_prefs_get_##name(GisPrefs *self, const gchar *key)                        \
{                                                                                    \
	gchar **keys  = g_strsplit(key, "/", 2);                                     \
	c_type value = gis_prefs_get_##name##_v(self, keys[0], keys[1]);             \
	g_strfreev(keys);                                                            \
	return value;                                                                \
}                                                                                    \
                                                                                     \
void gis_prefs_set_##name##_v(GisPrefs *self,                                        \
		const gchar *group, const gchar *key, const c_type value)            \
{                                                                                    \
	g_key_file_set_##name(self->key_file, group, key, value);                    \
	gchar *all = g_strconcat(group, "/", key, NULL);                             \
	g_signal_emit(self, signals[SIG_PREF_CHANGED], 0,                            \
			all, g_type, &value);                                        \
	g_free(all);                                                                 \
}                                                                                    \
void gis_prefs_set_##name(GisPrefs *self, const gchar *key, const c_type value)      \
{                                                                                    \
	gchar **keys = g_strsplit(key, "/", 2);                                      \
	gis_prefs_set_##name##_v(self, keys[0], keys[1], value);                     \
	g_strfreev(keys);                                                            \
}                                                                                    \

make_pref_type(string,  gchar*,   G_TYPE_STRING)
make_pref_type(boolean, gboolean, G_TYPE_BOOLEAN)
make_pref_type(integer, gint,     G_TYPE_INT)
make_pref_type(double,  gdouble,  G_TYPE_DOUBLE)


/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisPrefs, gis_prefs, G_TYPE_OBJECT);
static void gis_prefs_init(GisPrefs *self)
{
	g_debug("GisPrefs: init");
	self->key_file = g_key_file_new();
}
static GObject *gis_prefs_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	g_debug("gis_prefs: constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(gis_prefs_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}
static void gis_prefs_dispose(GObject *_self)
{
	g_debug("GisPrefs: dispose");
	GisPrefs *self = GIS_PREFS(_self);
	if (self->key_file) {
		gsize length;
		gchar *dir = g_path_get_dirname(self->key_path);
		g_mkdir_with_parents(dir, 0755);
		gchar *data = g_key_file_to_data(self->key_file, &length, NULL);
		g_file_set_contents(self->key_path, data, length, NULL);
		g_key_file_free(self->key_file);
		g_free(self->key_path);
		g_free(dir);
		g_free(data);
		self->key_file = NULL;
	}
	G_OBJECT_CLASS(gis_prefs_parent_class)->dispose(_self);
}
static void gis_prefs_finalize(GObject *_self)
{
	g_debug("GisPrefs: finalize");
	G_OBJECT_CLASS(gis_prefs_parent_class)->finalize(_self);
}
static void gis_prefs_class_init(GisPrefsClass *klass)
{
	g_debug("GisPrefs: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = gis_prefs_constructor;
	gobject_class->dispose      = gis_prefs_dispose;
	gobject_class->finalize     = gis_prefs_finalize;
	signals[SIG_PREF_CHANGED] = g_signal_new(
			"pref-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			gis_cclosure_marshal_VOID__STRING_UINT_POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING,
			G_TYPE_UINT,
			G_TYPE_POINTER);
}
