SUBDIRS = gis plugins

AM_CFLAGS   = -Wall -Werror -Wno-unused --std=gnu99 
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(SOUP_CFLAGS)
AM_LDADD    = $(GLIB_LIBS) $(GTK_LIBS) $(SOUP_LIBS)

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = $(AM_CPPFLAGS) -DDATADIR="\"$(datadir)\""
aweather_LDADD    = $(AM_LDADD) gis/libgis.la

wsr88ddec       = wsr88ddec.c
wsr88ddec_LDADD = -lbz2

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	LD_LIBRARY_PATH=gis/.libs .libs/aweather -o -d 7 -s KVNX

gdb: all
	LD_LIBRARY_PATH=gis/.libs gdb .libs/aweather

ddd: all
	LD_LIBRARY_PATH=gis/.libs ddd .libs/aweather

memcheck: all
	LD_LIBRARY_PATH=gis/.libs               \
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=100              \
	         --suppressions=gtk.suppression \
	         .libs/aweather                 \
	2> valgrind.out
