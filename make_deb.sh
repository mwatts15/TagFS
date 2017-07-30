#!/bin/sh -e

BUILD=${BUILDDIR:-"$(pwd)/build"}
BUILD=$(readlink -nf "$BUILD")
TARBALL=$(readlink -f $1)
DEBIAN_TARBALL=$(readlink -f $2)

cd ${BUILD}

fakeroot tar xpf $TARBALL
SOURCE_DIR=$(echo tagfs-*)

VERSION=${SOURCE_DIR#tagfs-}
cd $SOURCE_DIR
fakeroot tar xpf $DEBIAN_TARBALL
debuild && cp $BUILD/tagfs_${VERSION}*.deb $BUILD/tagfs.deb

