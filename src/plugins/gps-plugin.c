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

/* TODO:
 *    If gpsd connection fails, try to connect again periodically.
 *    If gps stops sending data there should be an indication that it's stale.
 */

#define _XOPEN_SOURCE
#include <config.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <math.h>

#include <grits.h>
#include <gps.h>

#include "gps-plugin.h"
#include "level2.h"
#include "../aweather-location.h"

#include "../compat.h"

/* interval to update map with new gps data in seconds. */
#define GPS_UPDATE_INTERVAL     (2)

/* Filename and search path to use for gps marker, should be configurable */
#define GPS_MARKER_ICON_PATH    ".:" PKGDATADIR
#define GPS_MARKER_ICON         "arrow.png"

/* number of track points per group and number of groups to maintain */
#define GPS_TRACK_POINTS        (256)
#define GPS_TRACK_GROUPS        (16)
#define GPS_TRACK_POINTS_FACTOR (1.5)

/* interval to update log file in seconds (default value for slider) */
#define GPS_LOG_DEFAULT_UPDATE_INTERVAL (30)
#define GPS_LOG_EXT             "csv"

/* For updating the status bar conveniently */
#define GPS_STATUSBAR_CONTEXT   "GPS"

#if 0
#define GPS_STATUS(gps, format, args...) \
	do { \
		gchar *buf = g_strdup_printf(format, ##args); \
		gtk_statusbar_push(GTK_STATUSBAR(gps->status_bar), \
		    gtk_statusbar_get_context_id( \
		      GTK_STATUSBAR(gps->status_bar), \
		      GPS_STATUSBAR_CONTEXT), \
		buf); \
	} while (0)
#endif

#define GPS_STATUS(gps, format, args...) \
	do { \
		gchar *buf = g_strdup_printf(format, ##args); \
		g_debug("STATUS: %s", buf); \
	} while (0)


/********************
 * Helper functions *
 ********************/

/* Find a readable file in a colon delimeted path */
static gchar *find_path(const gchar *path, const gchar *filename)
{
	gchar *end_ptr, *fullpath;

	end_ptr = (gchar *)path;

	/* find first : */
	while (*end_ptr != ':' && *end_ptr != '\0')
		end_ptr++;
	fullpath = g_strdup_printf("%.*s/%s", (int)(end_ptr-path), path,
	                       filename);
	g_debug("GritsPluginGps: find_path - searching %s", fullpath);
	if (access(fullpath, R_OK) == 0) {
		g_debug("GritsPluginGps: find_path - found %s", fullpath);
		return fullpath;	/* caller frees */
	}

	g_free(fullpath);
	/* recurse */
	if (*end_ptr == '\0') {
		return NULL;
	} else {
		return find_path(end_ptr + 1, filename);
	}
}


/********************
 * GPS Status Table *
 ********************/

static gchar *gps_get_status(struct gps_data_t *gps_data)
{
	gchar *status_color;
	gchar *status_text;

	switch (gps_data->fix.mode) {
	case MODE_NOT_SEEN:
		status_color = "red";
		status_text = "No Signal";
		break;
	case MODE_NO_FIX:
		status_color = "red";
		status_text = "No Fix";
		break;
	case MODE_2D:
		status_color = "yellow";
		status_text = "2D Mode";
		break;
	case MODE_3D:
		status_color = "green";
		status_text = "3D Mode";
		break;
	default:
		status_color = "black";
		status_text = "Unknown";
		break;
	}
	return g_strdup_printf("<span foreground=\"%s\">%s</span>",
				status_color, status_text);
}

#if 0
static gchar *gps_get_online(struct gps_data_t *gps_data)
{
	gchar *status_str;
	gchar *online_str;

	if (gps_data->online == -1.0) {
		online_str = "Offline";
	} else {
		online_str = "Online";
	}

	switch (gps_data->status) {
	case 0:
		status_str = "No Fix";
		break;
	case 1:
		status_str = "Fix Acquired";
		break;
	case 2:
		status_str = "DGPS Fix";
		break;
	default:
		status_str = "Unknown Status";
		break;
	}

	return g_strdup_printf("%lf,%s,%s", gps_data->online, online_str, status_str);
}
#endif

static gchar *gps_get_latitude(struct gps_data_t *gps_data)
{
	return g_strdup_printf("%10.4f", gps_data->fix.latitude);
}

static gchar *gps_get_longitude(struct gps_data_t *gps_data)
{
	return g_strdup_printf("%10.4f", gps_data->fix.longitude);
}

static gchar *gps_get_elevation(struct gps_data_t *gps_data)
{
	/* XXX Make units (m/ft) settable */
	return g_strdup_printf("%5.0lf %4s",
		    (gps_data->fix.altitude * METERS_TO_FEET), "ft");
}

static gchar *gps_get_heading(struct gps_data_t *gps_data)
{
	/* XXX Make units (m/ft) settable */
	return g_strdup_printf("%5.0lf %4s", gps_data->fix.track, "deg");
}

static gchar *gps_get_speed(struct gps_data_t *gps_data)
{
	/* XXX Make units (m/ft) settable */
	return g_strdup_printf("%5.0f %4s",
		(gps_data->fix.speed*3600.0*METERS_TO_FEET/5280.0), "mph");
}

struct {
	const gchar *label;
	const gchar *initial_val;
	gchar     *(*get_data)(struct gps_data_t *);
	guint        font_size;
	GtkWidget   *label_widget;
	GtkWidget   *value_widget;
} gps_table[] = {
	{"Status:",    "No Data", gps_get_status,    14, NULL, NULL},
//	{"Online:",    "No Data", gps_get_online,    14, NULL, NULL},
	{"Latitude:",  "No Data", gps_get_latitude,  14, NULL, NULL},
	{"Longitude:", "No Data", gps_get_longitude, 14, NULL, NULL},
	{"Elevation:", "No Data", gps_get_elevation, 14, NULL, NULL},
	{"Heading:",   "No Data", gps_get_heading,   14, NULL, NULL},
	{"Speed:",     "No Data", gps_get_speed,     14, NULL, NULL},
};


/******************
 * Track handling *
 ******************/

static void gps_track_init(GpsTrack *track)
{
	/* Save a spot at the end for the NULL termination */
	track->points = (gpointer)g_new0(double*, GPS_TRACK_GROUPS + 1);
	track->cur_point  = 0;
	track->cur_group  = 0;
	track->num_points = 1;	/* starts at 1 so realloc logic works */
	track->line = NULL;
}

static void gps_track_clear(GpsTrack *track)
{
	gint pi;
	for (pi = 0; pi < GPS_TRACK_GROUPS; pi++) {
		if (track->points[pi] != NULL) {
			g_free(track->points[pi]);
			track->points[pi] = NULL;
		}
	}
	track->cur_point  = 0;
	track->cur_group  = 0;
	track->num_points = 1;	/* starts at 1 so realloc logic works */
}

static void gps_track_free(GpsTrack *track)
{
	gps_track_clear(track);
	g_free(track->points);
}

/* add a new track group (points in a track group are connected, and
 * separated from points in other track groups).  */
static void gps_track_group_incr(GpsTrack *track)
{
	gdouble (**points)[3] = track->points; /* for simplicity */

	/* Just return if they increment it again before any points have
	 * been added.
	 */
	if (points[track->cur_group] == NULL) {
		return;
	}

	g_debug("GritsPluginGps: track_group_incr - track group %u->%u.",
		track->cur_group, track->cur_group + 1);

	track->cur_group++;
	track->cur_point  = 0;
	track->num_points = 1;	/* starts at 1 so realloc logic works */

	if (track->cur_group >= GPS_TRACK_GROUPS) {
		g_debug("GritsPluginGps: track_group_incr - track group %u "
		        "is at max %u, shifting groups",
		        track->cur_group, GPS_TRACK_GROUPS);

		/* Free the oldest one which falls off the end */
		g_free(points[0]);

		/* shift the rest down, last one should be NULL already */
		/* note we alloc GPS_TRACK_GROUPS+1 */
		for (int pi = 0; pi < GPS_TRACK_GROUPS; pi++) {
			points[pi] = points[pi+1];
		}

		/* always write into the last group */
		track->cur_group = GPS_TRACK_GROUPS - 1;
	}
}

static void gps_track_add_point(GpsTrack *track,
    gdouble lat, gdouble lon, gdouble elevation)
{
	gdouble (**points)[3] = track->points; /* for simplicity */

	g_debug("GritsPluginGps: track_add_point");

	g_assert(track->cur_group < GPS_TRACK_GROUPS &&
	        (track->cur_point <= track->num_points));

	/* resize/allocate the point group if the current one is full */
	if (track->cur_point >= track->num_points - 1) {
		guint new_size = track->num_points == 1 ?
			    GPS_TRACK_POINTS :
			    track->num_points * GPS_TRACK_POINTS_FACTOR;
		g_debug("GritsPluginGps: track_add_point - reallocating points "
			"array from %u points to %u points.\n",
			track->num_points, new_size);
		points[track->cur_group] = (gpointer)g_renew(gdouble,
			    points[track->cur_group], 3*(new_size+1));
		track->num_points = new_size;
	}

	g_assert(points[track->cur_group] != NULL);

	/* Add the coordinate */
	lle2xyz(lat, lon, elevation,
	    &points[track->cur_group][track->cur_point][0],
	    &points[track->cur_group][track->cur_point][1],
	    &points[track->cur_group][track->cur_point][2]);

	track->cur_point++;

	/* make sure last point is always 0s so the line drawing stops. */
	points[track->cur_group][track->cur_point][0] = 0.0;
	points[track->cur_group][track->cur_point][1] = 0.0;
	points[track->cur_group][track->cur_point][2] = 0.0;
}


/*****************
 * Track Logging *
 *****************/

static gchar *gps_get_date_string(double gps_time)
{
	static gchar	buf[256];
	time_t int_time = (time_t)gps_time;
	struct tm	tm_time;

	gmtime_r(&int_time, &tm_time);

	snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
	         tm_time.tm_year+1900, tm_time.tm_mon+1, tm_time.tm_mday);

	return buf;
}

static gchar *gps_get_time_string(time_t gps_time)
{
	static gchar buf[256];
	time_t int_time = (time_t)gps_time;
	struct tm tm_time;

	gmtime_r(&int_time, &tm_time);

	snprintf(buf, sizeof(buf), "%02d:%02d:%02dZ",
	         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

	return buf;
}

static gboolean gps_write_log(gpointer data)
{
	GritsPluginGps *gps = (GritsPluginGps *)data;
	struct gps_data_t *gps_data = &gps->gps_data;
	gchar buf[256];
	gchar filename[256];
	gint fd;
	gboolean new_file = FALSE;

	if (gps_data == NULL) {
		g_warning("Skipped write to GPS log file: "
		          "can not get GPS coordinates.");
		GPS_STATUS(gps, "Skipped write to GPS log file: "
		          "can not get GPS coordinates.");
		return TRUE;
	}

	/* get filename from text entry box.  If empty, generate a name from
	 * the date and time and set it. */
	if (strlen(gtk_entry_get_text(
	              GTK_ENTRY(gps->ui.gps_log_filename_entry))) == 0) {
		snprintf(filename, sizeof(filename),
			    "%sT%s.%s",
			    gps_get_date_string(gps->gps_data.fix.time),
			    gps_get_time_string(gps->gps_data.fix.time),
			    GPS_LOG_EXT);
		gtk_entry_set_text(GTK_ENTRY(gps->ui.gps_log_filename_entry),
			    filename);
	}

	strncpy(filename,
	    gtk_entry_get_text(GTK_ENTRY(gps->ui.gps_log_filename_entry)),
	    sizeof(filename));

	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
		new_file = TRUE;
	}

	if ((fd = open(filename, O_CREAT|O_APPEND|O_WRONLY, 0644)) == -1) {
		g_warning("Error opening log file %s: %s",
				filename, strerror(errno));
		return FALSE;
	}

	if (new_file) {
		/* write header and reset record counter */
		snprintf(buf, sizeof(buf),
			"No,Date,Time,Lat,Lon,Ele,Head,Speed,RTR\n");
		if (write(fd, buf, strlen(buf)) == -1) {
		    g_warning("Error writing header to log file %s: %s",
				    filename, strerror(errno));
		}
		gps->ui.gps_log_number = 1;
	}

	/* Write log entry.  Make sure this matches the header */
	/* "No,Date,Time,Lat,Lon,Ele,Head,Speed,Fix,RTR\n" */
	/* RTR values: T=time, B=button push, S=speed, D=distance */
	snprintf(buf, sizeof(buf), "%d,%s,%s,%lf,%lf,%lf,%lf,%lf,%c\n",
			gps->ui.gps_log_number++,
			gps_get_date_string(gps->gps_data.fix.time),
			gps_get_time_string(gps->gps_data.fix.time),
			//gps_data->fix.time,
			gps_data->fix.latitude,
			gps_data->fix.longitude,
			gps_data->fix.altitude * METERS_TO_FEET,
			gps_data->fix.track,
			gps_data->fix.speed * METERS_TO_FEET,
			'T'); /* position due to timer expired  */

	if (write(fd, buf, strlen(buf)) == -1) {
		g_warning("Could not write log number %d to log file %s: %s",
				gps->ui.gps_log_number-1, filename, strerror(errno));
	}
	close(fd);

	GPS_STATUS(gps, "Updated GPS log file %s.", filename);

	/* reschedule */
	return TRUE;
}


/****************
 * Main drawing *
 ****************/

static gboolean gps_data_is_valid(struct gps_data_t *gps_data)
{
	if (gps_data != NULL && gps_data->online != -1.0 &&
		gps_data->fix.mode >= MODE_2D &&
		gps_data->status > STATUS_NO_FIX) {
		return TRUE;
	}

	return FALSE;
}

static void gps_update_status(GritsPluginGps *gps)
{
	struct gps_data_t *gps_data = &gps->gps_data;

	/* gps table update */
	for (gint i = 0; i < G_N_ELEMENTS(gps_table); i++) {
		gchar *str = gps_table[i].get_data(gps_data);
		gtk_label_set_markup(GTK_LABEL(gps_table[i].value_widget), str);
		g_free(str);
	}
}

/* external interface to update UI from latest GPS data. */
gboolean gps_redraw_all(gpointer data)
{
	GritsPluginGps *gps = (GritsPluginGps *)data;
	g_assert(gps);

	struct gps_data_t *gps_data = &gps->gps_data;

	g_debug("GritsPluginGps: redraw_all");

	g_assert(gps_data);
	if (!gps_data_is_valid(gps_data)) {
		g_debug("GritsPluginGps: redraw_all - gps_data is not valid.");
		/* XXX Change marker to indicate data is not valid */
		return TRUE;
	}

	/* update position labels */
	gps_update_status(gps);

	/* Update track and marker position */
	if (gps_data_is_valid(gps_data) && gps->track.active) {
		g_debug("GritsPluginGps: redraw_all - updating track group %u "
		        "point %u at lat = %f, long = %f, track = %f",
		        gps->track.cur_group,
		        gps->track.cur_point,
		        gps_data->fix.latitude,
		        gps_data->fix.longitude,
		        gps_data->fix.track);

		gps_track_add_point(&gps->track,
			  gps_data->fix.latitude, gps_data->fix.longitude, 0.0);

		grits_object_destroy_pointer(&gps->track.line);

		gps->track.line = grits_line_new(gps->track.points);
		gps->track.line->color[0]  = 1.0;
		gps->track.line->color[1]  = 0;
		gps->track.line->color[2]  = 0.1;
		gps->track.line->color[3]  = 1.0;
		gps->track.line->width     = 3;

		grits_viewer_add(gps->viewer, GRITS_OBJECT(gps->track.line),
			    GRITS_LEVEL_OVERLAY, FALSE);
		grits_object_queue_draw(GRITS_OBJECT(gps->track.line));
	}

	if (gps_data_is_valid(gps_data)) {
		grits_object_destroy_pointer(&gps->marker);

		gchar *path = find_path(GPS_MARKER_ICON_PATH, GPS_MARKER_ICON);
		if (path) {
			gps->marker = grits_marker_icon_new("GPS", path,
				      gps_data->fix.track, TRUE, GRITS_MARKER_DMASK_ICON);
			g_free(path);
		} else {
			/* if icon not found just use a point */
			g_warning("Could not find GPS marker icon %s in path %s.",
				  GPS_MARKER_ICON, GPS_MARKER_ICON_PATH);
			gps->marker = grits_marker_icon_new("GPS", NULL,
				      gps_data->fix.track, FALSE,
				      GRITS_MARKER_DMASK_POINT |
				      GRITS_MARKER_DMASK_LABEL);
		}

		GRITS_OBJECT(gps->marker)->center.lat  = gps_data->fix.latitude;
		GRITS_OBJECT(gps->marker)->center.lon  = gps_data->fix.longitude;
		GRITS_OBJECT(gps->marker)->center.elev = 0.0;
		GRITS_OBJECT(gps->marker)->lod         = EARTH_R * 5;
		GRITS_MARKER(gps->marker)->ortho       = FALSE;

		grits_viewer_add(gps->viewer, GRITS_OBJECT(gps->marker),
		        GRITS_LEVEL_OVERLAY+1, FALSE);
		grits_object_queue_draw(GRITS_OBJECT(gps->marker));
	}

	if (gps->follow_gps && gps_data_is_valid(gps_data)) {
		/* Center map at current GPS position. */
		g_debug("GritsPluginGps: redraw_all - centering map at "
		        "lat = %f, long = %f, track = %f",
		            gps_data->fix.latitude,
		            gps_data->fix.longitude,
		            gps_data->fix.track);

		double lat, lon, elev;
		grits_viewer_get_location(gps->viewer, &lat, &lon, &elev);
		grits_viewer_set_location(gps->viewer, gps_data->fix.latitude,
					  gps_data->fix.longitude, elev);
		//grits_viewer_set_rotation(gps->viewer, 0, 0, 0);
	}

	/* reschedule */
	return TRUE;
}


/***************
 * Config Area *
 ***************/

/* GPS Data Frame */
static void gps_init_status_info(GritsPluginGps *gps, GtkWidget *gbox)
{
	gps->ui.gps_status_frame = gtk_frame_new("<b>GPS Data</b>");
	GtkWidget *label = gtk_frame_get_label_widget(
	                   GTK_FRAME(gps->ui.gps_status_frame));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(gps->ui.gps_status_frame),
	                   GTK_SHADOW_NONE);
	gps->ui.gps_status_table = gtk_table_new(5, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(gps->ui.gps_status_frame),
	           gps->ui.gps_status_table);


	/* gps data table setup */
	gint i;
	for (i = 0; i < G_N_ELEMENTS(gps_table); i++) {
		gps_table[i].label_widget = gtk_label_new(gps_table[i].label);
		gps_table[i].value_widget = gtk_label_new(gps_table[i].initial_val);

		gtk_misc_set_alignment(GTK_MISC(gps_table[i].label_widget), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(gps_table[i].value_widget), 1, 0);

		gtk_table_attach(GTK_TABLE(gps->ui.gps_status_table),
				gps_table[i].label_widget, 0, 1, i, i+1,
				GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
		gtk_table_attach(GTK_TABLE(gps->ui.gps_status_table),
				gps_table[i].value_widget, 1, 2, i, i+1,
				GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);

		PangoFontDescription *roman = pango_font_description_new();
		PangoFontDescription *mono  = pango_font_description_from_string("monospace");
		pango_font_description_set_size(roman, gps_table[i].font_size*PANGO_SCALE);
		pango_font_description_set_size(mono,  gps_table[i].font_size*PANGO_SCALE);
		gtk_widget_modify_font(gps_table[i].label_widget, roman);
		gtk_widget_modify_font(gps_table[i].value_widget, mono);
		pango_font_description_free(roman);
		pango_font_description_free(mono);
	}
	gtk_box_pack_start(GTK_BOX(gbox), gps->ui.gps_status_frame,
	                    FALSE, FALSE, 0);

	/* Start UI refresh task, which will reschedule itgps. */
	gps_redraw_all(gps);
	gps->gps_update_timeout_id = g_timeout_add(
		    GPS_UPDATE_INTERVAL*1000,
		    gps_redraw_all, gps);

}

/* GPS Control Frame */
static gboolean on_gps_follow_clicked_event(GtkWidget *widget, gpointer user_data)
{
	GritsPluginGps *gps = (GritsPluginGps *)user_data;

	g_debug("GritsPluginGps: follow_clicked_event - button status %d",
	        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		gps->follow_gps = TRUE;
	} else {
		gps->follow_gps = FALSE;
	}

	return FALSE;
}

static gboolean on_gps_track_enable_clicked_event(GtkWidget *widget, gpointer user_data)
{
	GritsPluginGps *gps = (GritsPluginGps *)user_data;

	g_debug("GritsPluginGps: track_enable_clicked_event");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		/* start logging trip history */
		GPS_STATUS(gps, "Enabled GPS track.");
		gps->track.active = TRUE;
	} else {
		/* stop logging trip history */
		GPS_STATUS(gps, "Disabled GPS track.");
		gps->track.active = FALSE;
		/* advance to the next track group, moving everything down if
		 * it's full. */
		gps_track_group_incr(&gps->track);
	}

	return FALSE;
}

