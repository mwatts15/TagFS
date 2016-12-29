#!/bin/sh

TMPDIR="$(mktemp -d)"

cleanup () {
    rm -rf $TMPDIR
}

trap cleanup EXIT 

get_version () {
    TAG_PREFIX=${TAG_PREFIX:-version_}
    tag=miku_hatsune
    tag_search_start=$1

    while [ "$tag" -a \( "x${tag#${TAG_PREFIX}}" = "x${tag}" \) ] ; do
        tag=$(git describe --abbrev=0 "${tag_search_start}" 2>/dev/null)
        tag_search_start=${tag}
    done

    tag=${tag#${TAG_PREFIX}}

    if [ ! $tag ] ; then
        echo No appropriate version tag could be found. Tag some commit like ${TAG_PREFIX}'<version>' to create a versioned tarball >&2
    fi
    echo -n $tag
}

while getopts lv: f ; do
    case $f in
    l)  list_deps=1;;
    v)  VERSION=$f;;
    \?) echo $USAGE; exit 1;;
    esac
done
shift `expr $OPTIND - 1`

tag_search_start=${1:-HEAD}
VERSION=${VERSION:-$(get_version "$tag_search_start")}
BASE_NAME="tagfs-${VERSION:?}"


make_files_list () {
    file_list="$TMPDIR/files"
    git ls-files . > "$file_list"
    echo "src/version.h" >> "$file_list"
    echo $file_list
}

FILE_LIST=$(make_files_list)

if [ $list_deps ] ; then
    echo tagfs.tar.bz2: $(echo -n $(cat $FILE_LIST))
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

