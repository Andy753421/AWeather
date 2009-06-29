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