static gboolean on_gps_track_clear_clicked_event(GtkWidget *widget, gpointer user_data)
{
	GritsPluginGps *gps = (GritsPluginGps *)user_data;

	g_debug("GritsPluginGps: track_clear_clicked_event");
	GPS_STATUS(gps, "Cleared GPS track.");
	gps_track_clear(&gps->track);

	return FALSE;
}

static void gps_init_control_frame(GritsPluginGps *gps, GtkWidget *gbox)
{
	/* Control checkboxes */
	GtkWidget *gps_control_frame = gtk_frame_new("<b>GPS Control</b>");
	GtkWidget *label = gtk_frame_get_label_widget(
	                   GTK_FRAME(gps_control_frame));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(gps_control_frame),
	                   GTK_SHADOW_NONE);
	GtkWidget *cbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_container_add(GTK_CONTAINER(gps_control_frame), cbox);
	gtk_box_pack_start(GTK_BOX(gbox), gps_control_frame, FALSE, FALSE, 0);

	gps->ui.gps_follow_checkbox =
	              gtk_check_button_new_with_label("Follow GPS");
	g_signal_connect(G_OBJECT(gps->ui.gps_follow_checkbox), "clicked",
	              G_CALLBACK(on_gps_follow_clicked_event),
	              (gpointer)gps);
	gtk_box_pack_start(GTK_BOX(cbox), gps->ui.gps_follow_checkbox,
	               FALSE, FALSE, 0);

	gps->ui.gps_track_checkbox =
	               gtk_check_button_new_with_label("Record Track");
	g_signal_connect(G_OBJECT(gps->ui.gps_track_checkbox), "clicked",
	               G_CALLBACK(on_gps_track_enable_clicked_event),
	               (gpointer)gps);
	gtk_box_pack_start(GTK_BOX(cbox), gps->ui.gps_track_checkbox,
	               FALSE, FALSE, 0);

	gps->ui.gps_clear_button = gtk_button_new_with_label("Clear Track");
	g_signal_connect(G_OBJECT(gps->ui.gps_clear_button), "clicked",
	              G_CALLBACK(on_gps_track_clear_clicked_event),
	              (gpointer)gps);
	gtk_box_pack_start(GTK_BOX(cbox), gps->ui.gps_clear_button,
	               FALSE, FALSE, 0);
}

