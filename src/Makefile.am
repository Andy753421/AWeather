SUBDIRS = plugins

AM_CFLAGS = -Wall --std=gnu99 

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = -DPKGDATADIR="\"$(pkgdatadir)\"" -DPLUGINSDIR="\"$(pkglibdir)\""
aweather_CFLAGS   = $(GIS_CFLAGS) $(AM_CFLAGS)
aweather_LDADD    = $(GIS_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_CFLAGS  = $(GLIB_CFLAGS) $(AM_CFLAGS)
wsr88ddec_LDADD   = $(GLIB_LIBS) -lbz2

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	.libs/aweather -o -d 7

gdb: all
	gdb .libs/aweather

ddd: all
	ddd .libs/aweather
