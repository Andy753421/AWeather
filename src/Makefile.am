AM_CPPFLAGS  = $(GTK_CFLAGS) -I/usr/local/trmm/GVBOX/include/
AM_CFLAGS    = -Wall -Werror --std=gnu99

bin_PROGRAMS = aweather

aweather_SOURCES  = aweather.c opengl.c cube.c radar.c 
aweather_LDADD    = $(GTK_LIBS) -L/usr/local/trmm/GVBOX/lib/ -lrsl

test: all
	LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ \
	./aweather
