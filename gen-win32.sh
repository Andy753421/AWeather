#!/bin/bash

GRITS="/scratch/grits-win32/src"
export PKG_CONFIG_PATH="$GRITS"
export LIBS="-L$GRITS"
export CPPFLAGS="-I$GRITS"
export CFLAGS="-g -Werror -Wno-unused -O3"
./autogen.sh \
	--enable-shared \
	--disable-static \
	--host=i686-pc-mingw32
