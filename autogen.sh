#!/bin/sh

gtkdocize
aclocal
autoheader
automake -a -c
autoconf

# Run configure
./configure "$@"
