AM_CFLAGS     = -Wall -Werror -Wno-unused --std=gnu99 
bin_PROGRAMS  = aweather wsr88ddec

BUILT_SOURCES = marshal.c marshal.h

aweather_SOURCES  = main.c \
	marshal.c         marshal.h         \
	aweather-gui.c    aweather-gui.h    \
	aweather-plugin.c aweather-plugin.h \
	gis-opengl.c      gis-opengl.h      \
	gis-view.c        gis-view.h        \
	gis-world.c       gis-world.h       \
	data.c            data.h            \
	location.c        location.h        \
	marching.c        marching.h        \
	plugin-radar.c    plugin-radar.h    \
	plugin-radar-colormap.c             \
	plugin-ridge.c    plugin-ridge.h    \
	plugin-example.c  plugin-example.h
aweather_CPPFLAGS = -DDATADIR="\"$(datadir)\"" \
	$(RSL_CFLAGS) $(SOUP_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS)
aweather_LDADD    = $(RSL_LIBS) $(SOUP_LIBS) $(GLIB_LIBS) $(GTK_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDADD   = -lbz2

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

.list.c:
	glib-genmarshal --prefix=aweather_cclosure_marshal --body   $< > $@
.list.h:
	glib-genmarshal --prefix=aweather_cclosure_marshal --header $< > $@

test: all
	./aweather -o -d 7 -s KVNX

gdb: all
	gdb ./aweather

ddd: all
	ddd ./aweather

memcheck:
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=100              \
	         --suppressions=gtk.suppression \
	         ./aweather                     \
	2> valgrind.out
