SUBDIRS = plugins

AM_CFLAGS   = -Wall --std=gnu99 $(GRITS_CFLAGS)
AM_LDFLAGS  = -Wl,--export-dynamic -Wl,--no-undefined

if !SYS_MAC
AM_LDFLAGS += -Wl,--as-needed 
endif

EXTRA_DIST  = compat.h

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = \
	-DHTMLDIR="\"$(DOTS)$(htmldir)\"" \
	-DICONDIR="\"$(DOTS)$(datadir)/icons\"" \
	-DPKGDATADIR="\"$(DOTS)$(pkgdatadir)\"" \
	-DPLUGINSDIR="\"$(DOTS)$(pkglibdir)\""
aweather_LDADD    = $(GRITS_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDADD   = $(GLIB_LIBS) -lbz2

if SYS_WIN
wsr88ddec_LDFLAGS     = -mwindows

aweather_SOURCES     += resource.rc
aweather_LDFLAGS      = -mwindows
aweather_CPPFLAGS    += -I$(top_srcdir)/lib

bin_PROGRAMS         += aweather-dbg
aweather_dbg_SOURCES  = $(aweather_SOURCES)
aweather_dbg_CPPFLAGS = $(aweather_CPPFLAGS)
aweather_dbg_LDADD    = $(aweather_LDADD)
endif

if SYS_MAC
aweather_CPPFLAGS    += $(MAC_CFLAGS)
aweather_LDADD       += $(MAC_LIBS)
endif

.rc.o: ../data/icons/48x48/aweather.ico
	$(RC) -o $@ $<

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	.libs/aweather -o -d 5
#.libs/aweather -o -d 7 -s KOUN -t '2011-02-01 10:01Z'
#.libs/aweather -o -d 7 -s KLSX -t '2010-12-31 17:56Z'
#.libs/aweather -o -d 7 -s KDGX -t '2010-12-31 23:10Z'
#.libs/aweather -o -d 7 -s KDGX -t '1999-05-03 23:51Z'

gdb: all
	gdb .libs/aweather

ddd: all
	ddd .libs/aweather

memcheck: all
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --track-origins=yes            \
	         --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=50               \
	         --suppressions=gtk.suppression \
	         --suppressions=/usr/lib/valgrind/default.supp \
	         .libs/aweather                 \
	2> valgrind.out
