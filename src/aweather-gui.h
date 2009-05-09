#ifndef __AWEATHER_GUI_H__
#define __AWEATHER_GUI_H__

#include <gtk/gtk.h>
#include <glib-object.h>
#include "aweather-view.h"

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
	GObject parent_instance;

	/* instance members */
	AWeatherView   *view;
	GtkBuilder     *builder;
	GtkWindow      *window;
	GtkNotebook    *tabs;
	GtkDrawingArea *drawing;
};

struct _AWeatherGuiClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType aweather_gui_get_type(void);

/* Methods */
AWeatherGui    *aweather_gui_new();
AWeatherView   *aweather_gui_get_view(AWeatherGui *gui);
GtkBuilder     *aweather_gui_get_builder(AWeatherGui *gui);
GtkWindow      *aweather_gui_get_window(AWeatherGui *gui);
GtkNotebook    *aweather_gui_get_tabs(AWeatherGui *gui);
GtkDrawingArea *aweather_gui_get_drawing(AWeatherGui *gui);

//void aweather_gui_register_plugin(AWeatherGui *gui, AWeatherPlugin *plugin);

#endif
