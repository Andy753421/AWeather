AM_CPPFLAGS  = -DDATADIR="\"$(datadir)\"" $(RSL_CFLAGS) $(GTK_CFLAGS) $(CURL_CFLAGS) 
AM_CFLAGS    = -Wall -Werror -Wno-unused --std=gnu99 
AM_LDFLAGS   = -Wl,--export-dynamic
bin_PROGRAMS = aweather wsr88ddec

aweather_SOURCES  = main.c \
	aweather-gui.c   aweather-gui.h   \
	aweather-view.c  aweather-view.h  \
	data.c           data.h           \
	location.c       location.h       \
	plugin-radar.c   plugin-radar.h   \
	plugin-radar-colormap.c           \
	plugin-ridge.c   plugin-ridge.h   \
	plugin-example.c plugin-example.h
aweather_LDADD    = $(RSL_LIBS) $(GTK_LIBS) $(CURL_LIBS)

wsr88ddec         = wsr88ddec.c
wsr88ddec_LDADD   = -lbz2

# LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ 

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
