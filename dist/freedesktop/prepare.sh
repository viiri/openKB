#!/bin/bash

function show_usage {
	echo "Usage: ./prepare.sh TOPDIR DESTDIR VERSION"
	exit
}
if [ "$1" = "" ]; then
	show_usage
fi
if [ "$2" = "" ]; then
	show_usage
fi
if [ "$3" = "" ]; then
	show_usage
fi

TOPDIR=$1
DESTDIR=$2
VERSION=$3

cp $TOPDIR/data/icon_32x32.png $DESTDIR/openkb.png
sed s/@VERSION@/$VERSION/g $TOPDIR/dist/freedesktop/openkb.desktop.in > $DESTDIR/openkb.desktop
