#!/bin/sh

aclocal
autoheader
automake -a -c
autoconf

# Run configure
./configure "$@"
