#!/bin/sh

gnulib-tool --no-vc-files --import strptime

libtoolize
aclocal -I m4
autoconf
autoheader
automake -a -c

# Run configure
./configure "$@"
