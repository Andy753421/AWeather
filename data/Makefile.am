gtkbuilderdir = $(datadir)/aweather/
dist_gtkbuilder_DATA = main.ui

configdir = $(datadir)/aweather/
dist_config_DATA = defaults.ini

fipsdir = $(datadir)/aweather/
dist_fips_DATA = fips.txt

logodir = $(datadir)/aweather/
dist_logo_DATA = logo.svg

icondir = $(datadir)/aweather/
dist_icon_DATA = car.png arrow.png

colordir = $(datadir)/aweather/colors/
dist_color_DATA = colors/*.clr

desktopdir = $(datadir)/applications
dist_desktop_DATA = aweather.desktop

icon16dir = $(datadir)/icons/hicolor/16x16/apps/
icon22dir = $(datadir)/icons/hicolor/22x22/apps/
icon24dir = $(datadir)/icons/hicolor/24x24/apps/
icon32dir = $(datadir)/icons/hicolor/32x32/apps/
icon48dir = $(datadir)/icons/hicolor/48x48/apps/
iconSCdir = $(datadir)/icons/hicolor/scalable/apps

dist_icon16_DATA = icons/16x16/aweather.png
dist_icon22_DATA = icons/22x22/aweather.png
dist_icon24_DATA = icons/24x24/aweather.png
dist_icon32_DATA = icons/32x32/aweather.png
dist_icon48_DATA = icons/48x48/aweather.png
dist_iconSC_DATA = icons/scalable/aweather.svg

dist_noinst_DATA = icons/48x48/aweather.ico

fips.txt:
	wget http://lug.rose-hulman.edu/proj/aweather/files/fips/fips.txt

install-data-hook:
	if test -z "$(DESTDIR)"; then \
		gtk-update-icon-cache -f -t $(datadir)/icons/hicolor; \
	fi

glade:
	GLADE_CATALOG_PATH=. glade-3 main.ui.in

.png.ico:
	icotool -c $< -o $@

maintainer-clean-local:
	rm -f Makefile.in
