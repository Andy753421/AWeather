#ifndef __AWEATHER_VIEW_H__
#define __AWEATHER_VIEW_H__

#include <glib-object.h>

/* Type macros */
#define AWEATHER_TYPE_VIEW            (aweather_view_get_type())
#define AWEATHER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   AWEATHER_TYPE_VIEW, AWeatherView))
#define AWEATHER_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   AWEATHER_TYPE_VIEW))
#define AWEATHER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), AWEATHER_TYPE_VIEW, AWeatherViewClass))
#define AWEATHER_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), AWEATHER_TYPE_VIEW))
#define AWEATHER_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   AWEATHER_TYPE_VIEW, AWeatherViewClass))

typedef struct _AWeatherView      AWeatherView;
typedef struct _AWeatherViewClass AWeatherViewClass;

struct _AWeatherView {
	GObject parent_instance;

	/* instance members */
	gint   time;
	gchar *location;
};

struct _AWeatherViewClass {
	GObjectClass parent_class;
	
	/* class members */
};

GType aweather_view_get_type(void);

/* Methods */
AWeatherView *aweather_view_new();
void aweather_view_set_time(AWeatherView *view, gint time);
gint aweather_view_get_time(AWeatherView *view);

void aweather_view_set_location(AWeatherView *view, const gchar *location);
gchar *aweather_view_get_location(AWeatherView *view);

#endif
