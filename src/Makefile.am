SUBDIRS = gis plugins

AM_CFLAGS = -Wall -Werror -Wno-unused --std=gnu99 

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = -DDATADIR="\"$(datadir)\"" $(SOUP_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS)
aweather_LDADD    = gis/libgis.la $(SOUP_LIBS) $(GLIB_LIBS) $(GTK_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDFLAGS = -lbz2

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	./aweather -o -d 7 -s KVNX

gdb: all
	gdb .libs/aweather

ddd: all
	ddd .libs/aweather

memcheck:
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=100              \
	         --suppressions=gtk.suppression \
	         .libs/aweather                 \
	2> valgrind.out
