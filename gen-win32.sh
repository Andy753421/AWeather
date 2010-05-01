#!/bin/bash
dir=$(dirname $(readlink -f $0))
PKG_CONFIG_PATH="../libgis-win32/src/" \
./autogen.sh \
	"--host=i686-pc-mingw32" \
	"--datadir=Z:$dir/data/" \
	"--libdir=Z:$dir/src/plugins/" \
	CFLAGS="-g -Werror -Wno-unused $CFLAGS"
