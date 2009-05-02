AM_CPPFLAGS  = $(GTK_CFLAGS)
AM_CFLAGS    = -Wall -Werror --std=gnu99

bin_PROGRAMS = aweather

aweather_SOURCES  = aweather.c opengl.c cube.c radar.c 
aweather_LDADD    = $(GTK_LIBS) -lrsl
