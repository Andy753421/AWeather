SUBDIRS = data src docs

ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in config.sub \
		       configure depcomp gtk-doc.make install-sh ltmain.sh \
		       Makefile.in missing

test: all
	(cd src && make test)

release: all dist
	@echo -en "\nRelease $(PACKAGE)-$(VERSION) (y/n): "
	@read ans && [ "$$ans" != "y" ] && exit 1 || exit 0
	git tag -s -m "Version $(VERSION)" v$(VERSION)
	git push --all
	echo $(VERSION) > LATEST
	scp LATEST $(PACKAGE)-$(VERSION).tar.* \
		"andy@pileus.org:/home/pileus/htdocs/$(PACKAGE)/files"; \
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
