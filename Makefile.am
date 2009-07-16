SUBDIRS = src data docs

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in gtk-doc.make \
                       configure depcomp install-sh Makefile.in missing

test: all
	(cd src && make test)

release: all dist
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
