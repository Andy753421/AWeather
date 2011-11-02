AM_CFLAGS   = -Wall --std=gnu99 $(GRITS_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib
AM_LDFLAGS  = -shared -module -avoid-version -Wl,--as-needed
LIBS        = $(GRITS_LIBS)

pluginsdir  = $(pkglibdir)

plugins_LTLIBRARIES = alert.la

alert_la_SOURCES = \
	alert.c      alert.h \
	alert-info.c alert-info.h
alert_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(dots)$(pkgdatadir)\""
alert_la_LIBADD  = $(GRITS_LIBS)

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
	-I$(top_srcdir)/src
radar_la_LIBADD  = $(RSL_LIBS) $(GRITS_LIBS)
endif

if WIN32
AM_LDFLAGS += -no-undefined
dots = ..
endif

test:
	( cd ../; make test )

MAINTAINERCLEANFILES = Makefile.in
