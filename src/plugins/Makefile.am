AM_CFLAGS   = -Wall -Werror -Wno-unused --std=gnu99  -I../
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(SOUP_CFLAGS)
AM_LDFLAGS  = -module -avoid-version

plugindir = $(libdir)/gis

plugin_LTLIBRARIES = \
	example.la \
	radar.la   \
	ridge.la

example_la_SOURCES = example.c example.h

ridge_la_SOURCES   = ridge.c   ridge.h

radar_la_SOURCES   = radar.c   radar.h \
	marching.c marching.h \
	radar-colormap.c
radar_la_CPPFLAGS = $(RSL_CFLAGS) $(AM_CPPFLAGS) 
radar_la_LIBADD   = $(RSL_LIBS)

MAINTAINERCLEANFILES = Makefile.in
