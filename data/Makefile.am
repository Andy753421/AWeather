gtkbuilderdir = $(datadir)/aweather/
dist_gtkbuilder_DATA = main.ui

desktopdir = $(datadir)/applications
dist_desktop_DATA = aweather.desktop

pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = aweather.png

.svg.png:
	convert $< $@

maintainer-clean-local:
	rm -f Makefile.in
