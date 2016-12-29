#!/bin/sh -e

TMPDIR="$(mktemp -d)"

cleanup () {
    rm -rf $TMPDIR
}

trap cleanup EXIT 
BASE=$(pwd)
TARBALL=$(readlink -f $1)

cd $TMPDIR
cp $TARBALL .
TARBALL=$(basename $TARBALL)

tar xf $TARBALL
PACKAGE=${TARBALL%.tar*}
EXTENSION=${TARBALL#$PACKAGE}
SOURCE_DIR=$(echo $PACKAGE-*)
VERSION=${SOURCE_DIR#$PACKAGE-}
mv $TARBALL ${PACKAGE}_${VERSION}.orig${EXTENSION}
cd $SOURCE_DIR
debuild
cp $TMPDIR/tagfs*deb $BASE/
