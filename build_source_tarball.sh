#!/bin/sh

TMPDIR="$(mktemp -d)"
HERE=$(pwd)
FILE_LIST="$TMPDIR/files"
BASE_NAME="tagfs-$(git describe --abbrev=10 --dirty)"
BASE_NAME=$(echo -n "$BASE_NAME" | sed 's/version_//')
DIR="$TMPDIR/$BASE_NAME"
mkdir $DIR
git ls-files . > "$FILE_LIST"
cd "$DIR"
while read f; do
    src="$HERE/$f"
    install -D "$src" "$f"
done < "$FILE_LIST"
cd $TMPDIR
tar cjf "$HERE/tagfs.tar.bz2" "$BASE_NAME"