/* Track Log Frame */
static gboolean on_gps_log_clicked_event(GtkWidget *widget, gpointer user_data)
{
	GritsPluginGps *gps = (GritsPluginGps *)user_data;

	g_debug("GritsPluginGps: log_clicked_event");

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)))  {
		gps_write_log(gps);

		/* Schedule log file write */
		gps->ui.gps_log_timeout_id = g_timeout_add(
		      gtk_range_get_value(
		        GTK_RANGE(gps->ui.gps_log_interval_slider))*1000,
		        gps_write_log, gps);
	} else {
		/* button unchecked */
		g_source_remove(gps->ui.gps_log_timeout_id);
		gps->ui.gps_log_timeout_id = 0;
		g_debug("GritsPluginGps: log_clicked_event - closed log file.");
	}

	return FALSE;
}

static gboolean on_gps_log_interval_changed_event(GtkWidget *widget, gpointer user_data)
{
	GritsPluginGps *gps = (GritsPluginGps *)user_data;

	g_assert(gps);

	g_debug("GritsPluginGps: log_interval_changed_event - value = %f",
	gtk_range_get_value(GTK_RANGE(widget)));

	if (gtk_toggle_button_get_active(
	                GTK_TOGGLE_BUTTON(gps->ui.gps_log_checkbox))) {
		g_assert(gps->ui.gps_log_timeout_id != 0);

		/* disable old timeout */
		g_source_remove(gps->ui.gps_log_timeout_id);
		gps->ui.gps_log_timeout_id = 0;

		/* Schedule new log file write */
		gps->ui.gps_log_timeout_id = g_timeout_add(
		         gtk_range_get_value(GTK_RANGE(widget))*1000,
		         gps_write_log, gps);
		gps_write_log(gps);
	}

	return FALSE;
}

