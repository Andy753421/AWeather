#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

#include "aweather-gui.h"
#include "plugin-radar.h"
#include "plugin-ridge.h"
#include "plugin-example.h"

/********
 * Main *
 ********/
int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	gtk_gl_init(&argc, &argv);

	/* Set up AWeather */
	AWeatherGui  *gui  = aweather_gui_new();
	//AWeatherView *view = aweather_gui_get_view(gui);

	/* Load plugins */
	//example_init(gui);
        ridge_init  (gui);
	radar_init  (gui);

	gtk_widget_show_all(aweather_gui_get_widget(gui, "window"));
	gtk_main();

	return 0;
}
