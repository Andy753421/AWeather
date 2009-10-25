#!/bin/sh

libtoolize
aclocal
autoheader
automake -a -c
autoconf

# Run configure
./configure "$@"
