AM_CFLAGS = -Wall -Werror -Wno-unused --std=gnu99  -I../ \
	$(RSL_CFLAGS) $(SOUP_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS)
AM_LDFLAGS = -module \
	$(RSL_LIBS) $(SOUP_LIBS) $(GLIB_LIBS) $(GTK_LIBS)

plugindir = $(libdir)/aweather

plugin_LTLIBRARIES = \
	example.la \
	radar.la   \
	ridge.la

example_la_SOURCES = \
	example.c \
	example.h

ridge_la_SOURCES = \
	ridge.c \
	ridge.h

radar_la_SOURCES = \
	radar.c \
	radar.h \
	radar-colormap.c


MAINTAINERCLEANFILES = Makefile.in
