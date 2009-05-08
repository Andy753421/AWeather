AM_CPPFLAGS  = -DDATADIR="\"$(datadir)\"" $(RSL_CFLAGS) $(GTK_CFLAGS) $(CURL_CFLAGS) 
AM_CFLAGS    = -O3 -Wall -Werror --std=gnu99 
AM_LDFLAGS   = -Wl,--export-dynamic
bin_PROGRAMS = aweather

aweather_SOURCES  = \
	aweather.c       aweather.h       \
	site.c           site.h           \
	opengl.c         opengl.h         \
	plugin-radar.c   plugin-radar.h   \
	plugin-ridge.c   plugin-ridge.h   \
	plugin-example.c plugin-example.h
aweather_LDADD    = $(RSL_LIBS) $(GTK_LIBS) $(CURL_LIBS)


test: all
	LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ \
	./aweather
