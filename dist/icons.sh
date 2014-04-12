#!/bin/bash

if [ "$1" = "" ]; then
	echo "Usage ./icons.sh TOPDIR"
	exit
fi

TOPDIR=$1

SOURCE=$TOPDIR/data/icon_32x32.png

cp $SOURCE $TOPDIR/dist/freedesktop/openkb.png
convert $SOURCE $TOPDIR/dist/win/openkb.ico

mkdir $TOPDIR/dist/osx/openkb.iconset
cp $SOURCE $TOPDIR/dist/osx/openkb.iconset/.
