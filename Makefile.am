SUBDIRS = data src

test: all
	LD_LIBRARY_PATH=/usr/local/trmm/GVBOX/lib/ \
	./src/aweather

maintainer-clean-local:
	rm -f aclocal.m4 compile config.guess config.h.in \
	      configure depcomp install-sh Makefile.in missing
