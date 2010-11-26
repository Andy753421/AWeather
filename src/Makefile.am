SUBDIRS = plugins

AM_CFLAGS   = -Wall --std=gnu99 $(GRITS_CFLAGS)
AM_LDFLAGS  = -Wl,--export-dynamic

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = \
	-DHTMLDIR="\"$(dots)$(htmldir)\"" \
	-DPKGDATADIR="\"$(dots)$(pkgdatadir)\"" \
	-DPLUGINSDIR="\"$(dots)$(pkglibdir)\""
aweather_LDADD    = $(GRITS_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDADD   = $(GLIB_LIBS) -lbz2

if WIN32
dots = ..

wsr88ddec_LDFLAGS     = -mwindows

aweather_SOURCES     += resource.rc
aweather_LDFLAGS      = -mwindows

bin_PROGRAMS         += aweather-dbg
aweather_dbg_SOURCES  = $(aweather_SOURCES)
aweather_dbg_CPPFLAGS = $(aweather_CPPFLAGS)
aweather_dbg_LDADD    = $(aweather_LDADD)
endif

.rc.o: ../data/aweather.ico
	$(RC) -o $@ $<

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	.libs/aweather -o -d 7

gdb: all
	gdb .libs/aweather

ddd: all
	ddd .libs/aweather
