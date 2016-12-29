#!/bin/sh

TMPDIR="$(mktemp -d)"

cleanup () {
    rm -rf $TMPDIR
}

trap cleanup EXIT 

echo $TMPDIR

TAG_PREFIX=${TAG_PREFIX:-version_}
tag=miku_hatsune
tag_search_start=${1:-HEAD}

while [ "$tag" -a \( "x${tag#${TAG_PREFIX}}" = "x${tag}" \) ] ; do
    tag=$(git describe --abbrev=0 ${tag_search_start} 2>/dev/null)
    tag_search_start=${tag}
done
tag=${tag#${TAG_PREFIX}}

if [ ! $tag ] ; then
    echo No appropriate version tag could be found. Tag some commit like ${TAG_PREFIX}'<version>' to create a versioned tarball >&2
    exit 128
fi

BASE_NAME="tagfs-${tag}"


make_files_list () {
    file_list="$TMPDIR/files"
    git ls-files . > "$file_list"
    echo "src/version.h" >> "$file_list"
    echo $file_list
}

FILE_LIST=$(make_files_list)

if [ "$1" = list_deps ] ; then
    cat $FILE_LIST
else
    HERE=$(pwd)

    DIR="$TMPDIR/$BASE_NAME"
    mkdir $DIR

    cd "$DIR"
    while read f; do
        src="$HERE/$f"
        install -D "$src" "$f"
    done < "$FILE_LIST"
    cd $TMPDIR
    tar cjf "$HERE/tagfs.tar.bz2" "$BASE_NAME"
fi

