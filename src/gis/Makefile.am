AM_CFLAGS   = -Wall --std=gnu99 
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(SOUP_CFLAGS)
AM_LDADD    = $(GLIB_LIBS) $(GTK_LIBS) $(SOUP_LIBS)

# Library
BUILT_SOURCES = gis-marshal.c gis-marshal.h
lib_LTLIBRARIES  = libgis.la
gis_includedir = $(includedir)/gis
gis_include_HEADERS = \
	gis.h         \
	gis-world.h   \
	gis-view.h    \
	gis-prefs.h   \
	gis-opengl.h  \
	gis-plugin.h  \
	gis-data.h    \
	gpqueue.h     \
	roam.h        \
	wms.h
libgis_la_SOURCES = \
	gis-world.c   gis-world.h   \
	gis-view.c    gis-view.h    \
	gis-prefs.c   gis-prefs.h   \
	gis-opengl.c  gis-opengl.h  \
	gis-plugin.c  gis-plugin.h  \
	gis-marshal.c gis-marshal.h \
	gis-data.c    gis-data.h    \
	roam.c        roam.h        \
	wms.c         wms.h         \
	gpqueue.c     gpqueue.h
libgis_la_CPPFLAGS = $(AM_CPPFLAGS) \
	-DDATADIR="\"$(datadir)\"" -DPLUGINDIR="\"$(libdir)/gis\""
libgis_la_LIBADD = $(AM_LDADD)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgis.pc

# Test programs
bin_PROGRAMS = gis_test wms_test

gis_test_SOURCES  = gis_test.c gis.h
gis_test_LDADD    = $(AM_LDADD) libgis.la

wms_test_SOURCES  = wms_test.c gis-world.c gis-world.h wms.c wms.h
wms_test_LDADD    = $(AM_LDADD)

MAINTAINERCLEANFILES = Makefile.in

.list.c:
	glib-genmarshal --prefix=gis_cclosure_marshal --body   $< > $@
.list.h:
	glib-genmarshal --prefix=gis_cclosure_marshal --header $< > $@

test: all
	make -C ../plugins all
	LD_LIBRARY_PATH=.libs .libs/gis_test

gdb: all
	LD_LIBRARY_PATH=.libs gdb .libs/gis_test

ddd: all
	LD_LIBRARY_PATH=.libs ddd .libs/gis_test

memcheck: all
	LD_LIBRARY_PATH=.libs                      \
	G_SLICE=always-malloc                      \
	G_DEBUG=gc-friendly,resident-modules       \
	valgrind --track-origins=yes               \
	         --leak-check=full                 \
	         --leak-resolution=high            \
	         --num-callers=100                 \
	         --suppressions=../gtk.suppression \
	         .libs/gis_test                    \
	2> valgrind.out
