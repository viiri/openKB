#!/bin/bash

# This script was made with help of the following:
# http://stackoverflow.com/questions/1596945/building-osx-app-bundle
# http://stackoverflow.com/questions/12306223/how-to-manually-create-icns-files-using-iconutil
# https://developer.apple.com/library/mac/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html
# https://developer.apple.com/library/mac/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html

function show_usage {
	echo "Usage: ./bundle.sh TOPDIR DESTDIR VERSION"
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

ICONDIR=$TOPDIR/dist/osx/openkb.iconset
APP=$DESTDIR/OpenKB.app

# Step 0.
# Clean up.

rm -rf $APP

# Step 1. 
# Create basic structure.

mkdir $APP
mkdir $APP/Contents
mkdir $APP/Contents/MacOS
mkdir $APP/Contents/Resources

# Step 2.
# Copy binary and dylibs.

cp $TOPDIR/openkb $APP/Contents/MacOS/openkb

DYLIBS=`otool -L $TOPDIR/openkb | grep SDL | awk -F' ' '{ print $1 }'`
for dylib in $DYLIBS; do
	NAME=`basename $dylib`
	echo "Dylib: $dylib [ $NAME ]" 
	cp $dylib $APP/Contents/MacOS/
	install_name_tool -change $dylib @executable_path/$NAME $APP/Contents/MacOS/openkb
done

cp $TOPDIR/dist/osx/launch_openkb.sh $APP/Contents/MacOS/launch_openkb.sh

# Step 3.
# Copy data and Info.plist.

#cp $TOPDIR/dist/osx/Info.plist.in $APP/Contents/Info.plist
sed s/@VERSION@/$VERSION/ $TOPDIR/dist/osx/Info.plist.in > $APP/Contents/Info.plist
cp -r $TOPDIR/data $APP/Contents/Resources

# Hack! For some reason, icon_32x32.png when being read by Mac version,
# crashes the program. Since it's not needed anyways, remove it.
rm $APP/Contents/Resources/data/icon_32x32.png
# TODO: Fix this properly.

# Step 4.
# Create iconset.

mkdir $ICONDIR
cp $TOPDIR/data/icon_32x32.png $ICONDIR/.
iconutil -c icns -o $APP/Contents/Resources/openkb.icns $ICONDIR
rm -rf $ICONDIR
