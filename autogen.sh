#!/bin/sh

gtkdocize
libtoolize
aclocal
autoheader
automake -a -c
autoconf

# Run configure
./configure "$@"
