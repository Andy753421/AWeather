#!/bin/bash
dir=$(dirname $(readlink -f $0))
# "--libdir=$dir/src/plugins"
# "--datadir=$dir/data"
./autogen.sh \
	--enable-gtk-doc \
	CFLAGS="-g -Werror -Wno-unused $CFLAGS" \
	LDFLAGS="-Wl,-z,defs"
