#!/bin/bash
dir=$(dirname $(readlink -f $0))
./autogen.sh \
	"--libdir=$dir/src/plugins" \
	"--datadir=$dir/data" \
	--enable-gtk-doc \
	CFLAGS="-g" \
	LDFLAGS="-Wl,-z,defs"
