#!/bin/sh

libtoolize
aclocal -I m4
autoconf
autoheader
automake -a -c

# Run configure
./configure "$@"
