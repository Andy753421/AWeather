SUBDIRS = src data docs

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.h.in gtk-doc.make \
                       configure depcomp install-sh Makefile.in missing

test: all
	LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ \
	./src/aweather

