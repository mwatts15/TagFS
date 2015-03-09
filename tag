#!/bin/sh

mkdir -p "$2"
base=`basename $1`
mv "$1" "$2/$base"