static void gps_init_track_log_frame(GritsPluginGps *gps, GtkWidget *gbox)
{
	/* Track log box with enable checkbox and filename entry */
	GtkWidget *gps_log_frame = gtk_frame_new("<b>Track Log</b>");
	GtkWidget *label = gtk_frame_get_label_widget(GTK_FRAME(gps_log_frame));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(gps_log_frame), GTK_SHADOW_NONE);
	GtkWidget *lbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_container_add(GTK_CONTAINER(gps_log_frame), lbox);
	gtk_box_pack_start(GTK_BOX(gbox), gps_log_frame,
			FALSE, FALSE, 0);

	gps->ui.gps_log_checkbox =
	       gtk_check_button_new_with_label("Log Position to File");
	g_signal_connect(G_OBJECT(gps->ui.gps_log_checkbox), "clicked",
	       G_CALLBACK(on_gps_log_clicked_event),
	       (gpointer)gps);
	gtk_box_pack_start(GTK_BOX(lbox), gps->ui.gps_log_checkbox,
	       FALSE, FALSE, 0);

	/* Set up filename entry box */
	GtkWidget *fbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *filename_label = gtk_label_new("Filename:");
	gtk_box_pack_start(GTK_BOX(fbox), filename_label, FALSE, FALSE, 0);
	gps->ui.gps_log_filename_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(fbox), gps->ui.gps_log_filename_entry,
	       TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lbox), fbox, FALSE, FALSE, 0);

	/* set up gps log interval slider */
	GtkWidget *ubox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	GtkWidget *interval_label = gtk_label_new("Update Interval:");
	gtk_box_pack_start(GTK_BOX(ubox), interval_label, FALSE, FALSE, 0);
	gps->ui.gps_log_interval_slider =
	            gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 600.0, 30.0);
	gtk_range_set_value(GTK_RANGE(gps->ui.gps_log_interval_slider),
	            GPS_LOG_DEFAULT_UPDATE_INTERVAL);
	g_signal_connect(G_OBJECT(gps->ui.gps_log_interval_slider),
	            "value-changed",
	            G_CALLBACK(on_gps_log_interval_changed_event),
	            (gpointer)gps);
	gtk_range_set_increments(
	            GTK_RANGE(gps->ui.gps_log_interval_slider),
	            10.0 /* step */, 30.0 /* page up/down */);
