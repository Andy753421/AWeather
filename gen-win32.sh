#!/bin/bash
gnulib-tool --no-vc-files --import strptime

GRITS="/scratch/grits-win32/src"
export PKG_CONFIG_PATH="$GRITS"
export LIBS="-L$GRITS"
export CPPFLAGS="-I$GRITS"
export CFLAGS="-g -Werror -Wno-unused -O3"
./autogen.sh --host=i686-pc-mingw32
