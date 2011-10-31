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

#ifndef __AWEATHER_GUI_H__
#define __AWEATHER_GUI_H__

#include <gtk/gtk.h>
#include <glib-object.h>

#include <grits.h>

/* Type macros */
#define AWEATHER_TYPE_GUI            (aweather_gui_get_type())
#define AWEATHER_GUI(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_GUI, AWeatherGui))
#define AWEATHER_IS_GUI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_GUI))
#define AWEATHER_GUI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_GUI, AWeatherGuiClass))
#define AWEATHER_IS_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_GUI))
#define AWEATHER_GUI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_GUI, AWeatherGuiClass))

typedef struct _AWeatherGui      AWeatherGui;
typedef struct _AWeatherGuiClass AWeatherGuiClass;

struct _AWeatherGui {
	GtkWindow parent_instance;

	/* instance members */
	GtkBuilder   *builder;
	GritsViewer  *viewer;
	GritsPlugins *plugins;
	GritsPrefs   *prefs;
	GtkListStore *gtk_plugins;
	guint         update_source;
};

struct _AWeatherGuiClass {
	GtkWindowClass parent_class;
	
	/* class members */
};

GType aweather_gui_get_type(void);

/* Methods */
AWeatherGui *aweather_gui_new();

GritsViewer *aweather_gui_get_viewer(AWeatherGui *gui);

GtkWidget   *aweather_gui_get_widget(AWeatherGui *gui, const gchar *name);
GObject     *aweather_gui_get_object(AWeatherGui *gui, const gchar *name);

void         aweather_gui_attach_plugin(AWeatherGui *self, const gchar *name);
void         aweather_gui_deattach_plugin(AWeatherGui *self, const gchar *name);

#endif
