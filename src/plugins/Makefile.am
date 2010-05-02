AM_CFLAGS   = -Wall --std=gnu99 $(GIS_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib
AM_LDFLAGS  = -module -avoid-version -no-undefined

pluginsdir  = $(pkglibdir)

plugins_LTLIBRARIES =

if HAVE_RSL
plugins_LTLIBRARIES += radar.la
radar_la_SOURCES = \
	radar.c  radar.h \
	level2.c level2.h \
	../aweather-colormap.c \
	../aweather-location.c
radar_la_LIBADD  = $(RSL_LIBS) $(GIS_LIBS) \
		   $(addprefix $(top_srcdir)/lib/,$(gl_LTLIBOBJS))
endif

test:
	( cd ../; make test )

MAINTAINERCLEANFILES = Makefile.in
