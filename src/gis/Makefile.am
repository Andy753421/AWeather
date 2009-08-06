AM_CFLAGS = -Wall -Werror -Wno-unused --std=gnu99 

BUILT_SOURCES = gis-marshal.c gis-marshal.h
lib_LTLIBRARIES  = libgis.la
libgis_la_SOURCES = \
	gis-prefs.c   gis-prefs.h  \
	gis-world.c   gis-world.h  \
	gis-view.c    gis-view.h   \
	gis-opengl.c  gis-opengl.h \
	gis-plugin.c  gis-plugin.h \
	gis-data.c    gis-data.h   \
	gis-marshal.c gis-marshal.h
libgis_la_CPPFLAGS = -DPLUGINDIR="\"$(libdir)/aweather\"" $(SOUP_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS)
libgis_la_LIBADD = $(SOUP_LIBS) $(GLIB_LIBS) $(GTK_LIBS)

MAINTAINERCLEANFILES = Makefile.in

.list.c:
	glib-genmarshal --prefix=gis_cclosure_marshal --body   $< > $@
.list.h:
	glib-genmarshal --prefix=gis_cclosure_marshal --header $< > $@
