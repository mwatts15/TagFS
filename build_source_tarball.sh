#!/bin/sh

BUILDDIR="$(pwd)/build"
mkdir $BUILDDIR

get_version () {
    if [ ! -d .git ] ; then 
        return
    fi
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

while getopts ldv: f ; do
    case $f in
    l)  list_deps=1;;
    d)  debian=1;;
    v)  VERSION=$f;;
    \?) echo $USAGE; exit 1;;
    esac
done
shift `expr $OPTIND - 1`

make_files_list () {
    file_list="$BUILDDIR/files"
    if [ -d .git ] ; then
        git ls-files . > "$file_list"
        echo "src/version.h" >> "$file_list"
    fi
    echo $file_list
}

make_tar_file () {
    DEST_NAME="$1"
    DIR="$BUILDDIR/$DEST_NAME"
    NAME=$2
    PREFIX="$3"
    DIR=$(readlink -f $DIR)
    if [ ${DIR#$BUILDDIR} = ${DIR} ] ; then
        echo "Given tar file name is not within the build directory" >&2
        return 122
    fi

    mkdir -p $DIR
    cd "$DIR"

    while read f; do
        src="$PROJECT_ROOT/$f"
        dst=${PREFIX}/$f
        mkdir -p $(dirname $dst)
        cp -p "$src" "$dst"
    done
    cd $BUILDDIR
    fakeroot tar cjpf "$BUILDDIR/$NAME" "$DEST_NAME"
    echo $NAME >&2
    tar tvpf "$BUILDDIR/$NAME" >&2
}

FILE_LIST=$(make_files_list)

if [ $list_deps ] ; then
    echo tagfs.tar.bz2: $(echo -n $(cat $FILE_LIST))
elif [ $debian ] ; then
    tag_search_start=${1:-HEAD}
    VERSION=${VERSION:-$(get_version "$tag_search_start")}
    BASE_NAME="tagfs-${VERSION:?}"

    PROJECT_ROOT=$(pwd)

    grep -v -e '^debian/' "$FILE_LIST" | make_tar_file "$BASE_NAME" tagfs_${VERSION}.orig.tar.bz2 || die "Couldn't make tar file"
    grep '^debian/' "$FILE_LIST" | make_tar_file "." tagfs_${VERSION}.debian.tar.bz2 || die "Couldn't make tar file"
    #DIR="$BUILDDIR/$BASE_NAME-debian"
    #mkdir $DIR
    #cd "$DIR"
    #cat "$FILE_LIST" | grep '^debian/' while read f; do
        #src="$PROJECT_ROOT/$f"
        #mkdir -p $(dirname $f)
        #cp -p "$src" "$f"
    #done

    #fakeroot tar cjpf "$BUILDDIR/tagfs_${VERSION}.debian.tar.bz2" "$BASE_NAME"
    #tar tvpf "$BUILDDIR/tagfs_${VERSION}.debian.tar.bz2" >&2

else
    tag_search_start=${1:-HEAD}
    VERSION=${VERSION:-$(get_version "$tag_search_start")}
    BASE_NAME="tagfs-${VERSION:?}"

    PROJECT_ROOT=$(pwd)

    DIR="$BUILDDIR/$BASE_NAME"
    mkdir $DIR

    cd "$DIR"
    while read f; do
        src="$PROJECT_ROOT/$f"
        mkdir -p $(dirname $f)
        cp -p "$src" "$f"
    done < "$FILE_LIST"
    cd $BUILDDIR
    fakeroot tar cjpf "$PROJECT_ROOT/tagfs.tar.bz2" "$BASE_NAME"
    tar tvpf "$PROJECT_ROOT/tagfs.tar.bz2" >&2
fi

