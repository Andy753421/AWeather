gtkbuilderdir = $(datadir)/aweather/
dist_gtkbuilder_DATA = main.ui

configdir = $(datadir)/aweather/
dist_config_DATA = defaults.ini

if WIN32
noinst_DATA = aweather.ico
else
desktopdir = $(datadir)/applications
dist_desktop_DATA = aweather.desktop

pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = aweather.png
endif

.svg.png:
	rsvg -w 48 -h 48 $< $@
.png.ico:
	icotool -c $< -o $@

maintainer-clean-local:
	rm -f Makefile.in
