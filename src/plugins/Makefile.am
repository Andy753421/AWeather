AM_CFLAGS   = -Wall -Werror -Wno-unused --std=gnu99  -I../
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS)
AM_LDFLAGS  = -module $(GLIB_LIBS) $(GTK_LIBS)
LIBS        = $(top_srcdir)/src/gis/libgis.la

plugindir = $(libdir)/gis

plugin_LTLIBRARIES = \
	example.la \
	radar.la   \
	ridge.la

example_la_SOURCES  = example.c example.h
example_la_CPPFLAGS = $(AM_CPPFLAGS) $(SOUP_CFLAGS)

ridge_la_SOURCES    = ridge.c ridge.h
ridge_la_CPPFLAGS   = $(AM_CPPFLAGS) $(SOUP_CFLAGS)

radar_la_SOURCES = \
	radar.c    radar.h \
	marching.c marching.h \
	radar-colormap.c
radar_la_CPPFLAGS = $(AM_CPPFLAGS) $(SOUP_CFLAGS) $(RSL_CFLAGS)
radar_la_LIBADD = $(SOUP_LIBS) $(RSL_LIBS)

MAINTAINERCLEANFILES = Makefile.in
