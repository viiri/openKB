#!/bin/sh

function show_usage {
	echo "Usage: ./toolszip.sh TOPDIR DESTDIR VERSION [DLLDIR]"
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
DLLDIR=$WINDIR
TMPDIR=tools

if [ ! "$4" = "" ]; then
	DLLDIR=$4
fi


ZIPFILE=$DESTDIR/openkb-tools-$VERSION-win32.zip

# Get "zip" utility
$TOPDIR/dist/win/get-zip.sh

# Cleanup and setup
rm -rf $TMPDIR
mkdir $TMPDIR

# Copy files
cp $TOPDIR/src/COPYING $TMPDIR/.
cp $TOPDIR/src/tools/*.exe $TMPDIR/.

# Zip the result
cd $TMPDIR
zip -r ../tmp.zip *
cd ..
mv tmp.zip $ZIPFILE

# Cleanup
#rm -rf zip