#if ! GTK_CHECK_VERSION(3,0,0)
	gtk_range_set_update_policy(
	            GTK_RANGE(gps->ui.gps_log_interval_slider),
	            GTK_UPDATE_DELAYED);
#endif
	gtk_box_pack_start(GTK_BOX(ubox), gps->ui.gps_log_interval_slider,
	            TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lbox), ubox, FALSE, FALSE, 0);
}


/*******************
 * GPSD interfaces *
 *******************/

static gboolean process_gps(GIOChannel *source, GIOCondition condition, gpointer data)
{
	struct gps_data_t *gps_data = (struct gps_data_t *)data;

	g_debug("GritsPluginGps: process_gps");

	/* Process any data from the gps and call the hook function */
	if (gps_data != NULL) {
		gdouble track = gps_data->fix.track;
		gint result = gps_read(gps_data);
		if (isnan(gps_data->fix.track))
			gps_data->fix.track = track;
		g_debug("GritsPluginGps: process_gps - gps_read returned %d, "
			"position %f, %f.", result,
			gps_data->fix.latitude, gps_data->fix.longitude);
	} else {
		g_warning("GritsPluginGps: process_gps - gps_data == NULL.");
	}
	return TRUE;
}

static gint initialize_gpsd(char *server, gchar *port, struct gps_data_t *gps_data)
{
#if GPSD_API_MAJOR_VERSION < 5
#error "GPSD protocol version 5 or greater required."
#endif
	gint result = gps_open(server, port, gps_data);
	if (result > 0) {
		g_warning("Unable to open gpsd connection to %s:%s: %d, %d, %s",
				server, port, result, errno, gps_errstr(errno));
		return 0;
	}

	(void)gps_stream(gps_data, WATCH_ENABLE|WATCH_JSON, NULL);
	g_debug("GritsPluginGps: initialize_gpsd - gpsd fd %u.", gps_data->gps_fd);
	GIOChannel *channel = g_io_channel_unix_new(gps_data->gps_fd);
	gint input_tag = g_io_add_watch(channel, G_IO_IN, process_gps, gps_data);
	g_io_channel_unref(channel);
	return input_tag;
}


