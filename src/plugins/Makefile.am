AM_CFLAGS   = -Wall --std=gnu99 $(GRITS_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib
AM_LDFLAGS  = -shared -module -avoid-version -Wl,--as-needed
LIBS        = $(GRITS_LIBS)

pluginsdir  = $(pkglibdir)

plugins_LTLIBRARIES =

if HAVE_RSL
plugins_LTLIBRARIES += radar.la
radar_la_SOURCES = \
	radar.c      radar.h \
	level2.c     level2.h \
	radar-info.c radar-info.h \
	../aweather-location.c \
	../aweather-location.h
radar_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(dots)$(pkgdatadir)\"" \
	-I$(top_srcdir)/src -I$(top_srcdir)/lib
radar_la_LIBADD  = $(RSL_LIBS) $(GRITS_LIBS) \
		   $(addprefix $(top_srcdir)/lib/,$(gl_LTLIBOBJS))
endif

if WIN32
AM_LDFLAGS += -no-undefined
dots = ..
endif

test:
	( cd ../; make test )

MAINTAINERCLEANFILES = Makefile.in
