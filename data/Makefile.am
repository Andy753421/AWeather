gtkbuilderdir = $(datadir)/aweather/
dist_gtkbuilder_DATA = main.ui

configdir = $(datadir)/aweather/
dist_config_DATA = defaults.ini

desktopdir = $(datadir)/applications
dist_desktop_DATA = aweather.desktop

pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = aweather.png

.svg.png:
	rsvg $< $@

maintainer-clean-local:
	rm -f Makefile.in