/**********************
 * GPS Plugin Methods *
 **********************/

/* Methods */
GritsPluginGps *grits_plugin_gps_new(GritsViewer *viewer, GritsPrefs *prefs)
{
	/* TODO: move to constructor if possible */
	g_debug("GritsPluginGps: new");
	GritsPluginGps *gps = g_object_new(GRITS_TYPE_PLUGIN_GPS, NULL);
	gps->viewer  = g_object_ref(viewer);
	gps->prefs   = g_object_ref(prefs);

	gps->input_tag = initialize_gpsd("localhost", DEFAULT_GPSD_PORT, &gps->gps_data);
	gps->follow_gps = FALSE;

	gps_track_init(&gps->track);
	gps_init_status_info(gps, gps->config);
	gps_init_control_frame(gps, gps->config);
	gps_init_track_log_frame(gps, gps->config);

	return gps;
}

static GtkWidget *grits_plugin_gps_get_config(GritsPlugin *_gps)
{
	GritsPluginGps *gps = GRITS_PLUGIN_GPS(_gps);
	return gps->config;
}

/* GObject code */
static void grits_plugin_gps_plugin_init(GritsPluginInterface *iface);
G_DEFINE_TYPE_WITH_CODE(GritsPluginGps, grits_plugin_gps, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(GRITS_TYPE_PLUGIN,
			grits_plugin_gps_plugin_init));

