#!/bin/bash
dir=$(dirname $(readlink -f $0))
echo CFLAGS="-g" ./autogen.sh "--datadir=$dir/data" --enable-gtk-doc
CFLAGS="-g" ./autogen.sh "--datadir=$dir/data" --enable-gtk-doc
