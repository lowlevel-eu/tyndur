#!/bin/bash

if [ "$1" == "" -o "$2" == "" ]; then
	echo "Usage: lpt-index.sh <package directory> <destination directory>"
	exit -1
fi

PACKAGE_DIRECTORY=$1
DESTINATION_DIRECTORY=$2

if [ ! -d "$DESTINATION_DIRECTORY" ]; then
	mkdir -p "$DESTINATION_DIRECTORY"
else
	rm $DESTINATION_DIRECTORY/*
fi

for file in $PACKAGE_DIRECTORY/*.tar
do
	extension=${file##*.}

	filename=${file##*/}
	filenamewithoutsuffix=${file%.*}
	packagename=${filename%-*}
	packageversion=${filenamewithoutsuffix##*-}

	if [[ $packagename == lib* ]]; then
		type="lib"
        else
		type="bin"
        fi

	echo "P $packagename" >> "$DESTINATION_DIRECTORY/packages.i386"
	echo "D no description" >> "$DESTINATION_DIRECTORY/packages.i386"
	echo "S $type" >> "$DESTINATION_DIRECTORY/packages.i386"
	echo "V $packageversion" >> "$DESTINATION_DIRECTORY/packages.i386"
	echo "s $(ls -l $file | awk '{print $5}')" >> "$DESTINATION_DIRECTORY/packages.i386"
	echo "" >> "$DESTINATION_DIRECTORY/packages.i386"

	cp "$file" "$DESTINATION_DIRECTORY/$packagename-$type-$packageversion-i386.tar"
done
