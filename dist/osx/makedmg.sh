#!/bin/bash

function show_usage {
	echo "Usage: ./makedmg.sh APPDIR DESTDIR VERSION"
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

APP=$1/OpenKB.app
DESTDIR=$2
VERSION=$3

# Step 0.
# Clean up.

rm -rf $DESTDIR/dmg
rm $DESTDIR/OpenKB-$VERSION.dmg

# Step 1.
# Create tmp dir and copy data.

mkdir $DESTDIR/dmg
cp -r $APP $DESTDIR/dmg/.
ln -s /Applications $DESTDIR/dmg/.

# Step 2.
# Invoke hdiutil
# See $ man hdiutil

echo -n "creating..."
hdiutil create $DESTDIR/OpenKB-$VERSION.dmg -srcfolder $DESTDIR/dmg -volname OpenKB-$VERSION

# Step last.
# Clean up again.

rm -rf $DESTDIR/dmg
