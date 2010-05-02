#!/bin/bash
gnulib-tool --no-vc-files --import strptime

GIS="/scratch/libgis-win32/src"
export PKG_CONFIG_PATH="$GIS"
export LIBS="-L$GIS"
export CPPFLAGS="-I$GIS"
export CFLAGS="-g -Werror -Wno-unused -O3"
./autogen.sh --host=i686-pc-mingw32
