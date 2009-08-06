SUBDIRS = src data docs

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in config.sub \
		       configure depcomp gtk-doc.make install-sh ltmain.sh \
		       Makefile.in missing


ACLOCAL_AMFLAGS=-I m4

test: all
	(cd src && make test)

release: all dist
	@echo -en "\nRelease $(PACKAGE)-$(VERSION) (y/n): "
	@read ans && [ "$$ans" != "y" ] && exit 1 || exit 0
	svn cp \
		https://lug.rose-hulman.edu/svn/proj-$(PACKAGE)/trunk \
		https://lug.rose-hulman.edu/svn/proj-$(PACKAGE)/tags/$(VERSION)
	echo $(VERSION) > LATEST
	scp LATEST $(PACKAGE)-$(VERSION).tar.* \
		"lug@lug.rose-hulman.edu:/home/lug/htdocs/proj/$(PACKAGE)"; \
	rm -f LATEST

depscan:
	gtkdoc-depscan             \
		--book=atk         \
		--book=gdk         \
		--book=gdk-pixbuf  \
		--book=glib        \
		--book=gio         \
		--book=gobject     \
		--book=gtk         \
		--book=pango       \
		--book=libsoup-2.4 \
		--list-unknown     \
		"$@"               \
		src/*.c
