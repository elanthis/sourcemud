#!/bin/sh

BASE_VERSION=`sed -n 's/^BASE_VERSION=\(.*\)$/\1/p' configure.ac`
CURVER=`sed -n 's/^VERSION=\(.*\)$/\1/p' configure.ac`

if [ -n "$1" ] ; then
	NEWVER="$1"
else
	DATE=`date +%Y%m%d`
	NEWVER="$BASE_VERSION.$DATE"
fi

if [ "x$CURVER" = "x$NEWVER" ] ; then
	# no change...
	exit 1
fi

echo "Current version: $CURVER"
echo "New version:     $NEWVER"

if ! echo "$NEWVER" | grep -q -E "^$BASE_VERSION" ; then
	echo
	read -p "Different version base - update configure.ac base version? (Y/n) " DOBASE
	echo
else
	DOBASE='n'
fi

if [ "x$NEWVER" != "x$CURVER" ] ; then
	echo "Updating configure.ac..."
	sed "/BASE_VERSION/!s/$CURVER/$NEWVER/" configure.ac > configure.new

	if [ "x$DOBASE" != "xn" ] ; then
		sed "s/^BASE_VERSION=.*$/BASE_VERSION=$NEWVER/" configure.new > configure.ac
		rm -f configure.new
	else
		mv -f configure.new configure.ac
	fi
fi

exit 0
