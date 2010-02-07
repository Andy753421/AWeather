AM_CFLAGS   = -Wall --std=gnu99  -I../
AM_CPPFLAGS = $(GIS_CFLAGS)
AM_LDFLAGS  = -module -avoid-version -no-undefined
LIBS        = $(GIS_LIBS)

pluginsdir  = $(pkglibdir)

plugins_LTLIBRARIES =

if HAVE_RSL
plugins_LTLIBRARIES += radar.la
radar_la_SOURCES = \
	radar.c  radar.h \
	level2.c level2.h \
	../aweather-colormap.c \
	../aweather-location.c
radar_la_LDFLAGS = $(AM_LDFLAGS) $(RSL_LIBS)
endif

test:
	( cd ../; make test )

MAINTAINERCLEANFILES = Makefile.in
