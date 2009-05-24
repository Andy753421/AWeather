AM_LDFLAGS    = $(RSL_LIBS) $(GTK_LIBS)
AM_CFLAGS     = -Wall -Werror -Wno-unused --std=gnu99 
bin_PROGRAMS  = aweather wsr88ddec

BUILT_SOURCES = marshal.c marshal.h

aweather_SOURCES  = main.c \
	marshal.c         marshal.h         \
	aweather-gui.c    aweather-gui.h    \
	aweather-view.c   aweather-view.h   \
	aweather-plugin.c aweather-plugin.h \
	data.c            data.h            \
	location.c        location.h        \
	plugin-radar.c    plugin-radar.h    \
	plugin-radar-colormap.c             \
	plugin-ridge.c    plugin-ridge.h    \
	plugin-example.c  plugin-example.h
aweather_CPPFLAGS = $(RSL_CFLAGS) $(GTK_CFLAGS) -DDATADIR="\"$(datadir)\"" 

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDADD   = -lbz2

.list.c:
	glib-genmarshal --prefix=aweather_cclosure_marshal --body   $< > $@
.list.h:
	glib-genmarshal --prefix=aweather_cclosure_marshal --header $< > $@

test: all
	./aweather

gdb: all
	gdb ./aweather

ddd: all
	ddd ./aweather

memcheck:
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=50               \
	         --suppressions=gtk.suppression \
	         ./aweather                     \
	2> memcheck.log
