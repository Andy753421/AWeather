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

#ifndef __ALERT_H__
#define __ALERT_H__

#include <glib-object.h>
#include <grits.h>

#define GRITS_TYPE_PLUGIN_ALERT            (grits_plugin_alert_get_type ())
#define GRITS_PLUGIN_ALERT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GRITS_TYPE_PLUGIN_ALERT, GritsPluginAlert))
#define GRITS_IS_PLUGIN_ALERT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GRITS_TYPE_PLUGIN_ALERT))
#define GRITS_PLUGIN_ALERT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GRITS_TYPE_PLUGIN_ALERT, GritsPluginAlertClass))
#define GRITS_IS_PLUGIN_ALERT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GRITS_TYPE_PLUGIN_ALERT))
#define GRITS_PLUGIN_ALERT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GRITS_TYPE_PLUGIN_ALERT, GritsPluginAlertClass))

typedef struct _GritsPluginAlert      GritsPluginAlert;
typedef struct _GritsPluginAlertClass GritsPluginAlertClass;

struct _GritsPluginAlert {
	GObject parent_instance;

	/* instance members */
	GritsViewer *viewer;
	GritsPrefs  *prefs;
	GtkWidget   *config;
	GtkWidget   *details;

	GritsHttp   *http;
	guint        refresh_id;
	guint        time_changed_id;
	guint        update_source;
	GThreadPool *threads;
	gboolean     aborted;

	GList       *msgs;
	time_t       updated;
	GTree       *counties;
	GList       *states;
};

struct _GritsPluginAlertClass {
	GObjectClass parent_class;
};

GType grits_plugin_alert_get_type();

/* Methods */
GritsPluginAlert *grits_plugin_alert_new(GritsViewer *viewer, GritsPrefs *prefs);

#endif

