AM_CFLAGS   = -Wall --std=gnu99  -I../
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(SOUP_CFLAGS)
AM_LDFLAGS  = -module -avoid-version
LIBS        = $(top_srcdir)/src/gis/libgis.la

plugindir = $(libdir)/gis

plugin_LTLIBRARIES =

if HAVE_RSL
plugin_LTLIBRARIES += radar.la
radar_la_SOURCES = radar.c   radar.h \
	marching.c marching.h \
	radar-colormap.c
radar_la_LDFLAGS = $(AM_LDFLAGS) $(RSL_LIBS)
endif

MAINTAINERCLEANFILES = Makefile.in
