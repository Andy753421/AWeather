SUBDIRS = src data docs

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in gtk-doc.make \
                       configure depcomp install-sh Makefile.in missing

test: all
	(cd src && make test)

post: all dist
	echo svn cp \
		https://lug.rose-hulman.edu/svn/proj-aweather/trunk \
		https://lug.rose-hulman.edu/svn/proj-aweather/tags/$(VERSION)
	echo $(VERSION) > LATEST
	scp LATEST plotter-$(VERSION).tar.* \
		"root@$(SERVER):/home/plotter/htdocs/files"; \
	echo rm -f LATEST
