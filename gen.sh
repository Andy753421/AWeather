#!/bin/bash
dir=$(dirname $(readlink -f $0))
CFLAGS="-g" \
./autogen.sh \
	"--libdir=$dir/src/plugins" \
	"--datadir=$dir/data" \
	--enable-gtk-doc
