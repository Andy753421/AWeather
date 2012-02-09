/*
 * Copyright (C) 2012 Adam Boggs <boggs@aircrafter.org>
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
#ifndef _GPS_PLUGIN_H
#define _GPS_PLUGIN_H

#define GRITS_TYPE_PLUGIN_GPS            (grits_plugin_gps_get_type ())
#define GRITS_PLUGIN_GPS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GRITS_TYPE_PLUGIN_GPS, GritsPluginGps))
#define GRITS_IS_PLUGIN_GPS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GRITS_TYPE_PLUGIN_GPS))
#define GRITS_PLUGIN_GPS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GRITS_TYPE_PLUGIN_GPS, GritsPluginGpsClass))
#define GRITS_IS_PLUGIN_GPS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GRITS_TYPE_PLUGIN_GPS))
#define GRITS_PLUGIN_GPS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GRITS_TYPE_PLUGIN_GPS, GritsPluginGpsClass))

typedef struct _GritsPluginGps      GritsPluginGps;
typedef struct _GritsPluginGpsClass GritsPluginGpsClass;

/* All the User Interface objects we need to keep track of. */
typedef struct {
	/* gps info frame */
	GtkWidget *gps_status_frame;
	GtkWidget *gps_status_table;
	GtkWidget *gps_status_label;
	GtkWidget *gps_latitude_label;
	GtkWidget *gps_longitude_label;
	GtkWidget *gps_heading_label;
	GtkWidget *gps_elevation_label;

	GtkWidget *status_bar;

	/* control frame */
	GtkWidget *gps_follow_checkbox;
	GtkWidget *gps_track_checkbox;
	GtkWidget *gps_clear_button;

	/* log frame */
	GtkWidget *gps_log_checkbox;
	GtkWidget *gps_log_filename_entry;
	GtkWidget *gps_log_interval_slider;
	guint      gps_log_timeout_id; /* id of timeout so we can delete it */
	guint      gps_log_number;     /* sequential log number */
} GpsUi;

typedef struct {
	/* track storage */
	gboolean   active;  /* Display track history */
	gdouble (**points)[3];
	GritsLine *line;
	guint      cur_point;
	guint      num_points;
	guint      cur_group;
} GpsTrack;

/* GPS private data */
struct _GritsPluginGps {
	GObject parent_instance;

	/* instance members */
	GritsViewer *viewer;
	GritsPrefs  *prefs;
	GtkWidget   *config;
	guint        tab_id;
	GritsMarker *marker;

	struct gps_data_t gps_data;

	gboolean     follow_gps;
	guint        gps_update_timeout_id; /* id of timeout so we can delete it */

	GpsTrack     track;
	GpsUi        ui;
};

struct _GritsPluginGpsClass {
	GObjectClass parent_class;
};

GType grits_plugin_gps_get_type();


#endif /* GPS_PLUGIN_H */

