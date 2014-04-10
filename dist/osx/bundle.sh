#!/bin/bash

# This script was made with help of the following:
# http://stackoverflow.com/questions/1596945/building-osx-app-bundle
# http://stackoverflow.com/questions/12306223/how-to-manually-create-icns-files-using-iconutil
# https://developer.apple.com/library/mac/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html
# https://developer.apple.com/library/mac/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html

function show_usage {
	echo "Usage: ./bundle.sh TOPDIR DESTDIR"
	exit
}
if [ "$1" = "" ]; then
	show_usage
fi
if [ "$2" = "" ]; then
	show_usage
fi

TOPDIR=$1
DESTDIR=$2

ICONDIR=$TOPDIR/dist/openkb.iconset
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

# Step 3.
# Copy data and Info.plist.

cp $TOPDIR/dist/osx/Info.plist $APP/Contents/.
cp -r $TOPDIR/data $APP/Contents/Resources

# Step 4.
# Create iconset.

iconutil -c icns -o $APP/Contents/Resources/openkb.icns $ICONDIR
