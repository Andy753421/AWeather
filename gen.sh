#!/bin/bash
dir=$(dirname $(readlink -f $0))
CFLAGS="-g -Werror -Wno-unused" \
./autogen.sh \
	"--libdir=$dir/src/plugins" \
	"--datadir=$dir/data" \
	--enable-gtk-doc
