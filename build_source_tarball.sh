#!/bin/sh

PROJECT_ROOT="$(pwd)"
MAKE_BUILDDIR=${BUILDDIR:-build} # We have to preserve this because it has to match exactly in the target
BUILDDIR=$(readlink -nf $MAKE_BUILDDIR)

die () {
    echo $1 >&2
    exit 123
}

get_version () {
    TAG_PREFIX=${TAG_PREFIX:-version_}
    tag=$(cat ${PROJECT_ROOT}/version)
    version=${tag#${TAG_PREFIX}}

    if [ ! $version ] ; then
        echo No appropriate version tag could be found. Tag some commit like ${TAG_PREFIX}'<version>' to create a versioned tarball >&2
    fi
    echo -n $version
}

mkdir -p $BUILDDIR 2>/dev/null 
if [ ! -d $BUILDDIR ] ; then
    die "Couldn't create the build directory"
fi

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
    echo distfiles
}

make_tar_file () {
    DEST_NAME="$1"
    DIR="$BUILDDIR/$DEST_NAME"
    NAME=$2
    PREFIX="$3"
    DIR=$(readlink -nf $DIR)
    BUILDDIR=$(readlink -nf $BUILDDIR)
    S=${DIR#${BUILDDIR}}
    if [ "$S" = "${DIR}" ] ; then
        echo "Given tar file '$S' name is not within the build directory '$DIR'" >&2
        return 122
    fi

    mkdir -p $DIR 2>/dev/null
    cd "$DIR"

    while read f; do
        src="$PROJECT_ROOT/$f"
        if [ ${PREFIX} ] ; then
            dst="${PREFIX}/$f"
        else
            dst=$f
        fi
        mkdir -p $(dirname $dst) 2> /dev/null
        cp -p "$src" "$dst"
    done
    fakeroot tar cjpf "$BUILDDIR/$NAME" --numeric-owner "$DEST_NAME"
    cd -
    rm -rf "$DIR"

    echo $NAME: >&2
    tar tvpf "$BUILDDIR/$NAME" >&2
}

FILE_LIST=$(make_files_list)

if [ $list_deps ] ; then
    if [ $debian ] ; then
        echo ${MAKE_BUILDDIR}/tagfs.orig.tar.bz2: $(echo -n $(grep -v '^debian/' "$FILE_LIST"))
        echo ${MAKE_BUILDDIR}/tagfs.debian.tar.bz2: $(echo -n $(grep '^debian/' "$FILE_LIST"))
    else
        echo ${MAKE_BUILDDIR}/tagfs.tar.bz2: $(echo -n $(cat $FILE_LIST))
    fi
elif [ $debian ] ; then
    VERSION=${VERSION:-$(get_version)}
    BASE_NAME="tagfs-${VERSION:?}"
    grep -v '^debian/' "$FILE_LIST" | make_tar_file "$BASE_NAME" tagfs_${VERSION}.orig.tar.bz2 "$BASE_NAME" || die "Couldn't make tar file"
    grep '^debian/' "$FILE_LIST" | make_tar_file "debian" tagfs_${VERSION}.debian.tar.bz2 "" || die "Couldn't make tar file"
else
    VERSION=${VERSION:-$(get_version)}
    BASE_NAME="tagfs-${VERSION:?}"
    make_tar_file "$BASE_NAME" tagfs.tar.bz2 "$BASE_NAME" < $FILE_LIST || die "Couldn't make tar file"
fi

