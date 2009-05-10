#include <glib.h>

#include "aweather-view.h"

G_DEFINE_TYPE(AWeatherView, aweather_view, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_TIME,
	PROP_LOCATION,
};

enum {
	SIG_TIME_CHANGED,
	SIG_LOCATION_CHANGED,
	NUM_SIGNALS,
};

static guint signals[NUM_SIGNALS];

/* Constructor / destructors */
static void aweather_view_init(AWeatherView *self)
{
	//g_message("aweather_view_init");
	/* Default values */
	self->time     = g_strdup(""); //g_strdup("LATEST");
	self->location = g_strdup(""); //g_strdup("IND");
}

static GObject *aweather_view_constructor(GType gtype, guint n_properties,
		GObjectConstructParam *properties)
{
	//g_message("aweather_view_constructor");
	GObjectClass *parent_class = G_OBJECT_CLASS(aweather_view_parent_class);
	return  parent_class->constructor(gtype, n_properties, properties);
}

static void aweather_view_dispose (GObject *gobject)
{
	//g_message("aweather_view_dispose");
	/* Drop references to other GObjects */
	G_OBJECT_CLASS(aweather_view_parent_class)->dispose(gobject);
}

static void aweather_view_finalize(GObject *gobject)
{
	//g_message("aweather_view_finalize");
	AWeatherView *self = AWEATHER_VIEW(gobject);
	g_free(self->location);
	G_OBJECT_CLASS(aweather_view_parent_class)->finalize(gobject);
}

static void aweather_view_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec)
{
	//g_message("aweather_view_set_property");
	AWeatherView *self = AWEATHER_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     aweather_view_set_time    (self, g_value_get_string(value)); break;
	case PROP_LOCATION: aweather_view_set_location(self, g_value_get_string(value)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void aweather_view_get_property(GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	//g_message("aweather_view_get_property");
	AWeatherView *self = AWEATHER_VIEW(object);
	switch (property_id) {
	case PROP_TIME:     g_value_set_string(value, aweather_view_get_time    (self)); break;
	case PROP_LOCATION: g_value_set_string(value, aweather_view_get_location(self)); break;
	default:            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void aweather_view_class_init(AWeatherViewClass *klass)
{
	//g_message("aweather_view_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor  = aweather_view_constructor;
	gobject_class->dispose      = aweather_view_dispose;
	gobject_class->finalize     = aweather_view_finalize;
	gobject_class->get_property = aweather_view_get_property;
	gobject_class->set_property = aweather_view_set_property;
	g_object_class_install_property(gobject_class, PROP_TIME,
		g_param_spec_pointer(
			"time",
			"time of the current frame",
			"(format unknown)", 
			G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_LOCATION,
		g_param_spec_pointer(
			"location",
			"location seen by the viewport",
			"Location of the viewport. Currently this is the name of the radar site.", 
			G_PARAM_READWRITE));
	signals[SIG_TIME_CHANGED] = g_signal_new(
			"time-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING);
	signals[SIG_LOCATION_CHANGED] = g_signal_new(
			"location-changed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING);

}

/* Methods */
AWeatherView *aweather_view_new()
{
	//g_message("aweather_view_new");
	return g_object_new(AWEATHER_TYPE_VIEW, NULL);
}

void aweather_view_set_time(AWeatherView *view, const char *time)
{
	g_assert(AWEATHER_IS_VIEW(view));
	//g_message("aweather_view:set_time: setting time to %s", time);
	view->time = g_strdup(time);
	g_signal_emit(view, signals[SIG_TIME_CHANGED], 0, time);
}

gchar *aweather_view_get_time(AWeatherView *view)
{
	g_assert(AWEATHER_IS_VIEW(view));
	//g_message("aweather_view_get_time");
	return view->time;
}

void aweather_view_set_location(AWeatherView *view, const gchar *location)
{
	g_assert(AWEATHER_IS_VIEW(view));
	//g_message("aweather_view_set_location");
	g_free(view->location);
	view->location = g_strdup(location);
	g_signal_emit(view, signals[SIG_LOCATION_CHANGED], 0, view->location);
}

gchar *aweather_view_get_location(AWeatherView *view)
{
	g_assert(AWEATHER_IS_VIEW(view));
	//g_message("aweather_view_get_location");
	return view->location;
}
