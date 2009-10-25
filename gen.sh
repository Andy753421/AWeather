#!/bin/bash
dir=$(dirname $(readlink -f $0))
PKG_CONFIG_PATH="../libgis/src/" \
./autogen.sh \
	"--datadir=$dir/data" \
	"--libdir=$dir/src/plugins" \
	CFLAGS="-g -Werror -Wno-unused $CFLAGS" \
	LDFLAGS="-Wl,-z,defs"
