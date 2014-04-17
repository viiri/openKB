#!/bin/sh

echo "Clearing cache..."
rm -rf autom4te.cache/
test $? != 0 && echo "Failed" && exit 1
echo "Running autoheader"
autoheader
test $? != 0 && echo "Failed" && exit 2
echo "Running aclocal"
aclocal
test $? != 0 && echo "Failed" && exit 3
echo "Running autoconf [twice]"
autoconf 2> /dev/null || autoconf
test $? != 0 && echo "Failed" && exit 4
echo "Running automake --add-mising (to get install-sh)"
automake --add-missing 2>/dev/null
if [ ! -f ./install-sh ]; then
 echo "Failed, try adding install-sh manually"
 exit 5
fi
chmod +x ./configure # For weird systems
echo "You can run './configure' now"
exit 0
