SUBDIRS = plugins

AM_CFLAGS   = -Wall --std=gnu99 
AM_CPPFLAGS = $(GIS_CFLAGS)
LIBS        = $(GIS_LIBS)

bin_PROGRAMS = aweather wsr88ddec
aweather_SOURCES  = main.c \
	aweather-gui.c      aweather-gui.h \
	aweather-location.c aweather-location.h
aweather_CPPFLAGS = $(AM_CPPFLAGS) -DDATADIR="\"$(datadir)\""

wsr88ddec       = wsr88ddec.c
wsr88ddec_LDADD = -lbz2

CLEANFILES = gmon.out valgrind.out
MAINTAINERCLEANFILES = Makefile.in

test: all
	aweather -o -d 7 -s KVNX

gdb: all
	gdb aweather

ddd: all
	ddd aweather
