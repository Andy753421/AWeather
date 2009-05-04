AM_CPPFLAGS  = $(GTK_CFLAGS) -I/usr/local/trmm/GVBOX/include/
AM_CFLAGS    = -O3 -Wall -Werror --std=gnu99

bin_PROGRAMS = aweather

aweather_SOURCES  = aweather.c opengl.c radar.c ridge.c example.c 
aweather_LDADD    = $(GTK_LIBS) -L/usr/local/trmm/GVBOX/lib/ -lrsl

test: all
	LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ \
	./aweather
