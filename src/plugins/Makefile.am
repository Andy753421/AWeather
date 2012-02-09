AM_CFLAGS   = -Wall --std=gnu99 $(GRITS_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib
AM_LDFLAGS  = -shared -module -avoid-version
LIBS        = $(GRITS_LIBS)

if SYS_WIN
AM_LDFLAGS += -no-undefined
endif
if !SYS_MAC
AM_LDFLAGS += -Wl,--as-needed
endif

pluginsdir  = $(pkglibdir)

plugins_LTLIBRARIES = alert.la

alert_la_SOURCES = \
	alert.c      alert.h \
	alert-info.c alert-info.h
alert_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(DOTS)$(pkgdatadir)\""
alert_la_LIBADD  = $(GRITS_LIBS)

if HAVE_GPSD
plugins_LTLIBRARIES += gps.la
gps_la_SOURCES = \
	gps-plugin.c      gps-plugin.h
gps_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(DOTS)$(pkgdatadir)\"" \
	-I$(top_srcdir)/src
gps_la_LIBADD  = $(GPSD_LIBS) $(GRITS_LIBS)
endif

if HAVE_RSL
plugins_LTLIBRARIES += radar.la
radar_la_SOURCES = \
	radar.c      radar.h \
	level2.c     level2.h \
	radar-info.c radar-info.h \
	../aweather-location.c \
	../aweather-location.h
radar_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(DOTS)$(pkgdatadir)\"" \
	-I$(top_srcdir)/src
radar_la_LIBADD  = $(RSL_LIBS) $(GRITS_LIBS)
endif

test:
	( cd ../; make test )

MAINTAINERCLEANFILES = Makefile.in
