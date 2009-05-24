gtkbuilderdir = $(datadir)/aweather/
dist_gtkbuilder_DATA = main.xml about.xml

.glade.xml:
	gtk-builder-convert $< $@

clean-local:
	rm -f *.xml
maintainer-clean-local:
	rm -f Makefile.in
