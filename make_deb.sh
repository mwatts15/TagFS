#!/bin/sh -e

BUILD="$(pwd)/build"

rm -rf $BUILD
mkdir -p $BUILD

BASE=$(pwd)
TARBALL=$(readlink -f $1)

cd $BUILD
cp $TARBALL .
TARBALL=$(basename $TARBALL)

fakeroot tar xpf $TARBALL
PACKAGE=${TARBALL%.tar*}
EXTENSION=${TARBALL#$PACKAGE}
SOURCE_DIR=$(echo ${PACKAGE}-*)
VERSION=${SOURCE_DIR#${PACKAGE}-}
mv $TARBALL ${PACKAGE}_${VERSION}.orig${EXTENSION}
cd $SOURCE_DIR
debuild && cp $BUILD/tagfs*.deb $BUILD/tagfs.deb

