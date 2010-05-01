SUBDIRS = lib src data docs

ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in config.sub \
		       configure depcomp gtk-doc.make install-sh ltmain.sh \
		       Makefile.in missing

EXTRA_DIST = m4/gnulib-cache.m4

test: all
	(cd src && make test)

release: all dist
	@echo -en "\nRelease $(PACKAGE)-$(VERSION) (y/n): "
	@read ans && [ "$$ans" != "y" ] && exit 1 || exit 0
	git tag -s -m "Version $(VERSION)" v$(VERSION)
	git push --all
	echo $(VERSION) > LATEST
	scp LATEST $(PACKAGE)-$(VERSION).tar.* \
		"spenceal@lug.rose-hulman.edu:/home/lug/htdocs/proj/$(PACKAGE)"; \
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
