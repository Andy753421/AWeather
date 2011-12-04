#!/bin/bash
dir=$(pwd)
PKG_CONFIG_PATH="../grits/src/" \
./autogen.sh \
	"--datadir=$dir/data" \
	"--libdir=$dir/src/plugins" \
	CFLAGS="-g -Werror $CFLAGS"