static void grits_plugin_gps_plugin_init(GritsPluginInterface *iface)
{
	g_debug("GritsPluginGps: plugin_init");
	/* Add methods to the interface */
	iface->get_config = grits_plugin_gps_get_config;
}

static void grits_plugin_gps_init(GritsPluginGps *gps)
{
	g_debug("GritsPluginGps: gps_init");

	gps->config = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
}

static void grits_plugin_gps_dispose(GObject *gobject)
{
	GritsPluginGps *gps = GRITS_PLUGIN_GPS(gobject);

	g_debug("GritsPluginGps: dispose");

	if (gps->gps_update_timeout_id) {
		g_source_remove(gps->gps_update_timeout_id);
		gps->gps_update_timeout_id = 0;
	}
	if (gps->ui.gps_log_timeout_id) {
		g_source_remove(gps->ui.gps_log_timeout_id);
		gps->ui.gps_log_timeout_id = 0;
	}
	if (gps->input_tag) {
		g_source_remove(gps->input_tag);
		gps->input_tag = 0;
	}
	if (gps->viewer) {
		GritsViewer *viewer = gps->viewer;
		gps->viewer = NULL;
		grits_object_destroy_pointer(&gps->marker);
		g_object_unref(gps->prefs);
		g_object_unref(viewer);
	}

	/* Drop references */
	G_OBJECT_CLASS(grits_plugin_gps_parent_class)->dispose(gobject);
}

static void grits_plugin_gps_finalize(GObject *gobject)
{
	GritsPluginGps *gps = GRITS_PLUGIN_GPS(gobject);

	g_debug("GritsPluginGps: finalize");

	/* Free data */
	gps_track_free(&gps->track);
	G_OBJECT_CLASS(grits_plugin_gps_parent_class)->finalize(gobject);
}

static void grits_plugin_gps_class_init(GritsPluginGpsClass *klass)
{
	g_debug("GritsPluginGps: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = grits_plugin_gps_dispose;
	gobject_class->finalize = grits_plugin_gps_finalize;
}
